#pragma once

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace md {
inline void print_hex_array(const u_char *buf, size_t len) {
  for (uint32_t i = 0; i < len; i++) {
    std::cout << std::setfill('0') << std::setw(2) << std::hex
              << (0xff & buf[i]) << " ";
  }
  std::cout << '\n';
  std::cout << std::resetiosflags(std::ios_base::basefield);
}

inline std::string bytes_to_str(const char bytes[], int len) {
  if (len == 0) {
    return std::string("");
  }

  // find the last char which is not a space
  int idx = len - 1;

  while (idx >= 0 && bytes[idx] == ' ') {
    --idx;
  }

  auto *buffer = new char[idx + 2];
  memcpy(buffer, bytes, idx + 1);
  buffer[idx + 1] = '\0';
  return std::string(buffer);
}

inline std::string timestamp_to_string(int64_t ts) {
  int millis = ts % 1000;
  ts /= 1000;
  int seconds = ts % 100;
  ts /= 100;
  int minutes = ts % 100;
  ts /= 100;
  int hours = ts % 100;

  std::stringstream ss;
  ss << std::setfill('0') << std::setw(2) << hours << ':' << std::setw(2)
     << minutes << ':' << std::setw(2) << seconds << '.' << std::setw(3)
     << millis;
  return ss.str();
}
} // namespace md
