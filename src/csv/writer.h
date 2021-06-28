#pragma once

#include "../md/order.h"
#include "../md/snapshot.h"
#include "../md/trade.h"
#include "../md/utils.h"

#include <fstream>

namespace csv {

class Writer {
public:
  explicit Writer(std::string outputfile, std::string header)
      : csv_file_(outputfile) {
    csv_file_ << std::setprecision(6) << std::fixed;
    csv_file_ << header << '\n';
  }

  void write_order(const md::Order &order, uint64_t pcap_ts, uint64_t pcap_seq);

  void write_trade(const md::Trade &trade, uint64_t pcap_ts, uint64_t pcap_seq);

  void write_snapshot(const md::SnapshotWrapper &snapshot, uint64_t pcap_ts,
                      uint64_t pcap_seq, int depth);

  static const int64_t TIME_MULT = 1000000000;
  static const int64_t PRICE_MULT = 10000;
  static const int64_t QUANTITY_MULT = 100;
  static const int64_t MD_PRICE_MULT = 1000000;
  static const int64_t AMOUNT_MULT = 10000; // cash
private:
  std::ofstream csv_file_;
};
} // namespace csv
