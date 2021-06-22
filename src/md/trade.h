#pragma once

#include "common.h"

namespace md
{
    struct __attribute__((packed)) Trade
    {
        uint16_t be_channel_no;
        uint64_t be_appl_seq_num;
        char md_stream_id[3];
        uint64_t be_bid_appl_seq_num;
        uint64_t be_offer_appl_seq_num;
        char security_id[8];
        char security_id_source[4];
        int64_t be_price;
        int64_t be_quantity;
        char execute_type;
        int64_t be_transaction_time;

        uint16_t channel_no() const
        {
            return be16toh(be_channel_no);
        }

        uint64_t appl_seq_num() const
        {
            return be64toh(be_appl_seq_num);
        }

        uint64_t bid_appl_seq_num() const
        {
            return be64toh(be_bid_appl_seq_num);
        }

        uint64_t offer_appl_seq_num() const
        {
            return be64toh(be_offer_appl_seq_num);
        }

        int64_t price() const
        {
            // price is not divided by 1e4
            return be64toh(be_price);
        }

        int64_t quantity() const
        {
            return be64toh(be_quantity);
        }

        int64_t transaction_time() const
        {
            return be64toh(be_transaction_time);
        }
    };
}
