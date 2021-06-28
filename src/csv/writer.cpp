#include "writer.h"
#include "../md/utils.h"

using namespace csv;

void Writer::write_order(const md::Order &order, uint64_t pcap_ts,
                         uint64_t pcap_seq) {
  std::string security_id =
      md::bytes_to_str(order.security_id, sizeof(order.security_id));
  csv_file_ << pcap_ts << ',' << pcap_seq << ",2,1,0,"
            << order.transaction_time() % Writer::TIME_MULT << ','
            << order.channel_no() << ',' << order.appl_seq_num() << ','
            << security_id << ',' << "2" + security_id << ",24," << order.side
            << ',' << order.order_type << ",-1," << order.price() << ','
            << order.quantity() / Writer::QUANTITY_MULT << '\n';
}

void Writer::write_trade(const md::Trade &trade, uint64_t pcap_ts,
                         uint64_t pcap_seq) {
  std::string security_id =
      md::bytes_to_str(trade.security_id, sizeof(trade.security_id));
  csv_file_ << pcap_ts << ',' << pcap_seq << ",2,1,0,"
            << trade.transaction_time() % Writer::TIME_MULT << ','
            << trade.channel_no() << ',' << trade.appl_seq_num() << ','
            << security_id << ',' << "2" + security_id << ",24,"
            << trade.execute_type << ",N,-1," << trade.price() << ','
            << trade.quantity() / Writer::QUANTITY_MULT << ','
            << trade.price() * trade.quantity() / Writer::QUANTITY_MULT << ','
            << trade.bid_appl_seq_num() << ',' << trade.offer_appl_seq_num()
            << '\n';
}

void Writer::write_snapshot(const md::SnapshotWrapper &snapshot,
                            uint64_t pcap_ts, uint64_t pcap_seq, int depth) {
  // placeholders
  csv_file_ << "09:42:12.094767,1587606132095370,23994," << pcap_ts << ','
            << pcap_seq << ",24," << snapshot.security_id << ",SZ,"
            << md::timestamp_to_string(snapshot.orig_time) << ','
            << snapshot.total_trade_volume / Writer::QUANTITY_MULT << ','
            << 1.0 * snapshot.total_trade_value / Writer::AMOUNT_MULT << ','
            << 1.0 * snapshot.latest_trade_price / Writer::MD_PRICE_MULT
            << ",0,";

  for (int i = 1; i <= depth; i++) {
    csv_file_ << 1.0 * snapshot.get_bid_level(i).price / Writer::MD_PRICE_MULT
              << ',';
  }
  for (int i = 1; i <= depth; i++) {
    csv_file_ << snapshot.get_bid_level(i).quantity / Writer::QUANTITY_MULT
              << ',';
  }

  for (int i = 1; i <= depth; i++) {
    csv_file_ << 1.0 * snapshot.get_ask_level(i).price / Writer::MD_PRICE_MULT
              << ',';
  }
  for (int i = 1; i <= depth; i++) {
    csv_file_ << snapshot.get_ask_level(i).quantity / Writer::QUANTITY_MULT
              << ',';
  }

  csv_file_ << 1.0 * snapshot.open_price / Writer::MD_PRICE_MULT << ','
            << snapshot.total_trade_num << '\n';
}
