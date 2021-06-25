#pragma once

#include <functional>
#include <memory>
#include <map>
#include <vector>
#include <algorithm>
#include <iostream>

#include <cassert>
#include <zlib.h>
#include <endian.h>

namespace md
{
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

        uint32_t size_before_compress() const
        {
            return be32toh(be_size_before_compress);
        }

        uint32_t size_after_compress() const
        {
            return be32toh(be_size_after_compress);
        }
    };

    // preprocessor of market data
    // it mainly does two things:
    //  1. construct message from udp packets
    //  2. uncompress message if needed

    class MdPreprocessor
    {
    public:
        using MdHandler = std::function<void(const u_char *, uint32_t)>;

        explicit MdPreprocessor(MdHandler handler) : md_handler_(handler)
        {
        }

        ~MdPreprocessor()
        {
            for (const auto &kv : cache_)
            {
                for (const auto &kv2 : kv.second)
                {
                    std::cerr << "WARN: unprocessed message with seq id "
                              << kv2.first << " in " << kv.first << " channel" << std::endl;
                }
            }
        }

        // feed in a new udp packet
        // return the message processed
        int process(const u_char *udp_payload);

        // return the message constructed after receiving the data
        // can be null
        const u_char *try_to_construct_message(const u_char *data);

        const u_char *uncompress_message(const u_char *message);

    private:
        MdHandler md_handler_;
        std::unique_ptr<u_char[]> concatenated_message_{nullptr};
        std::unique_ptr<u_char[]> decompressed_message_{nullptr};

        // key: sequence_id, value: payloads under this sequence_id
        using Cache = std::map<uint64_t, std::vector<std::vector<u_char>>>;
        // store splitted messages for each channel
        std::map<uint32_t, Cache> cache_;

        void save_to_cache(const PayloadHeader *payload);
        const u_char *construct_message(Cache &cache, uint64_t seq_id);
    };
}