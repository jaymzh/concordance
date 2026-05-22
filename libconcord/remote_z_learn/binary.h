/*
 * vim:tw=80:ai:tabstop=4:softtabstop=4:shiftwidth=4:expandtab
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * (C) Copyright Martin Wagner 2026
 */

#pragma once

#include <cstdint>
#include <vector>

namespace remote_z
{

//use on network data stream: parseHarmony16(p[i], p[i+1])
inline uint16_t parseHarmony16_network(uint8_t h, uint8_t l)
{
    return static_cast<uint16_t>(l) | (static_cast<uint16_t>(h) << 8);
}

inline bool parseHarmony16_network(const std::vector<uint8_t> &in,
                                   std::vector<uint16_t> &out)
{
    if ((in.size() % 2) != 0) return false;

    for (size_t i = 0; i < in.size(); i += 2)
        out.push_back(parseHarmony16_network(in[i], in[i + 1]));
    return true;
}

//use on network data stream: parseHarmony16(p[i], p[i+1])
inline uint16_t parseHarmony16_file(uint8_t h, uint8_t l)
{
    return static_cast<uint16_t>(h) | (static_cast<uint16_t>(l) << 8);
}

inline uint32_t parseHarmony32_file(uint8_t h,
                                    uint8_t m1,
                                    uint8_t m2,
                                    uint8_t l)
{
    return static_cast<uint32_t>(h) | (static_cast<uint16_t>(m1) << 8) |
           (static_cast<uint16_t>(m2) << 16) | (static_cast<uint16_t>(l) << 24);
}

inline bool parseHarmony16_file(const std::vector<uint8_t> &in,
                                std::vector<uint16_t> &out)
{
    if ((in.size() % 2) != 0) return false;

    for (size_t i = 0; i < in.size(); i += 2)
        out.push_back(parseHarmony16_file(in[i], in[i + 1]));
    return true;
}

inline void setHarmony16_file(uint16_t data, std::vector<uint8_t> &out)
{
    out.push_back(data);
    out.push_back(data >> 8);
}

inline void setHarmony32_file(uint32_t data, std::vector<uint8_t> &out)
{
    out.push_back(data);
    out.push_back(data >> 8);
    out.push_back(data >> 16);
    out.push_back(data >> 24);
}

inline void setHarmony16_file(const std::vector<uint16_t> &in,
                              std::vector<uint8_t> &out)
{
    for (const auto &d : in) setHarmony16_file(d, out);
}

}  // namespace remote_z
