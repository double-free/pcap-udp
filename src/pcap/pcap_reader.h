#pragma once

#include "udp_packet_processor.h"

#include <cassert>
#include <pcap.h>
#include <stdexcept>
#include <vector>
/*
struct timeval
{
　　long tv_sec;  // seconds since epoch
　　long tv_usec; // micro seconds
}
*/

inline uint64_t get_pcap_timestamp(const pcap_pkthdr &header) {
  return header.ts.tv_sec * 1000000 + header.ts.tv_usec;
}

class PcapReader {
public:
  explicit PcapReader(std::string filename) {
    file_ = pcap_open_offline(filename.c_str(), errbuf_);
    if (file_ == nullptr) {
      throw std::invalid_argument("invalid pcap file: " + filename);
    }
  }

  ~PcapReader() { pcap_close(file_); }

  int set_filter(const std::string &filter_str);

  void add_processor(UdpPacketProcessor *processor) {
    processors_.push_back(processor);
  }

  // return number of parsed packet, can be 0 or 1
  int read_pcap_packet(const u_char *packet);

  // loop until file ends, return how many packets parsed
  int process();

  const pcap_pkthdr &pcap_header() const { return header_; }

  uint64_t udp_packet_index() const { return udp_packet_index_; }

private:
  pcap_t *file_;
  pcap_pkthdr header_;
  uint64_t udp_packet_index_;
  char errbuf_[PCAP_ERRBUF_SIZE];

  // one procesor for one md feed
  std::vector<UdpPacketProcessor *> processors_;
};
