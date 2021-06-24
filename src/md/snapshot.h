#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <cassert>

namespace md
{
    struct __attribute__((packed)) SnapshotHeader
    {
        int64_t be_orig_time;
        uint16_t be_channel_no;
        char md_stream_id[3];
        char security_id[8];
        char security_id_source[4];
        char trade_phase_code[8];
        int64_t be_prev_close_price;
        int64_t be_total_trade_num;
        int64_t be_total_trade_volume;
        int64_t be_total_trade_value;
        uint32_t be_md_entry_num;

        int64_t orig_time() const
        {
            return be64toh(be_orig_time);
        }

        int64_t total_trade_num() const
        {
            return be64toh(be_total_trade_num);
        }

        int64_t total_trade_volume() const
        {
            return be64toh(be_total_trade_volume);
        }

        int64_t total_trade_value() const
        {
            return be64toh(be_total_trade_value);
        }

        uint32_t md_entry_num() const
        {
            return be32toh(be_md_entry_num);
        }
    };

    enum class MdEntryType : uint16_t
    {
        Buy = 0x2030,
        Sell = 0x2031,
        Latest = 0x2032,
        Open = 0x2034,
    };

    struct __attribute__((packed)) MarketDataEntry
    {
        uint16_t be_md_entry_type;
        int64_t be_md_entry_price;
        int64_t be_md_entry_size;
        uint16_t be_md_price_level;
        int64_t be_number_of_orders;
        uint32_t be_number_of_quantity_awared_orders;

        uint32_t number_of_quantity_awared_orders() const
        {
            return be32toh(be_number_of_quantity_awared_orders);
        }

        MdEntryType md_entry_type() const
        {
            return static_cast<MdEntryType>(be_md_entry_type);
        }

        int64_t price() const
        {
            return be64toh(be_md_entry_price);
        }
        int64_t quantity() const
        {
            return be64toh(be_md_entry_size);
        }
        uint16_t price_level() const
        {
            return be16toh(be_md_price_level);
        }
    };

    struct BookLevel
    {
        int64_t price{0};
        int64_t quantity{0};
    };

    class SnapshotWrapper
    {
    public:
        SnapshotWrapper()
        {
            const int max_level_num = 10;
            bid_book_.resize(max_level_num + 1);
            ask_book_.resize(max_level_num + 1);
        }

        void init(const u_char *raw_data);

        const BookLevel &get_bid_level(int n) const
        {
            return bid_book_[n];
        }

        const BookLevel &get_ask_level(int n) const
        {
            return ask_book_[n];
        }

        const int depth() const
        {
            return bid_book_.size() - 1;
        }

        std::string security_id;
        std::string orig_time;
        int64_t total_trade_num{0};
        int64_t total_trade_volume{0};
        int64_t total_trade_value{0};
        int64_t latest_trade_price{0};
        int64_t open_price{0};

    private:
        std::vector<BookLevel> bid_book_;
        std::vector<BookLevel> ask_book_;

        // return how many bytes parsed
        int parse_extended_fields(const MarketDataEntry &);
    };
}
