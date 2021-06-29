#include "md/arbitrator.h"
#include "md/preprocessor.h"
#include "md/utils.h"
#include "pcap/pcap_reader.h"

#include "csv/writer.h"

#include <iostream>
#include <set>

std::set<uint32_t> get_interested_stocks(std::string file) {
  std::set<uint32_t> stock_ids;
  std::ifstream f(file);
  std::string str;
  while (std::getline(f, str)) {
    stock_ids.insert(std::stoul(str));
  }

  return stock_ids;
}

int main(int argc, char const *argv[]) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0]
              << " <pcap file> <stock filter> <output prefix>" << '\n';
    return 1;
  }

  auto interested_stock_ids = get_interested_stocks(argv[2]);
  std::string output_prefix = std::string(argv[3]);

  auto reader = PcapReader(argv[1]);

  std::string order_header =
      R"(clockAtArrival,sequenceNo,exchId,securityType,__isRepeated,TransactTime,ChannelNo,ApplSeqNum,SecurityID,secid,mdSource,)"
      R"(Side,OrderType,__origTickSeq,Price,OrderQty)";
  std::string trade_header =
      R"(clockAtArrival,sequenceNo,exchId,securityType,__isRepeated,TransactTime,ChannelNo,ApplSeqNum,SecurityID,secid,mdSource,)"
      R"(ExecType,TradeBSFlag,__origTickSeq,TradePrice,TradeQty,TradeMoney,BidApplSeqNum,OfferApplSeqNum)";
  std::string snapshot_header =
      R"(ms,clock,threadId,clockAtArrival,sequenceNo,source,StockID,exchange,time,cum_volume,cum_amount,close,__origTickSeq,)"
      R"(bid1p,bid2p,bid3p,bid4p,bid5p,bid1q,bid2q,bid3q,bid4q,bid5q,ask1p,ask2p,ask3p,ask4p,ask5p,ask1q,ask2q,ask3q,ask4q,ask5q,)"
      R"(openPrice,numTrades)";

  csv::Writer order_writer(output_prefix + "_order.csv", order_header);
  csv::Writer trade_writer(output_prefix + "_trade.csv", trade_header);
  csv::Writer snapshot_writer(output_prefix + "_snapshot.csv", snapshot_header);
  std::map<md::MessageType, int> unhandled_message_count;

  md::MdArbitrator arbitrator;
  auto md_handler = [&](const u_char *data, uint32_t data_len) {
    md::PackedMarketData mds(data, data_len);
    for (const md::MdHeader *header = mds.next_md(); header != nullptr;
         header = mds.next_md()) {
      const u_char *body =
          reinterpret_cast<const u_char *>(header) + sizeof(md::MdHeader);
      switch (header->message_type()) {
      case md::MessageType::Order: {
        const auto &order = *reinterpret_cast<const md::Order *>(body);
        if (!arbitrator.record_order_or_trade(order.channel_no(),
                                              order.appl_seq_num())) {
          break;
        }
        uint32_t stock_id = std::stoul(
            md::bytes_to_str(order.security_id, sizeof(order.security_id)));
        if (interested_stock_ids.find(stock_id) != interested_stock_ids.end()) {
          order_writer.write_order(order,
                                   get_pcap_timestamp(reader.pcap_header()),
                                   reader.udp_packet_index());
        }

        break;
      }
      case md::MessageType::Trade: {
        const auto &trade = *reinterpret_cast<const md::Trade *>(body);

        if (!arbitrator.record_order_or_trade(trade.channel_no(),
                                              trade.appl_seq_num())) {
          break;
        }
        uint32_t stock_id = std::stoul(
            md::bytes_to_str(trade.security_id, sizeof(trade.security_id)));
        if (interested_stock_ids.find(stock_id) != interested_stock_ids.end()) {
          trade_writer.write_trade(trade,
                                   get_pcap_timestamp(reader.pcap_header()),
                                   reader.udp_packet_index());
        }

        break;
      }
      case md::MessageType::Snapshot: {
        md::SnapshotWrapper snapshot_wrapper(body); // max 10 levels
        if (!arbitrator.record_snapshot(snapshot_wrapper.security_id,
                                        snapshot_wrapper.orig_time)) {
          break;
        }
        uint32_t stock_id = std::stoul(snapshot_wrapper.security_id);
        if (interested_stock_ids.find(stock_id) != interested_stock_ids.end()) {
          // only write top 5 levels
          snapshot_wrapper.init_md_entries();
          int depth = 5;
          snapshot_writer.write_snapshot(
              snapshot_wrapper, get_pcap_timestamp(reader.pcap_header()),
              reader.udp_packet_index(), depth);
        }
        break;
      }

      case md::MessageType::SnapshotStats:
      case md::MessageType::Heartbeat: {
        // ignored
        break;
      }

      default:
        // unhandled message
        unhandled_message_count[header->message_type()] += 1;
      }
    }
  };

  // TODO: hard-coded address filter
  // arbitrate between two feeds
  std::string net1("172.27.1");
  std::string net2("172.27.129");
  if (reader.set_filter("net " + net1 + " or net " + net2) != 0) {
    std::cerr << "set filter failed" << '\n';
    return 2;
  }
  std::string netmask("255.255.255.0");
  md::MdPreprocessor processor1(net1 + ".0", netmask, md_handler);
  md::MdPreprocessor processor2(net2 + ".0", netmask, md_handler);
  reader.add_processor(&processor1);
  reader.add_processor(&processor2);
  int udp_packet_count = reader.process();

  std::cout << udp_packet_count << " udp packets processed\n";
  for (const auto &kv : unhandled_message_count) {
    std::cerr << "unhandled message type: " << static_cast<uint32_t>(kv.first)
              << ", count: " << kv.second << '\n';
  }
  return 0;
}
