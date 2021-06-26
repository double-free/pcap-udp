#include "preprocessor.h"
#include "utils.h"

using namespace md;

int MdPreprocessor::process(const u_char *udp_payload) {
  // PayloadHeader is dropped
  const u_char *message = try_to_construct_message(udp_payload);
  if (message == nullptr) {
    return 0;
  }

  // uncompress if needed
  // MessageHeader is dropped
  const u_char *raw_md = uncompress_message(message);
  if (raw_md == nullptr) {
    return 0;
  }

  const MessageHeader *header =
      reinterpret_cast<const MessageHeader *>(message);

  // Debug
  // std::cout << "try to handle message with size "
  //           << header->size_before_compress() << ": ";
  // print_hex_array(raw_md, header->size_before_compress());

  md_handler_(raw_md, header->size_before_compress());

  return 1;
}

// return the message constructed after receiving the data
// can be null
const u_char *MdPreprocessor::try_to_construct_message(const u_char *data) {
  if (data == nullptr) {
    return nullptr;
  }

  auto *payload = reinterpret_cast<const PayloadHeader *>(data);
  if (payload->total_packet_number() == 1) {
    return data + sizeof(PayloadHeader);
  }

  // it is splitted in several packets
  cache_[payload->channel_id()].save(payload);

  concatenated_message_ =
      cache_[payload->channel_id()].construct_message(payload->sequence_id());

  return concatenated_message_.get();
}

const u_char *MdPreprocessor::uncompress_message(const u_char *message) {
  if (message == nullptr) {
    return nullptr;
  }

  const auto *header = reinterpret_cast<const MessageHeader *>(message);

  if (header->compressed() == false) {
    // not compressed
    return message + sizeof(MessageHeader);
  }

  // uncompress
  size_t decompressed_size = header->size_before_compress();

  // TODO: avoid allocating in critical path
  decompressed_message_.reset(new u_char[decompressed_size]);

  int result = uncompress(decompressed_message_.get(), &decompressed_size,
                          message + sizeof(MessageHeader),
                          header->size_after_compress());

  assert(decompressed_size == header->size_before_compress());

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

void Cache::save(const PayloadHeader *payload) {

  if (storage_.find(payload->sequence_id()) == storage_.end()) {
    storage_[payload->sequence_id()].reserve(payload->total_packet_number());
  }

  uint32_t packet_index =
      payload->current_packet_index() + payload->initial_packet_index();

  const u_char *body =
      reinterpret_cast<const u_char *>(payload) + sizeof(PayloadHeader);

  // copy to make sure message staying valid
  storage_[payload->sequence_id()].fill(packet_index, body,
                                        payload->body_size());
}

std::unique_ptr<const u_char[]> Cache::construct_message(uint64_t seq_id) {
  if (storage_.find(seq_id) == storage_.end()) {
    return nullptr;
  }

  if (storage_[seq_id].full() == false) {
    return nullptr;
  }

  return storage_[seq_id].consume();
}
