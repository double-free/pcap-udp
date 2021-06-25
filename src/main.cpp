#include "md/preprocessor.h"
#include "pcap/pcap_reader.h"
#include "pcap/pcap_to_udp.h"

#include "csv/writer.h"

#include <iostream>

bool is_valid_packet(const u_char *udp_packet)
{
  const auto *udp_header = reinterpret_cast<const udphdr *>(udp_packet);

  uint16_t length = ntohs(udp_header->len);

  // we always assume this is a market data packet
  const auto *payload_header =
      reinterpret_cast<const md::PayloadHeader *>(udp_packet + sizeof(udphdr));

  if (length == payload_header->body_size() + sizeof(udphdr) + sizeof(md::PayloadHeader))
  {
    return true;
  }
  std::cout << "source port: " << ntohs(udp_header->source)
            << ", dest port: " << ntohs(udp_header->dest)
            << ", length: " << length << std::endl;
  md::print_hex_array(udp_packet, length);
  return false;
}

int main(int argc, char const *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << "<pcap file> <output prefix>" << '\n';
    return 1;
  }
  
  std::string output_prefix("md");
  if (argc >= 3) {
    output_prefix = std::string(argv[2]);
  }

  auto reader = PcapReader(argv[1]);
  // TODO: hard-coded address filter
  if (reader.set_filter("net 172.27.129") != 0)
  {
    std::cerr << "set filter failed" << '\n';
    return 2;
  }
  std::string order_header = R"(clockAtArrival,sequenceNo,exchId,securityType,__isRepeated,TransactTime,ChannelNo,ApplSeqNum,SecurityID,secid,mdSource,)"
                             R"(Side,OrderType,__origTickSeq,Price,OrderQty)";
  std::string trade_header = R"(clockAtArrival,sequenceNo,exchId,securityType,__isRepeated,TransactTime,ChannelNo,ApplSeqNum,SecurityID,secid,mdSource,)"
                             R"(ExecType,TradeBSFlag,__origTickSeq,TradePrice,TradeQty,TradeMoney,BidApplSeqNum,OfferApplSeqNum)";
  std::string snapshot_header = R"(ms,clock,threadId,clockAtArrival,sequenceNo,source,StockID,exchange,time,cum_volume,cum_amount,close,__origTickSeq,)"
                                R"(bid1p,bid2p,bid3p,bid4p,bid5p,bid1q,bid2q,bid3q,bid4q,bid5q,ask1p,ask2p,ask3p,ask4p,ask5p,ask1q,ask2q,ask3q,ask4q,ask5q,)"
                                R"(openPrice,numTrades)";

  csv::Writer order_writer(output_prefix + "_order.csv", order_header);
  csv::Writer trade_writer(output_prefix + "_trade.csv", trade_header);
  csv::Writer snapshot_writer(output_prefix + "_snapshot.csv", snapshot_header);

  std::map<md::MessageType, int> unhandled_message_count;
  auto md_handler = [&reader, &order_writer, &trade_writer, &snapshot_writer, &unhandled_message_count](const u_char *data, uint32_t data_len)
  {
    md::PackedMarketData mds(data, data_len);
    // std::cout << "got message with length " << data_len << ": ";
    // md::print_hex_array(data, data_len);
    for (const md::MdHeader *header = mds.next_md(); header != nullptr; header = mds.next_md())
    {
      const u_char *body = reinterpret_cast<const u_char *>(header) + sizeof(md::MdHeader);
      switch (header->message_type())
      {
      case md::MessageType::Order:
      {
        const auto *order = reinterpret_cast<const md::Order *>(body);
        order_writer.write_order(*order, get_pcap_timestamp(reader.pcap_header()), reader.pcap_packet_index());
        break;
      }
      case md::MessageType::Trade:
      {
        const auto *trade = reinterpret_cast<const md::Trade *>(body);
        trade_writer.write_trade(*trade, get_pcap_timestamp(reader.pcap_header()), reader.pcap_packet_index());
        break;
      }
      case md::MessageType::Snapshot:
      {
        md::SnapshotWrapper snapshot_wrapper; // max 10 levels
        snapshot_wrapper.init(body);
        // only write top 5 levels
        int depth = 5;
        snapshot_writer.write_snapshot(snapshot_wrapper, get_pcap_timestamp(reader.pcap_header()), reader.pcap_packet_index(), depth);
        break;
      }

      case md::MessageType::SnapshotStats:
      case md::MessageType::Heartbeat:
      {
        // ignored
        break;
      }

      default:
        // unhandled message
        unhandled_message_count[header->message_type()] += 1;
      }
    }
  };

  md::MdPreprocessor processor(md_handler);

  // processed message count
  int count = 0;

  const u_char *udp_packet = nullptr;
  do
  {
    udp_packet = reader.extract_from_pcap_packet(extract_udp_packet);
    if (udp_packet != nullptr && is_valid_packet(udp_packet))
    {
      count += processor.process(udp_packet + sizeof(udphdr));
    }
  } while (udp_packet != nullptr);

  for (const auto &kv : unhandled_message_count)
  {
    std::cerr << "unhandled message type: " << static_cast<uint32_t>(kv.first) << ", count: " << kv.second << '\n';
  }

  std::cout << count << " packets in total." << std::endl;
  return 0;
}
