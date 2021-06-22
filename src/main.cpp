#include "md/preprocessor.h"
#include "pcap/pcap_reader.h"
#include "pcap/pcap_to_udp.h"

#include "csv/writer.h"

#include <iostream>

int main(int argc, char const *argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << "<pcap file>" << '\n';
    return 1;
  }

  auto reader = PcapReader(argv[1]);
  // TODO: hard-coded address filter
  if (reader.set_filter("net 172.27.129 or net 127.27.1") != 0)
  {
    std::cerr << "set filter failed" << '\n';
    return 2;
  }
  std::string order_header = R"(clockAtArrival,sequenceNo,exchId,securityType,__isRepeated,TransactTime,ChannelNo,ApplSeqNum,SecurityID,secid,mdSource,)"
                             R"(Side,OrderType,__origTickSeq,Price,OrderQty)";
  std::string trade_header = R"(clockAtArrival,sequenceNo,exchId,securityType,__isRepeated,TransactTime,ChannelNo,ApplSeqNum,SecurityID,secid,mdSource,)"
                             R"(ExecType,TradeBSFlag,__origTickSeq,TradePrice,TradeQty,TradeMoney,BidApplSeqNum,OfferApplSeqNum)";

  csv::Writer order_writer("md_order.csv", order_header);
  csv::Writer trade_writer("md_trade.csv", trade_header);
  csv::Writer snapshot_writer("md_snapshot.csv", "");

  std::map<md::MessageType, int> unhandled_message_count;
  auto md_handler = [&order_writer, &trade_writer, &snapshot_writer, &unhandled_message_count](const u_char *md)
  {
    const auto *header = reinterpret_cast<const md::MdHeader *>(md);
    switch (header->message_type())
    {
    case md::MessageType::Order:
    {
      const auto *order = reinterpret_cast<const md::Order *>(md + sizeof(md::MdHeader));
      order_writer.write_order(*order, 123, 456);
      break;
    }
    case md::MessageType::Trade:
    {
      const auto *trade = reinterpret_cast<const md::Trade *>(md + sizeof(md::MdHeader));
      trade_writer.write_trade(*trade, 987654321, 123456789);
      break;
    }
    case md::MessageType::Snapshot:
    {
      const auto *snapshot = reinterpret_cast<const md::Snapshot *>(md + sizeof(md::MdHeader));
      snapshot_writer.write_snapshot(*snapshot);
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
  };

  MdPreprocessor processor(md_handler);

  // processed message count
  int count = 0;
  const u_char *udp_payload = nullptr;
  do
  {
    udp_payload = reader.extract_from_pcap_packet(extract_udp_payload);
    if (udp_payload != nullptr)
    {
      count += processor.process(udp_payload);
    }
  } while (udp_payload != nullptr);

  for (const auto &kv : unhandled_message_count)
  {
    std::cerr << "unhandled message type: " << static_cast<uint32_t>(kv.first) << ", count: " << kv.second << '\n';
  }

  std::cout << count << " packets in total." << std::endl;
  return 0;
}
