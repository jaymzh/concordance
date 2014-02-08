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
 * (C) Copyright Kevin Timmerman 2007
 * (C) Copyright Phil Dibowitz 2010
 */

#include <string.h>
#include "libconcord.h"
#include "lc_internal.h"
#include "hid.h"
#include "remote.h"
#include "usblan.h"
#include "protocol_z.h"

/* Have we acked the syn packet yet? */
static bool SYN_ACKED = false;
static unsigned int last_seq;
static unsigned int last_ack;
static unsigned int last_payload_bytes;

int TCP_Ack(bool increment_ack = false, bool fin = false) {
    uint8_t pkt[HID_UDP_MAX_PACKET_SIZE];

    /*
     * Note: It's the caller's responsibility to ensure we've already
     * seen the SYN packet.
     */

    uint8_t seq;
    uint8_t ack;
    uint8_t flags;

    seq = last_ack;
    ack = last_seq + last_payload_bytes;
    if (increment_ack)
        ack++;
    flags = TYPE_TCP_ACK;
    if (fin)
        flags |= TYPE_TCP_FIN;
    pkt[0] = 3;
    pkt[1] = flags;
    pkt[2] = seq;
    pkt[3] = ack;

    debug("Writing packet:");
#ifdef _DEBUG
    for (int i = 0; i <= pkt[0]; i++) {
        fprintf(stderr, "%02X ", pkt[i]);
    }
    fprintf(stderr, "\n");
#endif

    return HID_WriteReport(pkt);
}


/*
 * The HID-based zwave remotes have two modes: so called "UDP" and "TCP". Do
 * not confuse these with the network protocols of similar names.
 *
 * The non-HID based zwave remotes only use "TCP".
 *
 * For more information on the various Read/Write functions here and how they
 * fit together, see the specs/protocol_z.txt file.
 */
int CRemoteZ_HID::UDP_Write(uint8_t typ, uint8_t cmd, uint32_t len,
                            uint8_t *data)
{
    if (len > 60)
        return LC_ERROR;
    uint8_t pkt[HID_UDP_MAX_PACKET_SIZE];
    pkt[0] = 3+len;
    pkt[1] = 1; // UDP
    pkt[2] = typ;
    pkt[3] = 0xFF & cmd;
    if (data && len)
        memcpy(pkt + 4, data, len);

    debug("Writing packet:");
#ifdef _DEBUG
    for (int i = 0; i <= pkt[0]; i++) {
        fprintf(stderr, "%02X ", pkt[i]);
    }
    fprintf(stderr, "\n");
#endif

    return HID_WriteReport(pkt);
}

int CRemoteZ_HID::UDP_Read(uint8_t &status, uint32_t &len, uint8_t *data)
{
    uint8_t pkt[HID_UDP_MAX_PACKET_SIZE];
    int err;
    if ((err = HID_ReadReport(pkt))) {
        return LC_ERROR_READ;
    }

    debug("Reading packet:");
#ifdef _DEBUG
    for (int i = 0; i <= pkt[0]; i++) {
        fprintf(stderr, "%02X ", pkt[i]);
    }
    fprintf(stderr, "\n");
#endif

    if (pkt[0] < 4) {
        return LC_ERROR;
    }
    if (pkt[0] > 4) {
        status = pkt[4];
    }
    len = pkt[0] - 4;
    /*
     * pkt[0] is the index of the last byte, which means it is equal to the
     * length of the packet minus one byte.  We want to copy everything but the
     * first byte, so we copy pkt[0] bytes.
     */
    memcpy(data, pkt + 1, pkt[0]);
    return 0;
}

int CRemoteZ_HID::TCP_Write(uint8_t typ, uint8_t cmd, uint32_t len,
                            uint8_t *data)
{
    uint8_t pkt[HID_UDP_MAX_PACKET_SIZE];

    /*
     * Note: It's the caller's responsibility to ensure we've already
     * seen the SYN packet.
     */

    uint8_t seq;
    uint8_t ack;
    uint8_t flags;

    if (!SYN_ACKED) {
        seq = 0x28;
        ack = last_seq + 1;
        flags = TYPE_TCP_ACK | TYPE_TCP_SYN;
        SYN_ACKED = true;
    } else {
        seq = last_ack;
        ack = last_seq + last_payload_bytes;
        flags = TYPE_TCP_ACK;
    }

    if (len > 60)
        return LC_ERROR;
    pkt[0] = 5+len;
    pkt[1] = flags;
    pkt[2] = seq;
    pkt[3] = ack;
    pkt[4] = typ;
    pkt[5] = cmd;
    if (data && len)
        memcpy(pkt + 6, data, len);

    debug("Writing packet:");
#ifdef _DEBUG
    for (int i = 0; i <= pkt[0]; i++) {
        fprintf(stderr, "%02X ", pkt[i]);
    }
    fprintf(stderr, "\n");
#endif

    return HID_WriteReport(pkt);
}


int CRemoteZ_HID::TCP_Read(uint8_t &status, uint32_t &len, uint8_t *data)
{
    uint8_t pkt[HID_UDP_MAX_PACKET_SIZE];
    int err;
    /*
     * Many TCP operations can take a while, like computing checksums,
     * and it will be a while before we get a response. So we set the
     * timeout to 30 seconds.
     */
    if ((err = HID_ReadReport(pkt, 30000))) {
        return LC_ERROR_READ;
    }

    debug("Reading packet:");
#ifdef _DEBUG
    for (int i = 0; i <= pkt[0]; i++) {
        fprintf(stderr, "%02X ", pkt[i]);
    }
    fprintf(stderr, "\n");
#endif

    if (pkt[0] < 3) {
        return LC_ERROR;
    }
    /*
     * pkt[0] is the index of the last byte, which means it is equal to the
     * length of packet minus one byte.  'len' is expected to be set to the
     * payload size.  To get the payload size we subtract both the TCP and
     * UDP headers from pkt[0] and then add one.
     */
    len = pkt[0] - HID_TCP_HDR_SIZE - HID_UDP_HDR_SIZE + 1;
    last_seq = pkt[2];
    last_ack = pkt[3];
    last_payload_bytes = len + HID_UDP_HDR_SIZE; // tcp payload size
    //if(!len) return 0;
    //memcpy(data, pkt + 6, len);
    // include headers, minus the size
    memcpy(data, pkt + 1, len + HID_TCP_HDR_SIZE + HID_UDP_HDR_SIZE - 1);
    return 0;
}

int CRemoteZ_HID::Write(uint8_t typ, uint8_t cmd, uint32_t len, uint8_t *data)
{
    return UDP_Write(typ, cmd, len, data);
}

