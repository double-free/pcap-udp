#pragma once

#include "../md/order.h"
#include "../md/trade.h"
#include "../md/snapshot.h"
#include "../md/utils.h"

#include <fstream>

namespace csv
{
    class Writer
    {
    public:
        explicit Writer(std::string outputfile) : csv_file_(outputfile)
        {
        }

        void write_order(const md::Order &order)
        {
        }

        void write_trade(const md::Trade &trade, uint64_t pcap_ts, uint64_t pcap_seq)
        {
            if (has_header_ == false)
            {
                csv_file_ << "clockAtArrival,sequenceNo,exchId,securityType,__isRepeated,TransactTime,ChannelNo,"
                          << "ApplSeqNum,SecurityID,secid,mdSource,ExecType,TradeBSFlag,__origTickSeq,TradePrice,TradeQty,TradeMoney,BidApplSeqNum,OfferApplSeqNum" << '\n';
                has_header_ = true;
            }

            std::string security_id = md::bytes_to_str(trade.security_id, sizeof(trade.security_id));
            csv_file_ << pcap_ts << "," << pcap_seq << ",2,1,0," << trade.transaction_time() % 100000000 << ","
                      << trade.channel_no() << "," << trade.appl_seq_num() << "," << security_id << ","
                      << "2" + security_id << ",24," << trade.execute_type << ",N,-1," << trade.price() << ","
                      << trade.quantity() / 100 << "," << trade.price() * trade.quantity() / 100 << "," << trade.bid_appl_seq_num() << "," << trade.offer_appl_seq_num() << '\n';
        }

        void write_snapshot(const md::Snapshot &snapshot)
        {
        }

    private:
        bool has_header_{false};
        std::ofstream csv_file_;
    };
}
