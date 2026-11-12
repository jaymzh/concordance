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
 */

/*
 * ssir_dump - parse and decode the SsIr.bin IR database found in the
 * userconfig region of a zwave usbnet Harmony remote (1000/1100/...).
 *
 * The on-disk format and the pulse encoding are documented in
 * specs/ir_database.txt. This tool walks the tail index to every
 * device/sequence, locates the Press signal, and decodes it. Protocol is
 * detected from the pulse timing; NEC/extended-NEC, RC6 mode 6A (MCE), a
 * pulse-distance AC frame, and a fixed-mark/quantized-gap PVR scheme are
 * recognised. Unknown or learned-raw signals are reported as such.
 *
 * Build:  make      (or: cc -O2 -Wall -o ssir_dump ssir_dump.c)
 * Usage:  ssir_dump SsIr.bin
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static uint8_t *buf;
static long size;

static uint16_t rd16(long o)
{
    return (uint16_t)(buf[o] | (buf[o + 1] << 8));
}

static uint32_t rd24(long o)
{
    return (uint32_t)(buf[o] | (buf[o + 1] << 8) | (buf[o + 2] << 16));
}

/*
 * From a Press/Hold offset, find where the pulse stream begins: scan for the
 * lead-in pre-gap word (0x5097 or 0x4351), skip any run of it, and return the
 * offset of the first real pulse word. Returns -1 if not found.
 */
static long find_pulses(long off)
{
    long s, p;
    for (s = off; s + 1 < off + 500 && s + 1 < size; s++) {
        uint16_t w = rd16(s);
        if (w == 0x5097 || w == 0x4351)
            break;
    }
    if (s + 1 >= off + 500)
        return -1;
    p = s;
    while (p + 1 < size) {
        uint16_t w = rd16(p);
        if (w == 0x5097 || w == 0x4351)
            p += 2;
        else
            break;
    }
    return p;
}

#define MARK(w)  ((w) & 0x8000)
#define DUR(w)   ((w) & 0x7FFF)

/* Decode a standard NEC / extended-NEC frame starting at the AGC burst. */
static void decode_nec(long p, char *out, size_t n)
{
    int k;
    uint32_t val = 0;
    uint16_t m = rd16(p), s = rd16(p + 2);

    /* require an AGC: long mark (~4500 or ~9000) + ~4500 space */
    if (!MARK(m) || DUR(m) < 3500 || DUR(m) > 10000 ||
        MARK(s) || DUR(s) < 3500 || DUR(s) > 5200) {
        snprintf(out, n, "no NEC AGC");
        return;
    }
    p += 4;
    for (k = 0; k < 32 && p + 3 < size; k++, p += 4) {
        uint16_t mk = rd16(p), sp = rd16(p + 2);
        if (!MARK(mk))
            break;
        if (DUR(sp) > 1000)
            val |= (uint32_t)1 << k;
    }
    {
        uint8_t a0 = val & 0xFF, a1 = (val >> 8) & 0xFF;
        uint8_t c = (val >> 16) & 0xFF, ci = (val >> 24) & 0xFF;
        int ext = (ci != (uint8_t)~c);
        snprintf(out, n, "NEC%s addr=0x%02X%02X cmd=0x%02X",
                 ext ? "x" : "", a1, a0, c);
    }
}

/*
 * Decode RC6 mode 6A. Expand each pulse into round(dur/t) half-bit samples
 * and Manchester-decode; the toggle bit (index 4) is double width.
 */
