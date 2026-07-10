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

#include "protocol_z.h"

namespace remote_z
{

class Base
{
   public:
    enum class Status {
        OK,
        DONE,
        ERR_SIZE,
        ERR_UNKNOWN_RETURNCODE,  //received unknown return code from remote, see ProtocolStatus
        ERR_RESPONSE_FORMAT,  //response heder, layout format/values not as expected
        ERR_PAYLOAD_FORMAT,  //dito, payload
        ERR_TERM,
        ERR_TIMEOUT,
        ERR_UNKNOWN
    };

   protected:
    std::vector<uint16_t> payload;
    double clock = 0;
    std::vector<uint16_t> excess;

    //static values from child classes
    virtual uint32_t getHeaderMinSize() = 0;
    virtual uint32_t getProtoclCmd() = 0;
    virtual uint32_t getProtocolRespByte3() = 0;

    //assume status is byte data[2] in response
    enum class ProtocolStatus { OK = 0x01, TIMEOUT = 0x02 };

    void moveExcessBytes(bool check);

   public:
    /** generate request */
    virtual std::vector<uint8_t> get() = 0;

    /** check data block */
    virtual Status check(const std::vector<uint8_t> &data);

    /** get uninterpretet error byte */
    uint8_t getErrorByte(const std::vector<uint8_t> &data);

    /**
     * append data block
     *
     * @param data response data block
     * @param first this is the first ever block in this transfer (command single/stream doesn't matter)
     * @return see 'Status'
     */
    virtual Status addChunk(const std::vector<uint8_t> &data, bool first)
    {
        return Status::OK;
    };

    /** access payload. size() == 0 -> nothing available */
    virtual const std::vector<uint16_t> &getPayload() { return payload; }

    /** get IR clock */
    virtual const double getClock() { return clock; }

    /** read non-timing related words. word = unknown use */
    virtual const std::vector<uint16_t> &getExcess() { return excess; }
};

}  // namespace remote_z
