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

#include "stream.h"
#include "binary.h"

using namespace std;

namespace remote_z
{

Stream::Status Stream::parse(const std::vector<uint8_t> &p)
{
    uint16_t v;

    if ((p.size() % 2) != 0) return Status::ERR_SIZE;

    for (size_t i = 0; i < p.size(); i += 2) {
        uint16_t a = p[i + 1];
        uint16_t b = p[i];
        v = a | (b << 8);
        payload.push_back(v);
    }
    return Status::OK;
}

Stream::Status Stream::addChunk(const std::vector<uint8_t> &data, bool first)
{
    auto status = Base::check(data);
    if (status != Status::OK) return status;

    //data available, check remaining header + size
    if ((data.size() != VALID_CHUNK_SIZE) || (data[4] != 0x70))
        return Status::ERR_RESPONSE_FORMAT;
    //terminator
    if ((data[data.size() - 2] != 0x01) || (data[data.size() - 1] != 0x30))
        return Status::ERR_TERM;

    auto ret = remote_z::parseHarmony16_network(
        {data.begin() + 5, data.end() - 2}, payload);
    if (ret != true) return Status::ERR_SIZE;

    //move non-timing words
    moveExcessBytes(first);
    //calculate clock
    if (first) clock = static_cast<double>(excess[1]) * 1000000.0 / payload[0];

    return Status::OK;
}

}  // namespace remote_z
