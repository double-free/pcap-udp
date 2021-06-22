#pragma once

#include "common.h"

namespace md
{
    struct __attribute__((packed)) Order
    {
        uint16_t be_channel_no;
        uint64_t be_appl_seq_num;
        char md_stream_id[3];
        char security_id[8];
        char security_id_source[4];
        int64_t be_price;
        int64_t be_quantity;
        char side;
        int64_t be_transaction_time;
        char order_type;
    };
}
