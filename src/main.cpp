#include "md/preprocessor.h"
#include "pcap/pcap_reader.h"
#include "pcap/pcap_to_udp.h"
#include "md/md_parser.h"
#include "md/utils.h"

#include <iostream>

void md_handler(const u_char *md)
{
  const auto *header = reinterpret_cast<const md::MdHeader *>(md);

  if (header->message_type() == md::MessageType::Trade)
  {
    std::cout << "find a trade message with length " << header->body_size() << ": ";
    print_hex_array(md, header->body_size() + sizeof(md::MdHeader));
  }
}

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
