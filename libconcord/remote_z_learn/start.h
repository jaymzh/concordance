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

#include "base.h"

namespace remote_z
{

class Start : public Base
{
   private:
    const std::vector<uint8_t> frameReq = {0x01, 0x01, 0x00};

    uint32_t getHeaderMinSize() { return 4; };
    uint32_t getProtoclCmd() { return COMMAND_LEARNIR_START; };
    uint32_t getProtocolRespByte3() { return 0; };

   public:
    std::vector<uint8_t> get() { return frameReq; }
};

}  // namespace remote_z