int CRemoteZ_HID::Read(uint8_t &status, uint32_t &len, uint8_t *data)
{
    return UDP_Read(status, len, data);
}

int CRemoteZ_HID::ParseParams(uint32_t len, uint8_t *data, TParamList &pl)
{
    debug("ParseParams, %02x %02x %02x %02x %02x %02x\n", data[0], data[1],
          data[2], data[3], data[4], data[5]);
    switch (data[2]) {
        case COMMAND_GET_SYSTEM_INFO:
            pl.count = 8;
            pl.p[0] = data + 4;
            pl.p[1] = data + 6;
            pl.p[2] = data + 8;
            pl.p[3] = data + 10;
            pl.p[4] = data + 12;
            pl.p[5] = data + 14;
            pl.p[6] = data + 15;
            pl.p[7] = data + 17;
            break;
        case COMMAND_GET_CURRENT_TIME:
            pl.count = (len > 16) ? 12 : 8;
            pl.p[0] = data + 4;
            pl.p[1] = data + 6;
            pl.p[2] = data + 7;
            pl.p[3] = data + 8;
            pl.p[4] = data + 9;
            pl.p[5] = data + 10;
            pl.p[6] = data + 11;
            pl.p[7] = data + 12;
            pl.p[8] = data + 14;
            pl.p[9] = data + 16;
            pl.p[10] = data + 18;
            pl.p[11] = data + 20;
            break;
        case COMMAND_GET_GUID:
            pl.count = 1;
            pl.p[0] = data + 4;
            break;
    }
    return 0;
}


int CRemoteZ_USBNET::Write(uint8_t typ, uint8_t cmd, uint32_t len,
                           uint8_t *data)
{
    if (len > USBNET_MAX_PACKET_SIZE) {
        return LC_ERROR;
    }

    static const uint8_t service_type = SERVICE_FAMILY_CLIENT;
    const bool request = (typ == TYPE_REQUEST);
    const uint8_t status = STATUS_OK;

    uint8_t pkt[USBNET_MAX_PACKET_SIZE+3]; /* add standard 3-byte header */
    pkt[0] = (service_type << 4) | ((cmd >> 8) & 0x0F);
    pkt[1] = cmd & 0xFF;
    pkt[2] = request ? 0x80 : (status & 0x7F);

    if (len && data) {
        memcpy(pkt + 3, data, len);
        len += 3;
    } else {
        pkt[3] = 0x00;    // Param count
        len = 4;
    }

    return UsbLan_Write(len, pkt);
}

int CRemoteZ_USBNET::Read(uint8_t &status, uint32_t &len, uint8_t *data)
{
    uint8_t buf[1600];
    len = sizeof(buf);
    int err;
    if ((err = UsbLan_Read(len, buf)))
        return err;

    memcpy(data, buf, len);

    return err;
}

int CRemoteZ_USBNET::ParseParams(uint32_t len, uint8_t *data, TParamList &pl)
{
    unsigned int n = 0;
    unsigned int i = 4;
    while (i < len) {
        unsigned int param_len = data[i];
        switch (param_len & 0xC0) {
            case 0x00:
            case 0x80:
                param_len &= 0x3F;
                break;
            case 0x40:
                param_len = (param_len & 0x3F) * 4;
                break;
            case 0xC0:
                param_len = (param_len & 0x3F) * 512;
                break;
        }
        ++i;
        pl.p[n++] = data+i;
#ifdef _DEBUG
        fprintf(stderr, "DEBUG (%s): %3i:", __FUNCTION__, param_len);
        for(unsigned int j = 0; j < param_len; ++j)
            fprintf(stderr, " %02X", data[i+j]);
        fprintf(stderr, "\n");
#endif
        i += param_len;
    }

    //data[3];    // Number of parameters
    pl.count = n;

    return 0;
}