static void decode_rc6(long p, char *out, size_t n)
{
    char t[600];
    int nh = 0, i, lead, nb = 0, t_unit = 444;
    char bits[48];
    long q = p;

    for (i = 0; i < 160 && q + 1 < size; i++, q += 2) {
        uint16_t w = rd16(q);
        int u, j;
        if (DUR(w) > 8000)
            break;
        u = (DUR(w) + t_unit / 2) / t_unit;
        if (u < 1)
            u = 1;
        for (j = 0; j < u && nh < (int)sizeof(t); j++)
            t[nh++] = MARK(w) ? 1 : 0;
    }
    i = 0;
    while (i < nh && t[i] == 1)
        i++;
    lead = i;
    if (lead < 4) {
        snprintf(out, n, "no RC6 leader");
        return;
    }
    i = lead + 2;
    while (i + 1 < nh && nb < 40) {
        int w = (nb == 4) ? 2 : 1;
        int h1, h2, v;
        if (i + 2 * w > nh)
            break;
        h1 = t[i];
        h2 = t[i + w];
        if (h1 == 1 && h2 == 0)
            v = 1;
        else if (h1 == 0 && h2 == 1)
            v = 0;
        else
            v = h1;
        bits[nb++] = v ? '1' : '0';
        i += 2 * w;
    }
    if (nb >= 29) {
        unsigned long payload = 0;
        int mode = (bits[1] - '0') * 4 + (bits[2] - '0') * 2 + (bits[3] - '0');
        int k, pn = nb - 5;
        for (k = 0; k < pn; k++)
            payload = (payload << 1) | (bits[5 + k] - '0');
        snprintf(out, n, "RC6 mode=%d customer=0x%04lX cmd=0x%02lX",
                 mode, (payload >> (pn - 16)) & 0xFFFF, payload & 0xFF);
    } else {
        snprintf(out, n, "RC6? (%d bits)", nb);
    }
}

/* Decode a pulse-distance frame (AC-style): space short=0, long=1. */
static void decode_pd(long p, char *out, size_t n)
{
    uint8_t by[32];
    int nbit = 0, i, nby, thresh;
    int lo = 0x7FFF, hi = 0;
    uint16_t m = rd16(p), s = rd16(p + 2);

    if (!MARK(m) || MARK(s) || DUR(s) < 900) {
        snprintf(out, n, "no PD AGC");
        return;
    }
    p += 4;
    /* first pass: find the short/long space extremes to pick a threshold */
    for (i = 0; i < 256; i++) {
        uint16_t mk = rd16(p + i * 4), sp = rd16(p + i * 4 + 2);
        if (p + i * 4 + 3 >= size || !MARK(mk) || DUR(sp) > 8000)
            break;
        if (DUR(sp) < lo)
            lo = DUR(sp);
        if (DUR(sp) > hi)
            hi = DUR(sp);
    }
    thresh = (hi > lo) ? (lo + hi) / 2 : 800;
    for (i = 0; i < 256 && p + 3 < size; i++, p += 4) {
        uint16_t mk = rd16(p), sp = rd16(p + 2);
        int b;
        if (!MARK(mk) || DUR(sp) > 8000)
            break;
        b = (DUR(sp) > thresh) ? 1 : 0;
        if (b)
            by[nbit / 8] |= 1 << (nbit & 7);
        else if ((nbit & 7) == 0)
            by[nbit / 8] = 0;
        nbit++;
        if (nbit >= 256)
            break;
    }
    nby = nbit / 8;
    if (nby >= 2) {
        int k, sum = 0;
        char *o = out;
        size_t left = n;
        int w = snprintf(o, left, "PD %d-bit:", nbit);
        o += w;
        left -= w;
        for (k = 0; k < nby && k < 14 && left > 3; k++) {
            w = snprintf(o, left, " %02X", by[k]);
            o += w;
            left -= w;
            if (k < nby - 1)
                sum = (sum + by[k]) & 0xFF;
        }
        if (nby >= 2 && left > 10)
            snprintf(o, left, " (chk %s)",
                     (sum == by[nby - 1]) ? "ok" : "bad");
    } else {
        snprintf(out, n, "PD? (%d bits)", nbit);
    }
}

/*
 * Fixed-mark / quantized-gap scheme (observed on a PVR): constant ~210us
 * mark, gap quantized in ~136us steps giving a 4-bit symbol per mark.
 */
