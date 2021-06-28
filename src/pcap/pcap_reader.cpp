#include "pcap_reader.h"

int PcapReader::set_filter(const std::string &filter_str) {
  bpf_program filter;
  int result =
      pcap_compile(file_, &filter, filter_str.c_str(), 0 /*no optimize*/,
                   PCAP_NETMASK_UNKNOWN /*capture any interface*/);
  if (result != 0) {
    return result;
  }
  // apply filter
  return pcap_setfilter(file_, &filter);
}

int PcapReader::read_pcap_packet(const u_char *packet) {
  auto *ethernet_header = reinterpret_cast<const ether_header *>(packet);
  if (ntohs(ethernet_header->ether_type) != ETHERTYPE_IP) {
    // ignore non ip packet
    return 0;
  }

  auto *ip_header = reinterpret_cast<const ip *>(packet + sizeof(ether_header));
  if (ip_header->ip_p != IPPROTO_UDP) {
    // ignore non udp packet
    return 0;
  }

  udp_packet_index_ += 1;

  for (auto &processor : processors_) {
    if (processor.match(ip_header->ip_src)) {
      const u_char *udp_header = packet + sizeof(ether_header) + sizeof(ip);
      processor.process(*reinterpret_cast<const udphdr *>(udp_header),
                        udp_header + sizeof(udphdr));
      return 1;
    }
  }
  // does not match any processor
  return 0;
}

int PcapReader::process() {
  int processed_count = 0;
  for (u_char *next_packet = pcap_next(file_, &header_); next_packet != nullptr; next_packet = pcap_next(file_, &header_) {
    processed_count += read_pcap_packet(next_packet);
  }
  return processed_count;
}
