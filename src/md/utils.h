#pragma once

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>

namespace md
{
    inline void print_hex_array(const u_char *buf, size_t len)
    {
        for (uint32_t i = 0; i < len; i++)
        {
            std::cout << std::setfill('0') << std::setw(2) << std::hex << (0xff & buf[i]) << " ";
        }
        std::cout << '\n';
        std::cout << std::resetiosflags(std::ios_base::basefield);
    }

    inline std::string bytes_to_str(const char bytes[], int len)
    {
        if (len == 0)
        {
            return std::string("");
        }

        // find the last char which is not a space
        int idx = len - 1;

        while (idx >= 0 && bytes[idx] == ' ')
        {
            --idx;
        }

        auto *buffer = new char[idx + 2];
        memcpy(buffer, bytes, idx + 1);
        buffer[idx + 1] = '\0';
        return std::string(buffer);
    }
}
