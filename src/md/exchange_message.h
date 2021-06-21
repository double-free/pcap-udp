#pragma once

#include <zlib.h>
#include <endian.h>
#include <iostream>
#include <cassert>

struct __attribute__((packed)) MessageHeader
{
  uint32_t unknown_number; // 0x00640000
  uint16_t compressed;
  uint32_t be_size_before_compress;
  uint32_t be_size_after_compress;
  uint32_t be_length3; // same as be_size_after_compress
};

void parse_market_data(const u_char *md)
{
}

void parse_message(const u_char *message)
{
  if (message == nullptr)
  {
    return;
  }
  const auto *header = reinterpret_cast<const MessageHeader *>(message);

  if (header->compressed == 0x0000)
  {
    // not compressed
    parse_market_data(message);
    return;
  }

  // uncompress
  size_t decompressed_size = be32toh(header->be_size_before_compress);

  // TODO: avoid allocating in critical path
  u_char *decompressed = new u_char[decompressed_size];

  int result = uncompress(decompressed, &decompressed_size, message + sizeof(MessageHeader), be32toh(header->be_size_after_compress));
  assert(decompressed_size == be32toh(header->be_size_before_compress));

  if (result != Z_OK)
  {
    std::cerr << "uncompress failed with code " << result << ", skip this message" << '\n';
    return;
  }

  parse_market_data(decompressed);

  delete decompressed;
}