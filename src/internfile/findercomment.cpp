/* Copyright (C) 2024 J.F.Dockes
 *
 * License: GPL 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// Convert a 4 bytes char array representing an MSB first int into an int.
static inline uint32_t char4toint(const unsigned char *f)
{
    return (
               ((uint32_t)(f[3])) +
               ((uint32_t)(f[2]) << 8) +
               ((uint32_t)(f[1]) << 16) +
               ((uint32_t)(f[0]) << 24));
}

// Decode a MacOS plist in the very specific case of the data found in a
// com.apple.metadata:kMDItemFinderComment extended attribute value. This contains a single string,
// either bytes or unicode (not sure about the encoding, probably ucs-2?).
std::pair<std::string, std::string> decode_comment_plist(const unsigned char *cp0, int sz)
{
    if (memcmp(cp0, "bplist00", 8) != 0) {
        return {"", "not bplist00"};
    }
    const unsigned char *cp = cp0+8;
    // Next byte high 4 bits is 5 for a bytes (ASCII) string or 6 for Unicode
    bool unicode;
    auto format = ((*cp) & 0xf0) >> 4;
    auto numchars = (*cp) & 0x0f;
    if (format == 5) {
        unicode = false;
    } else if (format == 6) {
        unicode = true;
    } else {
        return {"", "format " + std::to_string(format) + " neither bytes nor unicode."};
    }
    // std::cerr << "format is " << format << " numchars " << numchars << "\n";
    if (numchars == 15) {
        // Then the actual length is in the following bytes.
        cp++;
        // std::cerr << "*cp now " << std::hex << (unsigned int)*cp << std::dec << "\n";
        if (((*cp) & 0xf0)>>4 != 1) {
            return {"", "Int len high nibble not 1"};
        }
        auto pow = (*cp) & 0x0f;
        auto ilen = 1 << pow;
        // std::cerr << "ilen is " << ilen << "\n";
        // The next ilen bytes are a big endian integer.
        if (ilen > 4) {
            return {"", "Bad integer size " + std::to_string(ilen)};
        }
        cp++;
        unsigned char ar[4] = {0,0,0,0};
        // Fill the array down from lsb (byte 3) down to msb depending on ilen
        for (int i = ilen-1; i >= 0; i--) {
            ar[i + (4-ilen)] = cp[i];
        }
        numchars = char4toint(ar);
        cp += ilen;
    } else {
        cp++;
    }
    auto bytelen = unicode ? numchars * 2 : numchars;
    // std::cerr << "numchars " << numchars << " numbytes " << bytelen << "\n";
    return {std::string((const char *)cp, bytelen), ""};
}
