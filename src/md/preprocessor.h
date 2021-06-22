#pragma once

#include "utils.h"

#include <functional>
#include <memory>
#include <map>
#include <vector>
#include <algorithm>
#include <iostream>

#include <cassert>
#include <zlib.h>
#include <endian.h>

struct __attribute__((packed)) PayloadHeader
{
    uint64_t be_sequence_id;
    uint32_t be_channel_id;
    uint16_t be_total_packet_number;
    uint16_t be_initial_packet_index;
    uint16_t be_current_packet_index;
    uint32_t be_body_size;

    uint64_t sequence_id() const { return be64toh(be_sequence_id); }
    uint32_t channel_id() const { return be32toh(be_channel_id); }
    uint16_t total_packet_number() const
    {
        return be16toh(be_total_packet_number);
    }
    uint16_t initial_packet_index() const
    {
        return be16toh(be_initial_packet_index);
    }
    uint16_t current_packet_index() const
    {
        return be16toh(be_current_packet_index);
    }
    uint32_t body_size() const { return be32toh(be_body_size); }
};

struct __attribute__((packed)) MessageHeader
{
    uint32_t unknown_number; // 0x00640000
    uint16_t be_compressed;
    uint32_t be_size_before_compress;
    uint32_t be_size_after_compress;
    uint32_t be_length3; // same as be_size_after_compress

    bool compressed() const
    {
        return be_compressed == 0x0001;
    }
};

// preprocessor of market data
// it mainly does two things:
//  1. construct message from udp packets
//  2. uncompress message if needed

class MdPreprocessor
{
public:
    using MdHandler = std::function<bool(const u_char *)>;

    explicit MdPreprocessor(MdHandler handler) : md_handler_(handler)
    {
    }

    ~MdPreprocessor()
    {
        for (const auto &kv : cache_)
        {
            for (const auto &kv2 : kv.second)
            {
                std::cerr << "WARN: unprocessed message with seq id " << kv2.first << " in " << kv.first << " channel" << '\n';
            }
        }
    }

    // feed in a new udp packet
    // return the message processed
    int process(const u_char *udp_payload)
    {
        const u_char *message = try_to_construct_message(udp_payload);
        if (message == nullptr)
        {
            return 0;
        }

        // uncompress if needed
        // PayloadHeader and MessageHeader is dropped after this function
        const u_char *raw_md = uncompress_message(message + sizeof(PayloadHeader));
        if (raw_md == nullptr)
        {
            return 0;
        }

        if (md_handler_(raw_md) == false)
        {
            // somehow the handling failed, print the payload
            std::cout << "failed to handle message: ";
            md::print_hex_array(udp_payload, sizeof(PayloadHeader) + sizeof(MessageHeader) + 64);
            return 0;
        }
        return 1;
    }

    // return the message constructed after receiving the data
    // can be null
    const u_char *try_to_construct_message(const u_char *data)
    {
        if (data == nullptr)
        {
            return nullptr;
        }

        auto *payload = reinterpret_cast<const PayloadHeader *>(data);
        if (payload->total_packet_number() == 1)
        {
            return data;
        }

        // it is splitted in several packets
        save_to_cache(payload);

        const u_char *concat_message = construct_message(cache_[payload->channel_id()], payload->sequence_id());

        return concat_message;
    }

    const u_char *uncompress_message(const u_char *message)
    {
        if (message == nullptr)
        {
            return nullptr;
        }

        const auto *header = reinterpret_cast<const MessageHeader *>(message);

        if (header->compressed() == false)
        {
            // not compressed
            return message + sizeof(MessageHeader);
        }

        // uncompress
        size_t decompressed_size = be32toh(header->be_size_before_compress);

        // TODO: avoid allocating in critical path
        decompressed_message_.reset(new u_char[decompressed_size]);

        int result = uncompress(decompressed_message_.get(), &decompressed_size, message + sizeof(MessageHeader), be32toh(header->be_size_after_compress));
        assert(decompressed_size == be32toh(header->be_size_before_compress));
        if (result != Z_OK)
        {
            std::cerr << "uncompress failed with code " << result << ", skip this message" << '\n';
            return nullptr;
        }
        return decompressed_message_.get();
    }

private:
    MdHandler md_handler_;
    std::unique_ptr<u_char[]> concatenated_message_{nullptr};
    std::unique_ptr<u_char[]> decompressed_message_{nullptr};

    // key: sequence_id, value: payloads under this sequence_id
    using Cache = std::map<uint64_t, std::vector<std::vector<u_char>>>;
    // store splitted messages for each channel
    std::map<uint32_t, Cache> cache_;

    void save_to_cache(const PayloadHeader *payload)
    {
        if (payload->body_size() > 1500)
        {
            std::cerr << "packet size too big: " << payload->body_size() << '\n';
            std::cerr << "sequence id: " << payload->sequence_id() << '\n';
            md::print_hex_array(reinterpret_cast<const u_char *>(payload), 128);
        }

        auto &per_channel_cache = cache_[payload->channel_id()];
        if (per_channel_cache[payload->sequence_id()].empty())
        {
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

        for (int i = 0; i < payload->body_size(); i++)
        {
            char_vec.push_back(body[i]);
        }
    }

    const u_char *construct_message(Cache &cache, uint64_t seq_id)
    {
        if (cache.find(seq_id) == cache.end())
        {
            return nullptr;
        }

        const auto &char_vecs = cache[seq_id];

        bool received = std::all_of(
            char_vecs.begin(), char_vecs.end(),
            [](const std::vector<u_char> &vec)
            { return vec.empty() == false; });
        if (received == false)
        {
            return nullptr;
        }

        int length = 0;
        std::for_each(
            char_vecs.begin(), char_vecs.end(),
            [&length](const std::vector<u_char> &vec)
            { length += vec.size(); });

        concatenated_message_.reset(new u_char[length]);
        u_char *buffer = concatenated_message_.get();

        int index = 0;
        for (const auto &char_vec : char_vecs)
        {
            for (u_char c : char_vec)
            {
                buffer[index] = c;
                ++index;
            }
        }
        // no longer used
        cache.erase(seq_id);

        return buffer;
    }
};