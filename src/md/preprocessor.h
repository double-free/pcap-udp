#pragma once

#include "../pcap/udp_packet_processor.h"

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
struct __attribute__((packed)) UdpPayload {
  int64_t be_sequence_id;
  uint32_t be_channel_id;
  uint16_t be_total_packet_number;
  uint16_t be_initial_packet_index;
  uint16_t be_current_packet_index;
  uint32_t be_body_size;

  int64_t sequence_id() const { return be64toh(be_sequence_id); }
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

  const u_char *body() const {
    return reinterpret_cast<const u_char *>(this) + sizeof(*this);
  }
};

struct __attribute__((packed)) Message {
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

  const u_char *body() const {
    return reinterpret_cast<const u_char *>(this) + sizeof(*this);
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
    // std::move is needed because it lives longer than the scope
    return std::move(raw_data_);
  }

private:
  std::vector<bool> filled_;
  std::unique_ptr<u_char[]> raw_data_;
};

// for every channel, the manager works to:
//  1. construct message from udp packets
//  2. drop outdated/duplicated udp packet
class MessageManager {
public:
  // returns whether the payload contains a new message
  bool handle(const UdpPayload &payload);
  const Message *consume_message(int64_t seq_id);

private:
  // this is set to the latest "processed" message, -1 means no previous message
  // we only drop outdated message, do nothing for out-ordered ones
  int64_t last_seq_id_{-1};
  std::map<int64_t, Buffer> storage_;
  void store(const UdpPayload &payload);

  // message from current packet, guaranteed to be a whole message or nullptr
  const Message *realtime_msg_{nullptr};
  // message constructed from storage
  std::unique_ptr<u_char[]> cached_msg_;
};

// preprocessor of market data
// it mainly does two things:
//  1. construct message from udp packets
//  2. uncompress message if needed
class MdPreprocessor : public UdpPacketProcessor {
public:
  using MdHandler = std::function<void(const u_char *, uint32_t)>;

  MdPreprocessor(std::string net, std::string netmask, MdHandler handler)
      : UdpPacketProcessor(net, netmask), md_handler_(handler) {}

  // override UdpPacketProcessor::process
  void process(const udphdr &udp_header, const u_char *udp_payload) override;

private:
  MdHandler md_handler_;

  // we need to hold these message until next comes
  std::unique_ptr<u_char[]> decompressed_message_;
  const u_char *uncompress_message(const Message &message);

  // for each channel, there is a message manager
  std::map<uint32_t, MessageManager> msg_managers_;
};
} // namespace md
