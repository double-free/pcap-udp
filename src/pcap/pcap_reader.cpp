#include "pcap_reader.h"

// network
#include <iostream>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

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
    if (processor->match(ip_header->ip_src)) {
      const u_char *udp_header = packet + sizeof(ether_header) + sizeof(ip);
      processor->process(*reinterpret_cast<const udphdr *>(udp_header),
                         udp_header + sizeof(udphdr));
      return 1;
    }
  }
  // does not match any processor
  std::cout << "udp packet " << udp_packet_index_ << " from "
            << inet_ntoa(ip_header->ip_src)
            << " does not match any of the processor\n";
  return 0;
}

uint64_t PcapReader::process(long stop_epoch_seconds) {
  uint64_t processed_count = 0;
  for (const u_char *next_packet = pcap_next(file_, &header_);
       next_packet != nullptr; next_packet = pcap_next(file_, &header_)) {
    if (header_.ts.tv_sec > stop_epoch_seconds) {
      break;
    }
    processed_count += read_pcap_packet(next_packet);
  }
  return processed_count;
}
