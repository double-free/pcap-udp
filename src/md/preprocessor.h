#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include <cassert>
#include <endian.h>
#include <zlib.h>

namespace md {
struct __attribute__((packed)) PayloadHeader {
  uint64_t be_sequence_id;
  uint32_t be_channel_id;
  uint16_t be_total_packet_number;
  uint16_t be_initial_packet_index;
  uint16_t be_current_packet_index;
  uint32_t be_body_size;

  uint64_t sequence_id() const { return be64toh(be_sequence_id); }
  uint32_t channel_id() const { return be32toh(be_channel_id); }
  uint16_t total_packet_number() const {
    return be16toh(be_total_packet_number);
  }
  uint16_t initial_packet_index() const {
    return be16toh(be_initial_packet_index);
  }
  uint16_t current_packet_index() const {
    return be16toh(be_current_packet_index);
  }
  uint32_t body_size() const { return be32toh(be_body_size); }
};

struct __attribute__((packed)) MessageHeader {
  uint32_t unknown_number; // 0x00640000
  uint16_t be_compressed;
  uint32_t be_size_before_compress;
  uint32_t be_size_after_compress;
  uint32_t be_length3; // same as be_size_after_compress

  bool compressed() const { return be_compressed == 0x0001; }

  uint32_t size_before_compress() const {
    return be32toh(be_size_before_compress);
  }

  uint32_t size_after_compress() const {
    return be32toh(be_size_after_compress);
  }
};

// buffer stores all packets received under a sequence id
class Buffer {
public:
  static const uint32_t MAX_PACKET_LEN = 0x054e;

  // reserve space for n packets
  void reserve(uint16_t packet_num) {
    raw_data_.reset(new u_char[packet_num * MAX_PACKET_LEN]);
    filled_.resize(packet_num, false);
  }

  void fill(uint16_t packet_index, const u_char *src, uint32_t length);

  bool full() const {
    return filled_.empty() == false &&
           std::all_of(filled_.begin(), filled_.end(),
                       [](bool x) { return x; });
  }

  std::unique_ptr<u_char[]> consume() {
    assert(full());
    // std::move is needed because it is not a stack object
    return std::move(raw_data_);
  }

private:
  std::vector<bool> filled_;
  std::unique_ptr<u_char[]> raw_data_;
};

class Cache {
public:
  void save(const PayloadHeader *payload);
  std::unique_ptr<const u_char[]> construct_message(uint64_t seq_id);
  uint64_t get_last_seq_id() const { return last_seq_id_; }

private:
  uint64_t last_seq_id_{0};
  std::map<uint64_t, Buffer> storage_;
};

// preprocessor of market data
// it mainly does two things:
//  1. construct message from udp packets
//  2. uncompress message if needed

class MdPreprocessor {
public:
  using MdHandler = std::function<void(const u_char *, uint32_t)>;

  explicit MdPreprocessor(MdHandler handler) : md_handler_(handler) {}

  // feed in a new udp packet
  // return the message processed
  int process(const u_char *udp_payload);

  // return the message constructed after receiving the data
  // can be null
  const u_char *try_to_construct_message(const u_char *data);

  const u_char *uncompress_message(const u_char *message);

private:
  MdHandler md_handler_;

  // we need to hold these message until next comes
  std::unique_ptr<u_char[]> decompressed_message_;
  std::unique_ptr<const u_char[]> concatenated_message_;

  std::map<uint32_t, Cache> cache_;
};
} // namespace md
