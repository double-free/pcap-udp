#pragma once

#include <endian.h>

namespace md
{
    enum class MessageType : uint32_t
    {
        Snapshot = 300111,
        Trade = 300191,
        Order = 300192,
    };

    struct __attribute__((packed)) MdHeader
    {
        uint16_t unknown;
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
}