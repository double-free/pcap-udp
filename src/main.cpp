#include "md/preprocessor.h"
#include "pcap/pcap_reader.h"
#include "pcap/pcap_to_udp.h"
#include "md/utils.h"

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

  csv::Writer order_writer("md_order.csv");
  csv::Writer trade_writer("md_trade.csv");
  csv::Writer snapshot_writer("md_snapshot.csv");
  auto md_handler = [&order_writer, &trade_writer, &snapshot_writer](const u_char *md) -> bool
  {
    const auto *header = reinterpret_cast<const md::MdHeader *>(md);
    switch (header->message_type())
    {
    case md::MessageType::Order:
    {
      const auto *order = reinterpret_cast<const md::Order *>(md + sizeof(md::MdHeader));
      order_writer.write_order(*order);
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
    default:
      return false;
    }
    return true;
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

  std::cout << count << " packets in total." << std::endl;
  return 0;
}
