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
#include "base.h"

using namespace std;

namespace remote_z
{

Base::Status Base::check(const std::vector<uint8_t> &data)
{
    if (data.size() < getHeaderMinSize()) return Status::ERR_SIZE;
    if ((data[0] != 0x20) || (data[1] != getProtoclCmd()) ||
        (data[3] != getProtocolRespByte3())) {
        return Status::ERR_RESPONSE_FORMAT;
    }
    switch (static_cast<ProtocolStatus>(data[2])) {
        case ProtocolStatus::OK:
            return Status::OK;
        case ProtocolStatus::TIMEOUT:
            return Status::ERR_TIMEOUT;
        default:
            return Status::ERR_UNKNOWN_RETURNCODE;
    }
}

uint8_t Base::getErrorByte(const std::vector<uint8_t> &data)
{
    if (data.size() < getHeaderMinSize()) return 255;
    return data[2];
}

void Base::moveExcessBytes(bool check)
{
    if (payload.size() < 3) return;

    if (check) {
        excess.push_back(payload[0]);
        excess.push_back(payload[2]);
        payload.erase(payload.begin() + 2);
        payload.erase(payload.begin() + 0);
    }
}

}  // namespace remote_z