static void decode_fixedmark(long p, char *out, size_t n)
{
    char *o = out;
    size_t left = n;
    int i, w;

    w = snprintf(o, left, "sym:");
    o += w;
    left -= w;
    for (i = 0; i < 60 && p + 1 < size && left > 2; i++, p += 2) {
        uint16_t word = rd16(p);
        int d = DUR(word);
        if (word == 0x7FFF || d > 15000)
            break;
        if (MARK(word))
            continue;        /* constant mark; data is in the gap */
        if (d < 650) {
            w = snprintf(o, left, "L");
        } else {
            int k = (d - 752 + 68) / 136;
            if (k < 0 || k > 15)
                w = snprintf(o, left, "?");
            else
                w = snprintf(o, left, "%X", k);
        }
        o += w;
        left -= w;
    }
}

/* Classify by the first pulses and dispatch to the right decoder. */
static void decode_signal(long off, char *out, size_t n)
{
    long p = find_pulses(off);
    uint16_t m0, s0;
    int const_mark, i;

    if (p < 0) {
        snprintf(out, n, "no signal");
        return;
    }
    m0 = rd16(p);
    s0 = rd16(p + 2);

    /* constant ~210us mark across the first few marks => fixed-mark scheme */
    const_mark = 1;
    for (i = 0; i < 20 && p + i * 2 + 1 < size; i++) {
        uint16_t w = rd16(p + i * 2);
        if (MARK(w) && (DUR(w) < 150 || DUR(w) > 400) && DUR(w) < 2000) {
            const_mark = 0;
            break;
        }
    }
    if (const_mark && MARK(m0) == 0) {
        /* lead-in then a short mark; peek the first mark */
        uint16_t fm = rd16(p);
        if (MARK(fm) && DUR(fm) < 400) {
            decode_fixedmark(p, out, n);
            return;
        }
    }
    if (!MARK(m0)) {
        snprintf(out, n, "unaligned/raw");
        return;
    }
    if (DUR(m0) >= 3500 && DUR(m0) <= 10000 &&
        DUR(s0) >= 3500 && DUR(s0) <= 5200) {
        decode_nec(p, out, n);
    } else if (DUR(m0) >= 2000 && DUR(m0) <= 5000 &&
               DUR(s0) >= 1000 && DUR(s0) <= 2500) {
        decode_pd(p, out, n);
    } else if (DUR(m0) >= 2000 && DUR(m0) <= 3200 &&
               DUR(s0) >= 600 && DUR(s0) <= 1200) {
        decode_rc6(p, out, n);
    } else if (DUR(m0) < 400) {
        decode_fixedmark(p, out, n);
    } else {
        snprintf(out, n, "raw/learned (mark=%u space=%u)", DUR(m0), DUR(s0));
    }
}

int main(int argc, char **argv)
{
    FILE *f;
    uint32_t idx;
    int ndev, d;

    if (argc < 2) {
        fprintf(stderr, "usage: %s SsIr.bin\n", argv[0]);
        return 2;
    }
    f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc(size);
    if (!buf || fread(buf, 1, size, f) != (size_t)size) {
        fprintf(stderr, "read failed\n");
        fclose(f);
        return 1;
    }
    fclose(f);

    idx = rd24(2);
    if (idx + 1 >= (uint32_t)size) {
        fprintf(stderr, "bad index offset\n");
        return 1;
    }
    ndev = buf[idx];
    printf("# %s: %ld bytes, %d section(s), %d device(s)\n",
           argv[1], size, buf[0], ndev);

    for (d = 0; d < ndev; d++) {
        uint32_t devoff = rd24(idx + 1 + (long)d * 3);
        int entries = rd16(devoff + 1);
        int seqs = entries / 2, s;
        printf("\n# device %d  (%d sequences)\n", d, seqs);
        for (s = 0; s < seqs; s++) {
            uint32_t press = rd24(devoff + 3 + (long)(2 * s) * 3);
            char desc[256];
            decode_signal(press, desc, sizeof(desc));
            printf("  dev %d seq %-3d : %s\n", d, s, desc);
        }
    }
    free(buf);
    return 0;
}
