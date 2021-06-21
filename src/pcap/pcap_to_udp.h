#pragma once

// network
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <iostream>

const u_char *extract_udp_payload(const pcap_pkthdr &header,
                                  const u_char *packet)
{
  auto *ethernet_header = reinterpret_cast<const ether_header *>(packet);
  if (ntohs(ethernet_header->ether_type) != ETHERTYPE_IP)
  {
    // ignore non ip packet
    return nullptr;
  }

  auto *ip_header = reinterpret_cast<const ip *>(packet + sizeof(ether_header));
  if (ip_header->ip_p != IPPROTO_UDP)
  {
    // ignore non udp packet
    return nullptr;
  }

  auto *udp_header = reinterpret_cast<const udphdr *>(
      packet + sizeof(ether_header) + sizeof(ip));

  std::cout << "source port: " << ntohs(udp_header->source)
            << ", dest port: " << ntohs(udp_header->dest)
            << ", length: " << ntohs(udp_header->len) << std::endl;

  auto *udp_payload =
      reinterpret_cast<const u_char *>(udp_header) + sizeof(udphdr);
  return udp_payload;
}
