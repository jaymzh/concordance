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

#include <sstream>
#include <iomanip>
#include <algorithm>

#include "data.h"

using namespace std;

namespace remote_z
{

void TimingStream::addMarkSegment(const vector<uint16_t> &raw)
{
    for (size_t i = 0; i < raw.size(); i += 2)
        if (i + 1 < raw.size())
            data.push_back(Block::fromMarkSegment(raw[i], raw[i + 1]));
}

void TimingStream::addMarkPause(const vector<uint16_t> &raw)
{
    for (size_t i = 0; i < raw.size(); i += 2)
        if (i + 1 < raw.size())
            data.push_back(Block::fromMarkPause(raw[i], raw[i + 1]));
}

vector<uint16_t> TimingStream::convertMarkPause() const
{
    vector<uint16_t> stream;

    for (const auto &item : data) {
        stream.push_back(item.mark_us);
        stream.push_back(item.pause_us);
    }
    return stream;
}

string TimingStream::convertGnuplot(bool activeHigh) const
{
    vector<pair<uint32_t, int>> plotData;
    uint32_t time_us = 0;
    stringstream str;

    uint32_t markValue = 1;
    if (!activeHigh) markValue = 0;

    for (const auto &block : data) {
        // Start of mark (ON state)
        plotData.push_back({time_us, markValue});

        // End of mark, start of segment (OFF state)
        time_us += block.mark_us;
        plotData.push_back({time_us, 0});

        // End of segment (prepare for next mark)
        time_us += block.pause_us;
    }

    str << "time(µs) mark" << endl;
    for (const auto &[time, amp] : plotData) str << time << " " << amp << endl;
    return str.str();
}

string TimingStream::convertHexString() const
{
    stringstream str;

    for (const auto &block : data) {
        str << hex << setw(4) << setfill('0') << block.mark_us << " " << hex
            << setw(4) << setfill('0') << block.segment_us
            << " ";  //raw data, not mark/pause!
    }
    str << dec;

    return str.str();
}

string TimingStream::convertIntString() const
{
    stringstream str;

    for (const auto &block : data) {
        str << "MP" << block.mark_us << ":" << block.pause_us << "; ";
        //str << "MS" << block.mark_us << ":" << block.segment_us << "; ";
    }

    return str.str();
}

string TimingStream::convertAsciiPlot(uint32_t width, bool activeHigh) const
{
    //todo active high...
    string header;
    string top;
    string bottom;
    uint32_t used = 0;
    uint32_t base = 250;  //us per char //todo so am einfachsten.
    bool level = false;
    bool truncated = false;
    auto append = [&](const string &t, const string &b, uint32_t count = 1) {
        if (used < width) {
            top += t;
            bottom += b;
            used += count;
        }
    };
    string truncationMarker = "...";
    width = width - 3;

    if (data.empty() || (width < 25)) return "";

    header = "µs per div: " + to_string(base);

    for (const auto &block : data) {
        // divs, do round. for shorter pulse min 1 div, even on round-down
        int mark_divs = max(1u, (block.mark_us + base / 2) / base);
        if (block.mark_us == 0) {
            //silence block, no pulse
            mark_divs = 0;
        }
        int pause_divs = max(1u, (block.pause_us + base / 2) / base);

        // rising edge
        if (!level && (mark_divs > 0)) {
            append("┌", "┘");
            level = true;
            if (used >= width) {
                truncated = true;
                break;
            }
        }

        for (int i = 1; i < mark_divs; i++) {
            append("─", " ");
            if (used >= width) {
                truncated = true;
                break;
            }
        }
        if (used >= width) {
            truncated = true;
            break;
        }

        // falling edge
        if (level && (mark_divs > 0)) {
            append("┐", "└");
            level = false;
            if (used >= width) {
                truncated = true;
                break;
            }
        }

        for (int i = 1; i < pause_divs; i++) {
            append(" ", "─");
            if ((i > 4) && (pause_divs > 12)) {
                //crop empty data
                append("     ", "...──", 5);
                break;
            }
            if (used >= width) {
                truncated = true;
                break;
            }
        }
        if (used >= width) {
            truncated = true;
            break;
        }
    }

    if (truncated) bottom += "\\++";

    return header + '\n' + top + '\n' + bottom + '\n';
}

}  // namespace remote_z
