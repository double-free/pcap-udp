#pragma once

// network
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <iostream>

// Debug
#include "../md/utils.h"

const u_char *extract_udp_packet(const pcap_pkthdr &header,
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

  auto *udp_header = packet + sizeof(ether_header) + sizeof(ip);

  return udp_header;
}
