#pragma once

#include <algorithm>
#include <iostream>
#include <map>

#include <endian.h>

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
  uint16_t compressed;
  uint32_t length1;
  uint32_t length2;
  uint32_t length3;
};

void parse_message(const u_char *message) {
  if (message == nullptr) {
    return;
  }
  const auto *header = reinterpret_cast<const MessageHeader *>(message);
  if (header->compressed == 0x0001) {
    // TODO: decompress
    std::cout << "find a compressed packet" << '\n';
  }
}

class MessageParser {
public:
  // key: sequence_id, value: payloads under this sequence_id
  using Cache = std::map<uint64_t, std::vector<std::vector<u_char>>>;

  // return how many message processed
  int parse_data(const u_char *data) {
    if (data == nullptr) {
      return 0;
    }

    auto *payload = reinterpret_cast<const PayloadHeader *>(data);
    if (payload->total_packet_number() == 1) {
      parse_message(data + sizeof(PayloadHeader));
      return 1;
    }

    save_to_cache(payload);

    return try_to_process_cache(cache_[payload->channel_id()]);
  }

private:
  // store splitted messages for each channel
  std::map<uint32_t, Cache> cache_;

  void save_to_cache(const PayloadHeader *payload) {
    auto &per_channel_cache = cache_[payload->channel_id()];
    if (per_channel_cache[payload->sequence_id()].empty()) {
      per_channel_cache[payload->sequence_id()].resize(
          payload->total_packet_number());
    }

    uint32_t packet_index =
        payload->current_packet_index() + payload->initial_packet_index();
    auto &char_vec = per_channel_cache[payload->sequence_id()][packet_index];
    char_vec.reserve(payload->body_size());

    // copy to make sure message staying valid
    const u_char *body =
        reinterpret_cast<const u_char *>(payload) + sizeof(PayloadHeader);
    for (int i = 0; i < payload->body_size(); i++) {
      char_vec.push_back(body[i]);
    }
  }

  int try_to_process_cache(Cache &cache) {
    std::vector<uint64_t> received_sequence_ids;

    for (const auto &kv : cache) {
      uint64_t seq = kv.first;
      const auto &char_vecs = kv.second;
      bool received = std::all_of(
          char_vecs.begin(), char_vecs.end(),
          [](const std::vector<u_char> &vec) { return vec.empty() == false; });
      if (received) {
        received_sequence_ids.push_back(seq);
      }
    }

    int message_count = received_sequence_ids.size();

    for (uint64_t seq_id : received_sequence_ids) {
      int length = 0;
      const auto &char_vecs = cache[seq_id];
      std::for_each(
          char_vecs.begin(), char_vecs.end(),
          [&length](const std::vector<u_char> &vec) { length += vec.size(); });

      u_char *concatenate_message = new u_char[length];

      int index = 0;
      for (const auto &char_vec : char_vecs) {
        for (u_char c : char_vec) {
          concatenate_message[index] = c;
          ++index;
        }
      }

      parse_message(concatenate_message);

      // not used any more, reclaim
      delete concatenate_message;
      cache.erase(seq_id);
    }

    return message_count;
  }
};
