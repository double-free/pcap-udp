#include "preprocessor.h"
#include "utils.h"

using namespace md;

bool is_valid_packet(const udphdr &udp_header, const u_char *udp_payload) {
  uint16_t length = ntohs(udp_header.len);

  // we always assume this is a market data packet
  const auto *payload_header =
      reinterpret_cast<const md::UdpPayload *>(udp_payload);

  if (length ==
      payload_header->body_size() + sizeof(udphdr) + sizeof(md::UdpPayload)) {
    return true;
  }
  std::cerr << "find a udp packet does not match our protocol, source port: "
            << ntohs(udp_header.source)
            << ", dest port: " << ntohs(udp_header.dest)
            << ", length: " << length << std::endl;
  md::print_hex_array(udp_payload, length);
  return false;
}

void MdPreprocessor::process(const udphdr &udp_header,
                             const u_char *udp_payload) {

  assert(udp_payload != nullptr);
  if (is_valid_packet(udp_header, udp_payload) == false) {
    return;
  }

  const auto &payload = *reinterpret_cast<const UdpPayload *>(udp_payload);

  auto &msg_manager = msg_managers_[payload.channel_id()];

  msg_manager.handle(payload);

  const Message *msg = msg_manager.consume_message(payload.sequence_id());
  if (msg == nullptr) {
    return;
  }

  const u_char *raw_md = uncompress_message(*msg);
  if (raw_md == 0) {
    return;
  }

  // Debug
  // std::cout << "handle market data with size " << msg->size_before_compress()
  //           << ": ";
  // print_hex_array(raw_md, msg->size_before_compress());

  md_handler_(raw_md, msg->size_before_compress());
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
              << ", skip this message: " << '\n';

    print_hex_array(message.body() - sizeof(Message),
                    sizeof(Message) + message.size_after_compress());
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

bool MessageManager::handle(const UdpPayload &payload) {
  if (payload.sequence_id() <= last_seq_id_) {
    // outdated, drop it
    std::cerr << "channel " << payload.channel_id()
              << " dropped a outdated packet with seq: "
              << payload.sequence_id() << ", current seq: " << last_seq_id_
              << '\n';
    print_hex_array(reinterpret_cast<const u_char *>(&payload),
                    sizeof(UdpPayload) + payload.body_size());
    return false;
  }

  // seq gap, print a warn and ignore
  if (payload.sequence_id() > last_seq_id_ + 1) {
    std::cerr << " channel " << payload.channel_id()
              << " gets seq gap in packet with seq: " << payload.sequence_id()
              << ", current seq: " << last_seq_id_ << '\n';
    print_hex_array(reinterpret_cast<const u_char *>(&payload),
                    sizeof(UdpPayload) + payload.body_size());
  }

  if (payload.total_packet_number() != 1) {
    store(payload);
    return false;
  }

  // we get the "next" packet with a "whole" message
  // set the pointer and wait for consume_message() call
  realtime_msg_ = reinterpret_cast<const Message *>(payload.body());
  return true;
}

void MessageManager::store(const UdpPayload &payload) {
  if (storage_.find(payload.sequence_id()) == storage_.end()) {
    storage_[payload.sequence_id()].reserve(payload.total_packet_number());
  }

  uint32_t packet_index =
      payload.current_packet_index() + payload.initial_packet_index();
  // copy to make sure message staying valid
  storage_[payload.sequence_id()].fill(packet_index, payload.body(),
                                       payload.body_size());
}

// can be null
// update last_seq_id_ if successfully consume a message
const Message *MessageManager::consume_message(int64_t seq_id) {
  if (realtime_msg_ != nullptr) {
    last_seq_id_ = seq_id;
    const auto *tmp = realtime_msg_;
    // set realtime_msg_ to nullptr because we "consumed" it
    // we do not own it so do not need to delete it
    // it is a pointer to the original udp buffer
    realtime_msg_ = nullptr;
    return tmp;
  }

  // try to construct from cache
  if (storage_.find(seq_id) == storage_.end()) {
    return nullptr;
  }

  if (storage_[seq_id].full() == false) {
    return nullptr;
  }

  last_seq_id_ = seq_id;
  cached_msg_ = storage_[seq_id].consume();
  // clear the entry
  storage_.erase(seq_id);
  return reinterpret_cast<const Message *>(cached_msg_.get());
}
