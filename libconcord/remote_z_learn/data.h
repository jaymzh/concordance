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
#include <chrono>
#include <string>

namespace remote_z
{

/** single data block */
class Block
{
   public:
    static Block fromMarkSegment(uint16_t mark_us, uint16_t segment_us)
    {
        return Block(mark_us, segment_us, segment_us - mark_us);
    }

    static Block fromMarkPause(uint16_t mark_us, uint16_t pause_us)
    {
        return Block(mark_us, pause_us + mark_us, pause_us);
    }

   protected:
    Block(uint16_t mark_us, uint16_t segment_us, uint16_t pause_us)
        : mark_us(mark_us), pause_us(pause_us), segment_us(segment_us)
    {
    }

   public:
    const uint16_t mark_us;
    const uint16_t pause_us;
    const uint16_t segment_us;

    std::chrono::microseconds mark() const
    {
        return std::chrono::microseconds(mark_us);
    }
    std::chrono::microseconds segment() const
    {
        return std::chrono::microseconds(segment_us);
    }
    std::chrono::microseconds pause() const
    {
        return std::chrono::microseconds(pause_us);
    }
};

/** entire stream (single frame or actual stream) of
 * timing data */
class TimingStream
{
   protected:
    std::vector<Block> data;

   public:
    TimingStream() {}

    /** Stream lesen, Codierung Mark, Mark+Pause (=Periodendauer).
     * Wir so in irlearn verwendet */
    static TimingStream fromMarkSegment(const std::vector<uint16_t> &raw)
    {
        TimingStream ts;
        ts.addMarkSegment(raw);
        return ts;
    }

    /** Stream lesen, Codierung Mark, Pause */
    static TimingStream fromMarkPause(const std::vector<uint16_t> &raw)
    {
        TimingStream ts;
        ts.addMarkPause(raw);
        return ts;
    }

    void addMarkSegment(const std::vector<uint16_t> &raw);
    void addMarkPause(const std::vector<uint16_t> &raw);

    std::vector<uint16_t> convertMarkPause() const;
    std::string convertGnuplot(bool activeHigh = true) const;
    std::string convertHexString() const;
    std::string convertIntString() const;
    std::string convertAsciiPlot(uint32_t width = 100,
                                 bool activeHigh = true) const;

    const std::vector<Block> &timings() const { return data; }
};

}  // namespace remote_z
