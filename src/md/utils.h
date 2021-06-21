#pragma once

#include <iostream>
#include <iomanip>

void print_hex_array(const u_char *buf, size_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        std::cout << std::setfill('0') << std::setw(2) << std::hex << (0xff & buf[i]) << " ";
    }
    std::cout << '\n';
    std::cout << std::resetiosflags(std::ios_base::basefield);
}