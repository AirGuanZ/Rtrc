#pragma once

// Source: https://github.com/tobiaslocker/base64

/*
MIT License

Copyright (c) 2019 Tobias Locker

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <algorithm>
#include <span>
#include <string>

namespace base64 {

inline std::string get_base64_chars() {
    static std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";
    return base64_chars;
}

inline std::string to_base64(std::span<const unsigned char> data) {
    int counter = 0;
    uint32_t bit_stream = 0;
    const std::string base64_chars = get_base64_chars();
    std::string encoded;
    int offset = 0;
    for(unsigned char c : data) {
        auto num_val = static_cast<unsigned int>(c);
        offset = 16 - counter % 3 * 8;
        bit_stream += num_val << offset;
        if(offset == 16) {
            encoded += base64_chars.at(bit_stream >> 18 & 0x3f);
        }
        if(offset == 8) {
            encoded += base64_chars.at(bit_stream >> 12 & 0x3f);
        }
        if(offset == 0 && counter != 3) {
            encoded += base64_chars.at(bit_stream >> 6 & 0x3f);
            encoded += base64_chars.at(bit_stream & 0x3f);
            bit_stream = 0;
        }
        counter++;
    }
    if(offset == 16) {
        encoded += base64_chars.at(bit_stream >> 12 & 0x3f);
        encoded += "==";
    }
    if(offset == 8) {
        encoded += base64_chars.at(bit_stream >> 6 & 0x3f);
        encoded += '=';
    }
    return encoded;
}

inline std::string to_base64(std::string const &data) {
    return to_base64(std::span(
            reinterpret_cast<const unsigned char*>(data.data()), data.size()));
}

inline std::string from_base64(std::string const &data) {
  int counter = 0;
  uint32_t bit_stream = 0;
  std::string decoded;
  int offset = 0;
  const std::string base64_chars = get_base64_chars();
  for (unsigned char c : data) {
    auto num_val = base64_chars.find(c);
    if (num_val != std::string::npos) {
      offset = 18 - counter % 4 * 6;
      bit_stream += num_val << offset;
      if (offset == 12) {
        decoded += static_cast<char>(bit_stream >> 16 & 0xff);
      }
      if (offset == 6) {
        decoded += static_cast<char>(bit_stream >> 8 & 0xff);
      }
      if (offset == 0 && counter != 4) {
        decoded += static_cast<char>(bit_stream & 0xff);
        bit_stream = 0;
      }
    } else if (c != '=') {
      return std::string();
    }
    counter++;
  }
  return decoded;
}

}

#include <Rtrc/Core/Container/Span.h>

RTRC_BEGIN

inline std::string ToBase64(Span<unsigned char> data)
{
    return base64::to_base64(data);
}

inline std::string ToBase64(const std::string &data)
{
    return base64::to_base64(data);
}

inline std::string FromBase64(const std::string &encoded)
{
    return base64::from_base64(encoded);
}

RTRC_END
