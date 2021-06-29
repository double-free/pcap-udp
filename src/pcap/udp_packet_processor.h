#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <stdexcept>
/*
struct in_addr {
    unsigned long s_addr;  // load with inet_aton()
};
*/

class UdpPacketProcessor {
public:
  explicit UdpPacketProcessor(std::string net_str, std::string netmask_str) {
    if (inet_aton(net_str.c_str(), &net_) == 0) {
      throw std::invalid_argument("invalid net: " + net_str);
    }

    if (inet_aton(netmask_str.c_str(), &netmask_) == 0) {
      throw std::invalid_argument("invalid netmask: " + netmask_str);
    }
  }

  virtual ~UdpPacketProcessor() {}

  bool match(const in_addr &ip) const {
    return (netmask_.s_addr & ip.s_addr) == (netmask_.s_addr & net_.s_addr);
  }

  virtual void process(const udphdr &udp_header, const u_char *udp_payload) {}

private:
  in_addr net_;
  in_addr netmask_;
};
