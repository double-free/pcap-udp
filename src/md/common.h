#pragma once

#include <endian.h>
#include <cstdint>
#include <iostream>

namespace md
{
    enum class MessageType : uint32_t
    {
        Snapshot = 300111,
        Trade = 300191,
        Order = 300192,
        Heartbeat = 390095,
        SnapshotStats = 390090,
    };

    struct __attribute__((packed)) MdHeader
    {
        uint32_t be_message_type;
        uint32_t be_body_size;

        MessageType message_type() const
        {
            return static_cast<MessageType>(be32toh(be_message_type));
        }

        uint32_t body_size() const
        {
            return be32toh(be_body_size);
        }
    };

    class PackedMarketData
    {
    public:
        explicit PackedMarketData(const unsigned char *raw_data, uint32_t len) : raw_data_(raw_data), data_len_(len)
        {
            const uint16_t *be_md_count = reinterpret_cast<const uint16_t *>(raw_data_);
            md_count_ = be16toh(*be_md_count);
        }

        uint16_t md_count() const
        {
            return md_count_;
        }

        const MdHeader *next_md()
        {
            if (raw_data_ == nullptr || index_ >= data_len_)
            {
                return nullptr;
            }
            const auto *header = reinterpret_cast<const MdHeader *>(raw_data_ + index_);
            index_ += sizeof(MdHeader) + header->body_size();
            return header;
        }

    private:
        uint16_t md_count_;
        uint32_t index_{sizeof(md_count_)};

        const uint32_t data_len_;
        const unsigned char *raw_data_{nullptr};
    };
}