#pragma once

#include <cassert>
#include <functional>
#include <pcap.h>
#include <stdexcept>

class PcapReader
{
public:
  using PacketHandler =
      std::function<const u_char *(const pcap_pkthdr &, const unsigned char *)>;

  explicit PcapReader(std::string filename)
  {
    file_ = pcap_open_offline(filename.c_str(), errbuf_);
    if (file_ == nullptr)
    {
      throw std::invalid_argument("invalid pcap file: " + filename);
    }
  }

  int set_filter(const std::string &filter_str)
  {
    bpf_program filter;
    int result =
        pcap_compile(file_, &filter, filter_str.c_str(), 0 /*no optimize*/,
                     PCAP_NETMASK_UNKNOWN /*capture any interface*/);
    if (result != 0)
    {
      return result;
    }
    // apply filter
    return pcap_setfilter(file_, &filter);
  }

  ~PcapReader() { pcap_close(file_); }

  // try to extract some data from pcap packet
  // return null if no pcap packet remaining
  const u_char *extract_from_pcap_packet(PacketHandler extractor)
  {
    const u_char *data = nullptr;
    while (data == nullptr)
    {
      const u_char *next_packet = pcap_next(file_, &header_);
      if (next_packet == nullptr)
      {
        // no more pcap packet
        return nullptr;
      }
      data = extractor(header_, next_packet);
    }
    assert(data);
    return data;
  }

private:
  pcap_t *file_;
  pcap_pkthdr header_;
  char errbuf_[PCAP_ERRBUF_SIZE];
};
