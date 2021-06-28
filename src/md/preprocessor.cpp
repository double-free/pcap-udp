#include "preprocessor.h"
#include "utils.h"

using namespace md;

int MdPreprocessor::process(const u_char *udp_data) {

  assert(udp_data != nullptr);

  const auto &payload = *reinterpret_cast<const UdpPayload *>(udp_data);

  auto &msg_manager = msg_managers_[payload.channel_id()];

  if (payload.sequence_id() <= msg_manager.get_last_seq_id()) {
    // outdated packet
    return 0;
  }

  auto process_message = [this, &msg_manager](const Message &msg,
                                              int64_t seq_id) -> int {
    const u_char *raw_md = uncompress_message(msg);
    if (raw_md == 0) {
      return 0;
    }

    // Debug
    // std::cout << "handle market data with size " <<
    // msg.size_before_compress() << ": ";
    // print_hex_array(raw_md, msg.size_before_compress());

    md_handler_(raw_md, msg.size_before_compress());
    msg_manager.set_last_seq_id(seq_id);
    return 1;
  };

  int processed_message_count = 0;
  if (msg_manager.fast_path(payload) == true) {
    // the simplest case that we don't need to copy anything
    const auto &msg = *reinterpret_cast<const Message *>(payload.body());
    processed_message_count += process_message(msg, payload.sequence_id());
    // we don't return here,
    // because it may have some following data to process in cache
  } else {
    // store it in cache
    msg_manager.store(payload);
  }

  // check if there's anything to process in cache
  std::unique_ptr<char[]> maybe_msg;
  do {
    int64_t next_seq_id = msg_manager.get_last_seq_id() + 1;
    std::unique_ptr<const u_char[]> maybe_msg =
        msg_manager.construct_message(next_seq_id);
    if (maybe_msg != nullptr) {
      const auto &cached_msg =
          *reinterpret_cast<const Message *>(maybe_msg.get());
      processed_message_count += process_message(cached_msg, next_seq_id);
    }
  } while (maybe_msg != nullptr);

  return processed_message_count;
}

const u_char *MdPreprocessor::uncompress_message(const Message &message) {
  if (message.compressed() == false) {
    // not compressed
    return message.body();
  }

  // uncompress
  size_t decompressed_size = message.size_before_compress();

  // TODO: avoid allocating in critical path
  decompressed_message_.reset(new u_char[decompressed_size]);

  int result = uncompress(decompressed_message_.get(), &decompressed_size,
                          message.body(), message.size_after_compress());

  assert(decompressed_size == message.size_before_compress());

  if (result != Z_OK) {
    std::cerr << "uncompress failed with code " << result
              << ", skip this message" << '\n';
    return nullptr;
  }

  return decompressed_message_.get();
}

void Buffer::fill(uint16_t packet_index, const u_char *src, uint32_t length) {
  assert(packet_index < filled_.size());
  assert(length <= MAX_PACKET_LEN);

  if (filled_[packet_index]) {
    // skip duplicate filling
    return;
  }

  u_char *dst = raw_data_.get() + packet_index * MAX_PACKET_LEN;
  std::memcpy(dst, src, length);
  filled_[packet_index] = true;
}

bool MessageManager::fast_path(const UdpPayload &payload) const {
  if (payload.total_packet_number() != 1) {
    return false;
  }

  if (last_seq_id_ == -1 || payload.sequence_id() == last_seq_id_ + 1) {
    // we get the "next" packet with a "whole" message
    return true;
  }

  // received a late sequence and needs wait for the previous one
  return false;
}

void MessageManager::store(const UdpPayload &payload) {
  if (payload.sequence_id() > last_seq_id_ + 1) {
    std::cout << "store an out of sequence packet with seq: "
              << payload.sequence_id() << ", current seq: " << last_seq_id_
              << '\n';
    print_hex_array(reinterpret_cast<const u_char *>(&payload),
                    sizeof(UdpPayload) + sizeof(payload.body_size()));
  }

  if (storage_.find(payload.sequence_id()) == storage_.end()) {
    storage_[payload.sequence_id()].reserve(payload.total_packet_number());
  }

  uint32_t packet_index =
      payload.current_packet_index() + payload.initial_packet_index();
  // copy to make sure message staying valid
  storage_[payload.sequence_id()].fill(packet_index, payload.body(),
                                       payload.body_size());
}

std::unique_ptr<const u_char[]>
MessageManager::construct_message(int64_t seq_id) {
  if (storage_.find(seq_id) == storage_.end()) {
    return nullptr;
  }

  if (storage_[seq_id].full() == false) {
    return nullptr;
  }

  auto msg = storage_[seq_id].consume();
  // clear the entry
  storage_.erase(seq_id);
  return msg;
}