int CRemoteZ_USBNET::TCPSendAndCheck(uint8_t cmd, uint32_t len, uint8_t *data)
{
    int err = 0;
    uint8_t status;
    unsigned int rlen;
    uint8_t rsp[60];

    if ((err = Write(TYPE_REQUEST, cmd, len, data))) {
        debug("Failed to send request %02X", cmd);
        return LC_ERROR_WRITE;
    }

    if ((err = Read(status, rlen, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR_READ;
    }

    if (rsp[2] != TYPE_RESPONSE) {
        debug("Packet didn't have response bit!");
        return LC_ERROR;
    }

    if (rsp[1] != cmd) {
        debug("The cmd bit didn't match our request packet");
        return LC_ERROR;
    }

    return 0;
}

int CRemoteZ_USBNET::UpdateConfig(const uint32_t len, const uint8_t *wr,
                                  lc_callback cb, void *cb_arg,
                                  uint32_t cb_stage, uint32_t xml_size,
                                  uint8_t *xml)
{
    int err = 0;
    int cb_count = 0;

    cb(LC_CB_STAGE_INITIALIZE_UPDATE, cb_count++, 0, 2,
       LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

    /* ACK it with a command to start an update */
    debug("START_UPDATE");
    // 2 parameters, each 1 byte.
    // 1st parameter "flags" seems to always be 0.
    uint8_t cmd[60] = { 0x02, 0x01, 0x00, 0x01, REGION_USER_CONFIG };
    if ((err = TCPSendAndCheck(COMMAND_START_UPDATE, 5, cmd))) {
        return err;
    }

    cb(LC_CB_STAGE_INITIALIZE_UPDATE, cb_count++, 1, 2,
        LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

    /* write update-header */
    debug("UPDATE_HEADER");
    cmd[0] = 0x02; // 2 parameters
    cmd[1] = 0x04; // 1st parameter 4 bytes (size)
    cmd[2] = (len & 0xFF000000) >> 24; 
    cmd[3] = (len & 0x00FF0000) >> 16;
    cmd[4] = (len & 0x0000FF00) >> 8;
    cmd[5] = (len & 0x000000FF);
    cmd[6] = 0x01; // 2nd parameter 1 byte (region id)
    cmd[7] = REGION_USER_CONFIG;
    if ((err = TCPSendAndCheck(COMMAND_WRITE_UPDATE_HEADER, 8, cmd))) {
        return err;
    }

    cb(LC_CB_STAGE_INITIALIZE_UPDATE, cb_count++, 2, 2,
       LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);
    cb_count = 0;

    /* write data */
    debug("UPDATE_DATA");
    uint32_t pkt_len;
    uint32_t tlen = len;
    uint8_t *wr_ptr = const_cast<uint8_t*>(wr);
    uint8_t tmp_pkt[1033];
    tmp_pkt[0] = 0x03; // 3 parameters
    tmp_pkt[1] = 0x01; // 1st parameter, 1 byte (region id)
    tmp_pkt[2] = REGION_USER_CONFIG;
    tmp_pkt[3] = 0xC2; // 2nd parameter, 1024 bytes (data)
    tmp_pkt[1028] = 0x04; // 3rd parameter, 4 bytes (length)
    while (tlen) {
        pkt_len = 1024; // max packet length seems to be 1024
        if (tlen < pkt_len) {
            pkt_len = tlen;
        }
        tlen -= pkt_len;

        memcpy(&tmp_pkt[4], wr_ptr, pkt_len);
        tmp_pkt[1029] = (pkt_len & 0xFF000000) >> 24;
        tmp_pkt[1030] = (pkt_len & 0x00FF0000) >> 16;
        tmp_pkt[1031] = (pkt_len & 0x0000FF00) >> 8;
        tmp_pkt[1032] = (pkt_len & 0x000000FF);

        debug("DATA %d, sending %d bytes, %d bytes left", cb_count,
            pkt_len, tlen);

        if ((err = TCPSendAndCheck(COMMAND_WRITE_UPDATE_DATA, 1033, tmp_pkt))) {
            return err;
        }
        wr_ptr += pkt_len;

        if (cb) {
            cb(LC_CB_STAGE_WRITE_CONFIG, cb_count++, (int)(wr_ptr - wr), len,
               LC_CB_COUNTER_TYPE_BYTES, cb_arg, NULL);
        }
    }

    /* write update-done */
    cb_count = 0;
    cb(LC_CB_STAGE_FINALIZE_UPDATE, cb_count++, 0, 3, LC_CB_COUNTER_TYPE_STEPS,
       cb_arg, NULL);

    debug("UPDATE_DATA_DONE");
    cmd[0] = 0x01; // 1 parameter
    cmd[1] = 0x01; // 1st parameter 1 byte (region id)
    cmd[2] = REGION_USER_CONFIG;
    if ((err = TCPSendAndCheck(COMMAND_WRITE_UPDATE_DATA_DONE, 3, cmd))) {
        return err;
    }

    cb(LC_CB_STAGE_FINALIZE_UPDATE, cb_count++, 1, 3, LC_CB_COUNTER_TYPE_STEPS,
       cb_arg, NULL);

    /* send get-cheksum */
    debug("GET_CHECKSUM");
    cmd[0] = 0x02; // 2 parameters
    cmd[1] = 0x02; // 1st parameter 2 bytes (seed)
    cmd[2] = 0xFF; // seed seems to always be FF
    cmd[3] = 0xFF; // seed seems to always be FF
    cmd[4] = 0x01; // 2nd parameter 1 byte (region id)
    cmd[5] = REGION_USER_CONFIG;
    if ((err = TCPSendAndCheck(COMMAND_GET_UPDATE_CHECKSUM, 6, cmd))) {
        return err;
    }

    cb(LC_CB_STAGE_FINALIZE_UPDATE, cb_count++, 2, 3, LC_CB_COUNTER_TYPE_STEPS,
       cb_arg, NULL);

    /* send finish-update */
    debug("FINISH_UPDATE");
    cmd[0] = 0x02; // 2 parameters
    cmd[1] = 0x01; // 1st parameter 1 byte (validate)
    cmd[2] = 0x01; // validate?
    cmd[3] = 0x01; // 2nd parameter 1 byte (region id)
    cmd[4] = REGION_USER_CONFIG;
    if ((err = TCPSendAndCheck(COMMAND_FINISH_UPDATE, 5, cmd))) {
        return err;
    }

    cb(LC_CB_STAGE_FINALIZE_UPDATE, cb_count++, 3, 3, LC_CB_COUNTER_TYPE_STEPS,
       cb_arg, NULL);

    return 0;
}

int CRemoteZ_USBNET::GetTime(const TRemoteInfo &ri, THarmonyTime &ht)
{
    int err = 0;
    if ((err = Write(TYPE_REQUEST, COMMAND_GET_CURRENT_TIME))) {
        debug("Failed to write to remote");
        return LC_ERROR_WRITE;
    }
    uint8_t time[60];
    unsigned int len;
    uint8_t status;
    if ((err = Read(status, len, time))) {
        debug("Failed to read to remote");
        return LC_ERROR_READ;
    }

    if (time[2] != TYPE_RESPONSE || time[1] != COMMAND_GET_CURRENT_TIME) {
        debug("Incorrect response type from Get Time");
        return LC_ERROR_INVALID_DATA_FROM_REMOTE;
    }

    CRemoteZ_Base::TParamList pl;
    ParseParams(len, time, pl);

    ht.year = GetWord(pl.p[0]);
    ht.month = *pl.p[1];
    ht.day = *pl.p[2];
    ht.hour = *pl.p[3];
    ht.minute = *pl.p[4];
    ht.second = *pl.p[5];
    ht.dow = *pl.p[6]&7;
    ht.utc_offset = static_cast<int16_t>(GetWord(pl.p[7]));
    if (pl.count > 11) {
        ht.timezone = reinterpret_cast<char*>(pl.p[11]);
        ht.timezone[3] = '\0';
    } else {
        ht.timezone = "";
    }

    return 0;
}

int CRemoteZ_USBNET::SetTime(const TRemoteInfo &ri, const THarmonyTime &ht,
                             lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
    debug("SetTime");
    int err = 0;

    uint8_t tsv[32] = {
        0x0C, // 12 parameters
        0x02, static_cast<uint8_t>(ht.year >> 8),
        static_cast<uint8_t>(ht.year), // 2 bytes
        0x01, static_cast<uint8_t>(ht.month),
        0x01, static_cast<uint8_t>(ht.day),
        0x01, static_cast<uint8_t>(ht.hour),
        0x01, static_cast<uint8_t>(ht.minute),
        0x01, static_cast<uint8_t>(ht.second),
        0x01, static_cast<uint8_t>(ht.dow),
        // utcOffset
        0x02, 0, 0, // 2 bytes - 900 doesn't seem to accept this
        // 0s
        0x02, 0, 0, // 2 bytes
        0x02, 0, 0, // 2 bytes
        0x02, 0, 0, // 2 bytes
        0x83, 0, 0, 0 // 900 doesn't seem to accept this, don't bother
        };

    if ((err = TCPSendAndCheck(COMMAND_UPDATE_TIME, 32, tsv))) {
        debug("Failed to write to remote");
        return err;
    }

    return 0;
}

int CRemoteZ_USBNET::LearnIR(uint32_t *freq, uint32_t **ir_signal,
                             uint32_t *ir_signal_length, lc_callback cb,
                             void *cb_arg, uint32_t cb_stage)
{
    return LC_ERROR_UNSUPP;
}

int CRemoteZ_USBNET::ReadRegion(uint8_t region, uint32_t &rgn_len, uint8_t *rd,
                                lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
    int err = 0;
    int cb_count = 0;
    uint8_t rsp[60];
    unsigned int rlen;
    uint8_t status;
    CRemoteZ_Base::TParamList pl;

    debug("READ_REGION");
    // 1 parameters, 1 byte, region to read.
    uint8_t cmd[60] = { 0x01, 0x01, region };
    if ((err = Write(TYPE_REQUEST, COMMAND_READ_REGION, 3, cmd))) {
        debug("Failed to write to remote");
        return LC_ERROR_WRITE;
    }
    if ((err = Read(status, rlen, rsp))) {
        debug("Failed to read to remote");
        return LC_ERROR_READ;
    }
    if (rsp[2] != TYPE_RESPONSE || rsp[1] != COMMAND_READ_REGION || rlen != 9 ||
        rsp[4] != 0x04) {
        debug("Incorrect response type from remote");
        return LC_ERROR_INVALID_DATA_FROM_REMOTE;
    }
    ParseParams(rlen, rsp, pl);
    rgn_len = GetWord32(pl.p[0]);

    debug("READ_REGION_DATA");
    uint32_t pkt_len;
    unsigned int data_to_read = rgn_len;
    uint8_t *rd_ptr = rd;
    uint8_t tmp_pkt[USBNET_MAX_PACKET_SIZE];
    cmd[0] = 0x01; // 1 parameter
    cmd[1] = 0x01; // 1st parameter, 1 byte (region id)
    cmd[2] = region;

    while (data_to_read) {
        if ((err = Write(TYPE_REQUEST, COMMAND_READ_REGION_DATA, 3, cmd))) {
            debug("Failed to write to remote");
            return LC_ERROR_WRITE;
        }
        if ((err = Read(status, rlen, tmp_pkt))) {
            debug("Failed to read to remote");
            return LC_ERROR_READ;
        }
        if (tmp_pkt[2] != TYPE_RESPONSE ||
            tmp_pkt[1] != COMMAND_READ_REGION_DATA) {
            debug("Incorrect response type from remote");
            return LC_ERROR_INVALID_DATA_FROM_REMOTE;
        }
        ParseParams(rlen, tmp_pkt, pl);
        pkt_len = GetWord32(pl.p[2]);
        data_to_read -= pkt_len;

        if (rd) {
            memcpy(rd_ptr, pl.p[1], pkt_len);
            rd_ptr += pkt_len;
        }

        debug("DATA %d, read %d bytes, %d bytes left", cb_count, pkt_len,
              data_to_read);

        if (cb) {
            cb(cb_stage, cb_count++, rgn_len - data_to_read, rgn_len,
               LC_CB_COUNTER_TYPE_BYTES, cb_arg, NULL);
        }
    }

    debug("READ_REGION_DONE");
    // 1 parameter, 1 byte, region to read.
    cmd[0] = 0x01;
    cmd[1] = 0x01;
    cmd[2] = region;
    if ((err = TCPSendAndCheck(COMMAND_READ_REGION_DONE, 3, cmd))) {
        return err;
    }

    return 0;
}

int CRemoteZ_Base::Reset(uint8_t kind)
{
    int err = 0;
    /*
     * TODO: I don't believe the zwaves have a "kind" of reset
     * ... is this needed here?
     */
    if (kind != 2) {
        return LC_ERROR;
    }

    if ((err = Write(TYPE_REQUEST, COMMAND_Z_RESET))) {
        debug("Failed to write to remote");
        return LC_ERROR_WRITE;
    }
    uint8_t rsp[60];
    unsigned int len;
    uint8_t status;
    if ((err = Read(status, len, rsp))) {
        debug("Failed to read to remote");
        return LC_ERROR_READ;
    }

    /*
     * TODO: Either the remote is gone at this point, and reading
     * will fail, or we need a "finalize action" command afterwards,
     * in which case we should check the actual return status from
     * the remote.
     */
    /*
    if (time[1] != TYPE_RESPONSE || time[2] != COMMAND_Z_RESET) {
        return LC_ERROR_INVALID_DATA_FROM_REMOTE;
    }
    */

    return 0;
}

int CRemoteZ_Base::GetIdentity(TRemoteInfo &ri, THIDINFO &hid, lc_callback cb,
                               void *cb_arg, uint32_t cb_stage)
{
    int err = 0;
    if ((err = Write(TYPE_REQUEST, COMMAND_GET_SYSTEM_INFO))) {
        debug("Failed to write to remote");
        return LC_ERROR_WRITE;
    }
    uint8_t rsp[60];
    unsigned int len;
    uint8_t status;
    if ((err = Read(status, len, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR_READ;
    }

    if (cb) {
        cb(cb_stage, 0, 1, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);
    }

    CRemoteZ_Base::TParamList pl;
    ParseParams(len, rsp, pl);

    ri.flash_mfg = 0x01;
    ri.flash_id = 0x49;
    ri.architecture = GetWord(pl.p[2]);
    ri.fw_ver_major = GetWord(pl.p[3]);
    ri.fw_ver_minor = GetWord(pl.p[4]);
    ri.fw_type = *pl.p[5];
    ri.skin = GetWord(pl.p[6]);
    const unsigned int hw = GetWord(pl.p[7]);
    ri.hw_ver_major = hw >> 8;        // ???
    ri.hw_ver_minor = (hw >> 4) & 0x0F;    // ???
    ri.hw_ver_micro = hw & 0x0F;        // 900 has this and is non-zero
    //ri.hw_ver_major = hw >> 4;
    //ri.hw_ver_minor = hw & 0x0F;
    ri.protocol = GetWord(pl.p[2]); // Seems to be the same as arch?

    setup_ri_pointers(ri);

    if (IsUSBNet()) {
        hid.vid = GetWord(pl.p[0]);
        hid.pid = GetWord(pl.p[1]);
        hid.ver = 0;
        hid.irl = 0;
        hid.orl = 0;
        hid.frl = 0;
        hid.mfg = ri.model->mfg;
        hid.prod = ri.model->model;
    }

    if ((err = Write(TYPE_REQUEST, COMMAND_GET_GUID))) {
        debug("Failed to write to remote");
        return LC_ERROR_WRITE;
    }
    if ((err = Read(status, len, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR_READ;
    }

    if (cb) {
        cb(LC_CB_STAGE_GET_IDENTITY, 1, 2, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg,
           NULL);
    }

    ParseParams(len, rsp, pl);

    make_serial(pl.p[0], ri);

    if (IsUSBNet()) {
        // Get the User Config Region to find the config bytes used.
        if ((err = ReadRegion(REGION_USER_CONFIG, ri.config_bytes_used, NULL,
                              NULL, NULL, 0))) {
            return err;
        }
    } else {
        ri.config_bytes_used = 0;
    }
    ri.max_config_size = 1;
    ri.valid_config = 1;

    if (!IsUSBNet()) {
        return 0;
    }
    
    // Get region info - everything below is extra stuff that only the
    // usbnet remotes seem to use.
    uint8_t rr[] = { 1, 1, 1 }; // AddByteParam(1);
    if ((err = Write(TYPE_REQUEST, COMMAND_GET_REGION_IDS, 3, rr))) {
        debug("Failed to write to remote");
        return LC_ERROR;
    }
    uint8_t rgn[64];
    if ((err = Read(status, len, rgn))) {
        debug("Failed to read from remote");
        return LC_ERROR;
    }
    ParseParams(len, rgn, pl);
    if (pl.count == 1) {
        const unsigned int rc = *(pl.p[0]-1) & 0x3F;
        ri.num_regions = rc + 1;
        ri.region_ids = new uint8_t[ri.num_regions];
        ri.region_versions = new char*[ri.num_regions];

        // There seems to be an implied Region 0 that the Windows
        // software requests but it is not listed in the Region IDs.
        uint8_t rz[] = { 1, 1, 0 }; // 1 parameter, 1 byte, value = 0
        if ((err = Write(TYPE_REQUEST, COMMAND_GET_REGION_VERSION, 3, rz))) {
            debug("Failed to write to remote");
            return LC_ERROR;
        }
        uint8_t rgz[64];
        if ((err = Read(status, len, rgz))) {
            debug("Failed to read from remote");
            return LC_ERROR;
        }
        CRemoteZ_Base::TParamList rzp;
        ParseParams(len, rgz, rzp);
        ri.region_ids[0] = 0x00; // Store region 0.
        ri.region_versions[0] = new char[4];
        snprintf(ri.region_versions[0], 4, "%d.%d", *(rzp.p[0]+1), *rzp.p[0]);

        for(unsigned int r = 0; r < rc; ++r) {
            const uint8_t rn = *(pl.p[0]+r);
            debug("Region %i", rn);
            uint8_t rv[] = { 1, 1, rn }; // AddByteParam(rn);
            if ((err = Write(TYPE_REQUEST, COMMAND_GET_REGION_VERSION, 3,
                             rv))) {
                debug("Failed to write to remote");
                return LC_ERROR;
            }
            uint8_t rgv[64];
            if ((err = Read(status, len, rgv))) {
                debug("Failed to read from remote");
                return LC_ERROR;
            }
            CRemoteZ_Base::TParamList rp;
            ParseParams(len, rgv, rp);
            ri.region_ids[r+1] = rn;
            ri.region_versions[r+1] = new char[4];
            snprintf(ri.region_versions[r+1], 4, "%d.%d", *(rp.p[0]+1),
                     *rp.p[0]);
        }
    }

    // Get HOMEID, NODEID, TID
    if ((err = Write(TYPE_REQUEST, COMMAND_GET_HOME_ID))) {
        debug("Failed to write to remote");
        return LC_ERROR;
    }
    if ((err = Read(status, len, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR;
    }
    ParseParams(len, rsp, pl);
    ri.home_id = (*pl.p[0] << 24) + (*(pl.p[0]+1) << 16) +
            (*(pl.p[0]+2) << 8) + *(pl.p[0]+3);
    if ((err = Write(TYPE_REQUEST, COMMAND_GET_NODE_ID))) {
        debug("Failed to write to remote");
        return LC_ERROR;
    }
    if ((err = Read(status, len, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR;
    }
    ParseParams(len, rsp, pl);
    ri.node_id = *pl.p[0];
    // Get the "TID" - not entirely sure what this is, but it appears to be
    // contained in the INTERFACE_LIST.
    if ((err = Write(TYPE_REQUEST, COMMAND_GET_INTERFACE_LIST))) {
        debug("Failed to write to remote");
        return LC_ERROR;
    }
    if ((err = Read(status, len, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR;
    }
    ParseParams(len, rsp, pl);
    ri.tid = new char[21];
    ri.tid[0] = '0';
    ri.tid[1] = 'x';
    // "TID" appears to be contained in the middle of the INTERFACE_LIST.
    for (int i = 0; i < 9; i++)
        snprintf(&ri.tid[(i*2)+2], 3, "%02X", *(pl.p[1]+i+2));
    ri.tid[20] = '\0';

    // Get the "XMLUserRFSetting" - the usbnet remote appears to have a web
    // server running where the Windows driver grabs this data.
    if ((err = GetXMLUserRFSetting(&ri.xml_user_rf_setting))) {
        debug("Failed to read XML User RF Settings");
        return LC_ERROR;
    }
    
    return 0;
}

int CRemoteZ_Base::ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd,
                             unsigned int protocol, bool verify, lc_callback cb,
                             void *cb_arg, uint32_t cb_stage)
{
    uint32_t tmp;
    return ReadRegion(addr, tmp, rd, cb, cb_arg, cb_stage);
}

/*
 * When reading a config from the remote, we need to look for a sequence of
 * four bytes to determine when to stop reading the config.  The tricky part is
 * that the sequence could be split across two packets.  Thus, we need to
 * search the last three bytes of the previous packet plus the current packet.
 * This function searches for the "end of file" sequence in a set of two
 * packets.  If the sequence is found, returns the number of bytes in the
 * second packet up to and including the sequence.  Returns 0 if the sequence
 * is not found.  Parameters point to the data section of the packets.  Note for
 * the first packet, only the last 3 bytes are expected to be passed in.
 */
int FindEndSeq(uint8_t *pkt_1, uint8_t *pkt_2)
{
    uint8_t end_seq[4] = { 0x44, 0x4B, 0x44, 0x4B }; // end of file sequence
    uint8_t tmp[57]; // 3 bytes from the 1st packet, 54 bytes from the 2nd
    memcpy(&tmp, pkt_1, 3);
    memcpy(&tmp[3], pkt_2, 54);
    for (int i=0; i<54; i++) {
        if (memcmp(&end_seq, &tmp[i], 4) == 0) {
            return i + 1;
        } 
    }
    return 0;
}

int CRemoteZ_HID::ReadRegion(uint8_t region, uint32_t &rgn_len, uint8_t *rd,
                             lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
    int err = 0;
    int cb_count = 0;
    uint8_t rsp[60];
    unsigned int rlen;
    uint8_t status;

    /* Start a TCP transfer */
    if ((err = Write(TYPE_REQUEST, COMMAND_INITIATE_UPDATE_TCP_CHANNEL))) {
        debug("Failed to write to remote");
        return LC_ERROR_WRITE;
    }

    /* Make sure the remote is ready to start the TCP transfer */
    if ((err = Read(status, rlen, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR_READ;
    }

    if (rsp[1] != TYPE_RESPONSE || rsp[2] !=
        COMMAND_INITIATE_UPDATE_TCP_CHANNEL) {
        return LC_ERROR;
    }

    /* Look for a SYN packet */
    debug("Looking for syn");
    if ((err = TCP_Read(status, rlen, rsp))) {
        debug("Failed to read syn from remote");
        return LC_ERROR_READ;
    }

    if (rsp[0] != TYPE_TCP_SYN) {
        debug("Not a SYN packet!");
        return LC_ERROR;
    }

    /* ACK it with a command to read a region */
    debug("READ_REGION");
    // 1 parameters, 1 byte, region to read.
    uint8_t cmd[60] = { region };
    if ((err = TCP_Write(TYPE_REQUEST, COMMAND_READ_REGION, 1, cmd))) {
        debug("Failed to write to remote");
        return LC_ERROR_WRITE;
    }
    if ((err = TCP_Read(status, rlen, rsp))) {
        debug("Failed to read to remote");
        return LC_ERROR_READ;
    }
    if (rsp[0] != TYPE_TCP_ACK || rsp[3] != TYPE_RESPONSE ||
        rsp[4] != COMMAND_READ_REGION) {
         debug("Incorrect response type from remote");
        return LC_ERROR_INVALID_DATA_FROM_REMOTE;
    }

    rgn_len = 0;

    debug("READ_REGION_DATA");
    int data_read = 0;
    uint8_t *rd_ptr = rd;
    cmd[0] = region;
    int eof_found = 0;
    uint8_t prev_pkt_tail[3] = { 0x00, 0x00, 0x00 };

    while (1) {
        if ((err = TCP_Write(TYPE_REQUEST, COMMAND_READ_REGION_DATA, 1, cmd))) {
            debug("Failed to write to remote");
            return LC_ERROR_WRITE;
        }
        if ((err = TCP_Read(status, rlen, rsp))) {
            debug("Failed to read to remote");
            return LC_ERROR_READ;
        }
        if (rsp[0] != TYPE_TCP_ACK || rsp[3] != TYPE_RESPONSE ||
            ((rsp[4] != COMMAND_READ_REGION_DATA) &&
            (rsp[4] != COMMAND_READ_REGION_DONE))) {
             debug("Incorrect response type from remote");
            return LC_ERROR_INVALID_DATA_FROM_REMOTE;
        }
        if (rsp[4] == COMMAND_READ_REGION_DONE) {
            break;
        }
        data_read += rlen;

        if (!eof_found) {
            eof_found = FindEndSeq(prev_pkt_tail, &rsp[5]);
            if (eof_found) {
                rlen = eof_found;
            }
            rgn_len += rlen;
            memcpy(&prev_pkt_tail, &rsp[56], 3);

            if (rd) {
                memcpy(rd_ptr, &rsp[5], rlen);
                rd_ptr += rlen;
            }
        }

        debug("DATA %d, read %d bytes, %d bytes total", cb_count, rlen,
              data_read);

        if (cb) {
            cb(cb_stage, cb_count++, data_read, data_read+1,
               LC_CB_COUNTER_TYPE_BYTES, cb_arg, NULL);
        }
    }

    debug("FIN-ACK");
    if ((err = TCP_Ack(false, true))) {
        debug("Failed to send fin-ack");
        return LC_ERROR_WRITE;
    }

    if ((err = TCP_Read(status, rlen, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR_READ;
    }

    /* Make sure we got an ack */
    if (rsp[0] != (TYPE_TCP_ACK | TYPE_TCP_FIN)) {
        debug("Failed to read finish-update ack");
        return LC_ERROR;
    }

    if ((err = TCP_Ack(true, false))) {
        debug("Failed to ack the ack of our fin-ack");
        return LC_ERROR_WRITE;
    }

    /* Return TCP state to initial conditions */
    SYN_ACKED = false;

    if (cb) {
        cb(cb_stage, cb_count++, data_read, data_read, LC_CB_COUNTER_TYPE_BYTES,
           cb_arg, NULL);
    }

    return 0;
}

int CRemoteZ_Base::InvalidateFlash(lc_callback cb, void *cb_arg,
                                   uint32_t cb_stage)
{
    return 0;
}

int CRemoteZ_Base::EraseFlash(uint32_t addr, uint32_t len,
                              const TRemoteInfo &ri, lc_callback cb,
                              void *cb_arg, uint32_t cb_stage)
{
    return 0;
}

int CRemoteZ_Base::WriteFlash(uint32_t addr, const uint32_t len,
                              const uint8_t *wr, unsigned int protocol,
                              lc_callback cb, void *arg, uint32_t cb_stage)
{
    return 0;
}

int CRemoteZ_Base::WriteRam(uint32_t addr, const uint32_t len, uint8_t *wr)
{
    return 0;
}

int CRemoteZ_Base::ReadRam(uint32_t addr, const uint32_t len, uint8_t *rd)
{
    return 0;
}

int CRemoteZ_Base::PrepFirmware(const TRemoteInfo &ri, lc_callback cb,
                                void *cb_arg, uint32_t cb_stage)
{
    return 0;
}

int CRemoteZ_Base::FinishFirmware(const TRemoteInfo &ri, lc_callback cb,
                                  void *cb_arg, uint32_t cb_stage)
{
    return 0;
}

int CRemoteZ_Base::PrepConfig(const TRemoteInfo &ri, lc_callback cb,
                              void *cb_arg, uint32_t cb_stage)
{
    return 0;
}

int CRemoteZ_Base::FinishConfig(const TRemoteInfo &ri, lc_callback cb,
                                void *cb_arg, uint32_t cb_stage)
{
    return 0;
}

int CRemoteZ_Base::GetTime(const TRemoteInfo &ri, THarmonyTime &ht)
{
    int err = 0;
    if ((err = Write(TYPE_REQUEST, COMMAND_GET_CURRENT_TIME))) {
        debug("Failed to write to remote");
        return LC_ERROR_WRITE;
    }
    uint8_t time[60];
    unsigned int len;
    uint8_t status;
    if ((err = Read(status, len, time))) {
        debug("Failed to read to remote");
        return LC_ERROR_READ;
    }

    if (time[1] != TYPE_RESPONSE || time[2] != COMMAND_GET_CURRENT_TIME) {
        debug("Incorrect response type from Get Time");
        return LC_ERROR_INVALID_DATA_FROM_REMOTE;
    }

    CRemoteZ_Base::TParamList pl;
    ParseParams(len, time, pl);

    ht.year = GetWord(pl.p[0]);
    ht.month = *pl.p[1];
    ht.day = *pl.p[2];
    ht.hour = *pl.p[3];
    ht.minute = *pl.p[4];
    ht.second = *pl.p[5];
    ht.dow = *pl.p[6]&7;
    ht.utc_offset = static_cast<int16_t>(GetWord(pl.p[7]));
    if (pl.count > 11) {
        ht.timezone = reinterpret_cast<char*>(pl.p[11]);
    } else {
        ht.timezone = "";
    }

    return 0;
}

int CRemoteZ_Base::SetTime(const TRemoteInfo &ri, const THarmonyTime &ht,
                           lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
    int err = 0;

    uint8_t tsv[16] = { static_cast<uint8_t>(ht.year), 0, // 2 bytes
        static_cast<uint8_t>(ht.month),
        static_cast<uint8_t>(ht.day),
        static_cast<uint8_t>(ht.hour),
        static_cast<uint8_t>(ht.minute),
        static_cast<uint8_t>(ht.second),
        static_cast<uint8_t>(ht.dow),
        // utcOffset
        static_cast<uint8_t>(ht.utc_offset), 0, // 2 bytes
        // 0s
        0, 0, // 2 bytes
        0, 0, // 2 bytes
        0, 0, // 2 bytes
        // ht.timezone.c_str()
        };

    if ((err = Write(TYPE_REQUEST, COMMAND_UPDATE_TIME, 16, tsv))) {
        debug("Failed to write to remote");
        return LC_ERROR_WRITE;
    }

    uint8_t rsp[60];
    unsigned int len;
    uint8_t status;
    if ((err = Read(status, len, rsp))) {
        debug("failed to read from remote");
        return LC_ERROR_READ;
    }

    if (rsp[1] != TYPE_RESPONSE || rsp[2] != COMMAND_UPDATE_TIME) {
            return LC_ERROR;
    }

    return 0;
}

int CRemoteZ_HID::TCPSendAndCheck(uint8_t cmd, uint32_t len, uint8_t *data,
                                  bool ackonly)
{
    int err = 0;
    uint8_t status;
    unsigned int rlen;
    uint8_t rsp[60];

    if ((err = TCP_Write(TYPE_REQUEST, cmd, len, data))) {
        debug("Failed to send request %02X", cmd);
        return LC_ERROR_WRITE;
    }

    if ((err = TCP_Read(status, rlen, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR_READ;
    }

    /* make sure it was the response we expected */
    if (rsp[0] != TYPE_TCP_ACK) {
        debug("Packet wasn't an ACK!");
        return LC_ERROR;
    }
    if (!ackonly) {
        if (rsp[3] != TYPE_RESPONSE) {
            debug("Packet didn't have response bit!");
            return LC_ERROR;
        }
        if (rsp[4] != cmd) {
            debug("The cmd bit didn't match our request packet");
            return LC_ERROR;
        }
    }

    return 0;
}

int CRemoteZ_HID::UpdateConfig(const uint32_t len, const uint8_t *wr,
                               lc_callback cb, void *cb_arg, uint32_t cb_stage,
                               uint32_t xml_size, uint8_t *xml)
{
    int err = 0;
    int cb_count = 0;

    cb(LC_CB_STAGE_INITIALIZE_UPDATE, cb_count++, 0, 4,
       LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);
    /* Start a TCP transfer */
    if ((err = Write(TYPE_REQUEST, COMMAND_INITIATE_UPDATE_TCP_CHANNEL))) {
        debug("Failed to write to remote");
        return LC_ERROR_WRITE;
    }

    uint8_t rsp[60];
    unsigned int rlen;
    uint8_t status;

    /* Make sure the remote is ready to start the TCP transfer */
    if ((err = Read(status, rlen, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR_READ;
    }

    if (rsp[1] != TYPE_RESPONSE || rsp[2] !=
        COMMAND_INITIATE_UPDATE_TCP_CHANNEL) {
        return LC_ERROR;
    }
    cb(LC_CB_STAGE_INITIALIZE_UPDATE, cb_count++, 1, 4,
       LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

    /* Look for a SYN packet */
    debug("Looking for syn");
    if ((err = TCP_Read(status, rlen, rsp))) {
        debug("Failed to read syn from remote");
        return LC_ERROR_READ;
    }

    if (rsp[0] != TYPE_TCP_SYN) {
        debug("Not a SYN packet!");
        return LC_ERROR;
    }
    cb(LC_CB_STAGE_INITIALIZE_UPDATE, cb_count++, 2, 4,
       LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

    /* ACK it with a command to start an update */
    debug("START_UPDATE");
    uint8_t cmd[60] = { 0x00, 0x04 };
    if ((err = TCP_Write(TYPE_REQUEST, COMMAND_START_UPDATE, 2, cmd))) {
        debug("Failed to write start-update to remote");
        return LC_ERROR_WRITE;
    }

    if ((err = TCP_Read(status, rlen, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR_READ;
    }

    /* make sure ot says 'start update response' */
    if (rsp[0] != TYPE_TCP_ACK || rsp[3] != TYPE_RESPONSE ||
        rsp[4] != COMMAND_START_UPDATE) {
        debug("Not expected ack");
        return LC_ERROR;
    }
    cb(LC_CB_STAGE_INITIALIZE_UPDATE, cb_count++, 3, 4,
       LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

    /* write update-header */
    debug("UPDATE_HEADER");
    uint32_t nlen = len;
    unsigned char *size_ptr = (unsigned char *)&nlen;
    for (int i = 0; i < 4; i++) {
        cmd[i] = size_ptr[i];
    }
    cmd[4] = 0x04;
    if ((err = TCPSendAndCheck(COMMAND_WRITE_UPDATE_HEADER, 5, cmd))) {
        return err;
    }
    cb(LC_CB_STAGE_INITIALIZE_UPDATE, cb_count++, 4, 4,
       LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);
    cb_count = 0;

    /* write data */
    debug("UPDATE_DATA");
    int pkt_len;
    int tlen = len;
    uint8_t *wr_ptr = const_cast<uint8_t*>(wr);
    while (tlen) {
        pkt_len = 58;
        if (tlen < pkt_len) {
            pkt_len = tlen;
        }
        tlen -= pkt_len;

        debug("DATA %d, sending %d bytes, %d bytes left", cb_count,
              pkt_len, tlen);

        if ((err = TCPSendAndCheck(COMMAND_WRITE_UPDATE_DATA, pkt_len,
            wr_ptr, true))) {
            return err;
        }
        wr_ptr += pkt_len;

        if (cb) {
            cb(LC_CB_STAGE_WRITE_CONFIG, cb_count++, (int)(wr_ptr - wr), len,
               LC_CB_COUNTER_TYPE_BYTES, cb_arg, NULL);
        }
    }

    /* write update-done */
    cb_count = 0;
    cb(LC_CB_STAGE_FINALIZE_UPDATE, cb_count++, 0, 6, LC_CB_COUNTER_TYPE_STEPS,
       cb_arg, NULL);
    debug("UPDATE_DATA_DONE");
    if ((err = TCPSendAndCheck(COMMAND_WRITE_UPDATE_DATA_DONE))) {
        return err;
    }
    cb(LC_CB_STAGE_FINALIZE_UPDATE, cb_count++, 1, 6, LC_CB_COUNTER_TYPE_STEPS,
       cb_arg, NULL);

    /* Funky ACK exchange - part 1 */
    debug("FUNKY-ACK");
    if ((err = TCP_Ack(false, false))) {
        debug("Failed to send funky-ack");
        return LC_ERROR_WRITE;
    }

    if ((err = TCP_Read(status, rlen, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR_READ;
    }

    /* Funky ACK exchange - part 2 */
    debug("FUNKY-ACK");
    if ((err = TCP_Ack(false, false))) {
        debug("Failed to send funky-ack");
        return LC_ERROR_WRITE;
    }

    if ((err = TCP_Read(status, rlen, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR_READ;
    }
    cb(LC_CB_STAGE_FINALIZE_UPDATE, cb_count++, 2, 6, LC_CB_COUNTER_TYPE_STEPS,
       cb_arg, NULL);

    /* send get-cheksum */
    debug("GET_CHECKSUM");
    cmd[0] = 0xFF;
    cmd[1] = 0xFF;
    cmd[2] = 0x04;
    if ((err = TCPSendAndCheck(COMMAND_GET_UPDATE_CHECKSUM, 3, cmd))) {
        return err;
    }
    cb(LC_CB_STAGE_FINALIZE_UPDATE, cb_count++, 3, 6, LC_CB_COUNTER_TYPE_STEPS,
       cb_arg, NULL);

    /* send finish-update */
    debug("FINISH_UPDATE");
    cmd[0] = 0x01;
    cmd[1] = 0x04;
    /*
     * OK, this is a bit weird. We will eventually get a response to the
     * FINISH_UPDATE, but first we get an "empty" ack, do another funky-ack
     * exchange, and then we'll get the real response to this FINISH_UPDATE.
     */
    if ((err = TCPSendAndCheck(COMMAND_FINISH_UPDATE, 2, cmd, true))) {
        return err;
    }
    cb(LC_CB_STAGE_FINALIZE_UPDATE, cb_count++, 4, 6, LC_CB_COUNTER_TYPE_STEPS,
       cb_arg, NULL);

    /* Funky ACK exchange */
    debug("FUNKY-ACK");
    if ((err = TCP_Ack(false, false))) {
        debug("Failed to send funky-ack");
        return LC_ERROR_WRITE;
    }

    /* Read their empty ack */
    if ((err = TCP_Read(status, rlen, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR_READ;
    }
    if (rsp[0] != TYPE_TCP_ACK) {
        debug("Failed to read ack");
        return LC_ERROR;
    }

    /* And then we should have the response we want. */
    if ((err = TCP_Read(status, rlen, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR_READ;
    }

    /* make sure we got an ack */
    if (rsp[0] != TYPE_TCP_ACK || rsp[3] != TYPE_RESPONSE ||
        rsp[4] != COMMAND_FINISH_UPDATE) {
        debug("Failed to read finish-update ack");
        return LC_ERROR;
    }
    cb(LC_CB_STAGE_FINALIZE_UPDATE, cb_count++, 5, 6, LC_CB_COUNTER_TYPE_STEPS,
       cb_arg, NULL);

    /* FIN-ACK */
    debug("FIN-ACK");
    if ((err = TCP_Ack(false, true))) {
        debug("Failed to send fin-ack");
        return LC_ERROR_WRITE;
    }

    if ((err = TCP_Read(status, rlen, rsp))) {
        debug("Failed to read from remote");
        return LC_ERROR_READ;
    }

    /* Make sure we got an ack */
    if (rsp[0] != (TYPE_TCP_ACK | TYPE_TCP_FIN)) {
        debug("Failed to read finish-update ack");
        return LC_ERROR;
    }

    if ((err = TCP_Ack(true, false))) {
        debug("Failed to ack the ack of our fin-ack");
        return LC_ERROR_WRITE;
    }
    cb(LC_CB_STAGE_FINALIZE_UPDATE, cb_count++, 6, 6, LC_CB_COUNTER_TYPE_STEPS,
       cb_arg, NULL);

    /*
     * Official traces seem to show a final ack to the above ack, but for us
     * it never comes... so we don't bother trying to read it.
     */

    /* Return TCP state to initial conditions */
    SYN_ACKED = false;

    return 0;
}

int CRemoteZ_HID::LearnIR(uint32_t *freq, uint32_t **ir_signal,
                          uint32_t *ir_signal_length, lc_callback cb,
                          void *cb_arg, uint32_t cb_stage)
{
    return LC_ERROR_UNSUPP;
}

int CRemoteZ_Base::ReadFile(const char *filename, uint8_t *rd,
                            const uint32_t rdlen, uint32_t *data_read,
                            uint8_t start_seq, lc_callback cb, void *cb_arg,
                            uint32_t cb_stage)
{
    return LC_ERROR_UNSUPP;
}

int CRemoteZ_Base::WriteFile(const char *filename, uint8_t *wr,
                             const uint32_t wrlen)
{
    return LC_ERROR_UNSUPP;
}
