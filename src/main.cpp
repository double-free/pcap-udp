#include "md/message.h"
#include "pcap/pcap_reader.h"
#include "pcap/pcap_to_udp.h"

#include <iostream>

int main(int argc, char const *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << "<pcap file>" << '\n';
    return 1;
  }

  auto reader = PcapReader(argv[1]);
  // hard-coded address
  if (reader.set_filter("net 172.27.129 or net 127.27.1") != 0) {
    std::cerr << "set filter failed" << '\n';
    return 2;
  }

  MessageParser parser;

  // processed message count
  int count = 0;
  const u_char *udp_payload = nullptr;
  do {
    udp_payload = reader.extract_from_pcap_packet(extract_udp_payload);
    if (udp_payload != nullptr) {
      count += parser.parse_data(udp_payload);
    }
  } while (udp_payload != nullptr);

  std::cout << count << " packets in total." << std::endl;
  return 0;
}
