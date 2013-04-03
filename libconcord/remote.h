/*
 * vi: formatoptions+=tc textwidth=80 tabstop=8 shiftwidth=8 noexpandtab:
 *
 * $Id$
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
 * (C) Copyright Phil Dibowitz 2007
 */

#ifndef REMOTE_H
#define REMOTE_H

#include "lc_internal.h"
#include "libconcord.h"

#define SERIAL_SIZE 48
#define FIRMWARE_MAX_SIZE 64*1024
/* Largest packet size for HID-UDP is 4 bytes (header) + 64 bytes (data) */
#define HID_UDP_MAX_PACKET_SIZE 68
#define HID_UDP_HDR_SIZE 2
#define HID_TCP_HDR_SIZE 4
/* Largest packet size for usbnet is the COMMAND_WRITE_UPDATE_DATA
   which is 1 (num params) + 3 (3 parameter size bytes) + 1 (param 1)
   + 1024 (param 2) + 4 (param 3) = 1033. */
#define USBNET_MAX_PACKET_SIZE 1033

/*
 * limits for IR signal learning, stop when any is reached:
 * timeouts in milliseconds, length in number of mark/space durations
 */
#define IR_LEARN_START_TIMEOUT   5000
#define MAX_IR_SIGNAL_DURATION   5000
#define IR_LEARN_DONE_TIMEOUT     500
#define MAX_IR_SIGNAL_LENGTH     1000

/*
 * WARNING: Do not change this!
 * These values have special meaning to the Z-Wave remotes
 */
enum TRegion {
    REGION_BOOTLOADER=0,		// 0
    REGION_SAFEMODE,			// 1
    REGION_APPLICATION,			// 2
    REGION_EMBEDDING_CONFIG,	// 3
    REGION_USER_CONFIG,			// 4
    REGION_PIC_REGION,			// 5
    REGION_SPARE1,				// 6
    REGION_ZWAVE,				// 7
    REGION_BLASTER_CONFIG,		// 8
    REGION_STATE_VARIABLE,		// 9
    REGION_LOGGING_REGION,		// 10
    REGION_SPARE2,				// 11
    REGION_SPARE3,				// 12
    REGION_SPARE4,				// 13
    REGION_SPARE5,				// 14
    REGION_SPARE6				// 15
};

struct TModel {
	const char	*mfg;
	const char	*model;
	const char	*code_name;
};

struct TFlash {
	uint8_t			mfg;
	uint8_t			id;
	uint32_t		size;
	unsigned int	bits;
	const uint32_t	*sectors;
	const char		*part;
};

#define SERIAL_LOCATION_EEPROM 1
#define SERIAL_LOCATION_FLASH  2
struct TArchInfo {
	int		serial_location;
	uint32_t	serial_address;
	uint32_t	flash_base;
	uint32_t	firmware_base;
	uint32_t	config_base;
	uint32_t	firmware_update_base;
	uint32_t	firmware_4847_offset;
	uint32_t	cookie;
	uint32_t	cookie_size;
	uint32_t	end_vector;
	const char	*micro;
	uint32_t	flash_size;
	uint32_t	ram_size;
	uint32_t	eeprom_size;
	const char	*usb;
};

struct TRemoteInfo {
	uint16_t		hw_ver_major;
	uint16_t		hw_ver_minor;
	uint16_t		hw_ver_micro;
	uint16_t		fw_ver_major;
	uint16_t		fw_ver_minor;
	uint8_t			fw_type;
	uint8_t			flash_mfg;
	uint8_t			flash_id;
	const TFlash	*flash;
	uint16_t		architecture;
	const TArchInfo	*arch;
	uint8_t			skin;
	uint8_t			protocol;
	const TModel	*model;
	char			*serial1;
	char			*serial2;
	char			*serial3;
	bool			valid_config;
	uint32_t		config_bytes_used;
	uint32_t		max_config_size;
	/* usbnet only from here down */
	uint8_t			num_regions;
	uint8_t			*region_ids;
	char			**region_versions;
	uint32_t		home_id;
	uint8_t			node_id;
	char			*tid;
	char			*xml_user_rf_setting;
};

struct THarmonyTime {
	unsigned int	second;
	unsigned int	minute;
	unsigned int	hour;
	unsigned int	dow;
	unsigned int	day;
	unsigned int	month;
	unsigned int	year;
	int			utc_offset;
	string			timezone;
};


void setup_ri_pointers(TRemoteInfo &ri);
void make_serial(uint8_t *ser, TRemoteInfo &ri);
int LearnIRInnerLoop(uint32_t *freq, uint32_t **ir_signal,
	uint32_t *ir_signal_length, uint8_t seq);

class CRemoteBase			// Base class for all remotes
{
public:
	CRemoteBase() {};
	virtual ~CRemoteBase() {};
	virtual int Reset(uint8_t kind)=0;
	virtual int GetIdentity(TRemoteInfo &ri, struct THIDINFO &hid,
		lc_callback cb=NULL, void *cb_arg=NULL,
		uint32_t cb_stage=0)=0;

	virtual int ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd,
		unsigned int protocol, bool verify=false,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0)=0;
	virtual int InvalidateFlash(lc_callback cb=NULL, void *cb_arg=NULL,
		uint32_t cb_stage=0)=0;
	virtual int EraseFlash(uint32_t addr, uint32_t len,
		const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0)=0;
	virtual int WriteFlash(uint32_t addr, const uint32_t len,
		const uint8_t *wr, unsigned int protocol, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0)=0;
	virtual int WriteRam(uint32_t addr, const uint32_t len, uint8_t *wr)=0;
	virtual int ReadRam(uint32_t addr, const uint32_t len, uint8_t *rd)=0;
	virtual int PrepFirmware(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0) = 0;
	virtual int FinishFirmware(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0) = 0;
	virtual int PrepConfig(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0)=0;
	virtual int FinishConfig(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0)=0;
	virtual int UpdateConfig(const uint32_t len,
		const uint8_t *wr, lc_callback cb, void *cb_arg,
		uint32_t cb_stage, uint32_t xml_size=0,
		uint8_t *xml=NULL)=0;
	virtual int GetTime(const TRemoteInfo &ri, THarmonyTime &ht)=0;
	virtual int SetTime(const TRemoteInfo &ri, const THarmonyTime &ht,
		lc_callback cb=NULL, void *cb_arg=NULL,
		uint32_t cb_stage=0)=0;
	virtual int LearnIR(uint32_t *freq, uint32_t **ir_signal,
		uint32_t *ir_signal_length, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0)=0;
	virtual int IsZRemote()=0;
	virtual int IsUSBNet()=0;
	virtual int IsMHRemote()=0;
};

class CRemote : public CRemoteBase	// All non-Z-Wave remotes
{
private:
	int ReadMiscByte(uint8_t addr, uint32_t len, uint8_t kind,
		uint8_t *rd);
	int ReadMiscWord(uint16_t addr, uint32_t len, uint8_t kind,
		uint16_t *rd);
	int WriteMiscByte(uint8_t addr, uint32_t len, uint8_t kind,
		uint8_t *wr);
	int WriteMiscWord(uint16_t addr, uint32_t len, uint8_t kind,
		uint16_t *wr);

public:
	CRemote() {};
	virtual ~CRemote() {};
	int Reset(uint8_t kind);
	int GetIdentity(struct TRemoteInfo &ri, struct THIDINFO &hid,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0);

	int ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd,
		unsigned int protocol, bool verify=false,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0);
	int InvalidateFlash(lc_callback cb=NULL, void *cb_arg=NULL,
		uint32_t cb_stage=0);
	int EraseFlash(uint32_t addr, uint32_t len, const TRemoteInfo &ri,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0);
	int WriteFlash(uint32_t addr, const uint32_t len, const uint8_t *wr,
		unsigned int protocol, lc_callback cb=NULL, void *cb_arg=NULL,
		uint32_t cb_stage=0);
	int WriteRam(uint32_t addr, const uint32_t len, uint8_t *wr);
	int ReadRam(uint32_t addr, const uint32_t len, uint8_t *rd);
	int PrepFirmware(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	int FinishFirmware(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	int PrepConfig(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	int FinishConfig(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	virtual int UpdateConfig(const uint32_t len,
		const uint8_t *wr, lc_callback cb, void *cb_arg,
		uint32_t cb_stage=0, uint32_t xml_size=0,
		uint8_t *xml=NULL) {};

	int GetTime(const TRemoteInfo &ri, THarmonyTime &ht);
	int SetTime(const TRemoteInfo &ri, const THarmonyTime &ht,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0);

	int LearnIR(uint32_t *freq, uint32_t **ir_signal, 
		uint32_t *ir_signal_length, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	int IsZRemote() {return false;}
	int IsUSBNet() {return false;}
	int IsMHRemote() {return false;}
};

// Base class for all Z-Wave remotes
class CRemoteZ_Base : public CRemoteBase
{
protected:
	struct TParamList {
		uint32_t count;
		uint8_t *p[32];
	};
	virtual int Write(uint8_t typ, uint8_t cmd, uint32_t len=0,
		uint8_t *data=NULL)=0;
	virtual int Read(uint8_t &status, uint32_t &len, uint8_t *data)=0;
	virtual int ParseParams(uint32_t len, uint8_t *data,
		TParamList &pl)=0;
	virtual uint16_t GetWord(uint8_t *x)=0;
	virtual int ReadRegion(uint8_t region, uint32_t &len, uint8_t *rd,
		lc_callback cb, void *cb_arg, uint32_t cb_stage)=0;

public:
	CRemoteZ_Base() {};
	virtual ~CRemoteZ_Base() {};
	int Reset(uint8_t kind);
	int GetIdentity(struct TRemoteInfo &ri, struct THIDINFO &hid,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0);

	int ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd,
		unsigned int protocol, bool verify=false,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0);
	int InvalidateFlash(lc_callback cb=NULL, void *cb_arg=NULL,
		uint32_t cb_stage=0);
	int EraseFlash(uint32_t addr, uint32_t len, const TRemoteInfo &ri,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0);
	int WriteFlash(uint32_t addr, const uint32_t len, const uint8_t *wr,
		unsigned int protocol, lc_callback cb=NULL, void *cb_arg=NULL,
		uint32_t cb_stage=0);
	int WriteRam(uint32_t addr, const uint32_t len, uint8_t *wr);
	int ReadRam(uint32_t addr, const uint32_t len, uint8_t *rd);
	int PrepFirmware(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	int FinishFirmware(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	int PrepConfig(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	int FinishConfig(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);

	int GetTime(const TRemoteInfo &ri, THarmonyTime &ht);
	int SetTime(const TRemoteInfo &ri, const THarmonyTime &ht,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0);

	int LearnIR(uint32_t *freq, uint32_t **ir_signal,
		uint32_t *ir_signal_length, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	int IsZRemote() {return true;}
	int IsMHRemote() {return false;}
};

// 890, 890Pro, AVL-300, RF Extender
class CRemoteZ_HID : public CRemoteZ_Base
{
private:
	uint8_t m_tx_seq;
	uint8_t m_rx_seq;
	int UDP_Write(uint8_t typ, uint8_t cmd, uint32_t len=0,
		uint8_t *data=NULL);
	int UDP_Read(uint8_t &status, uint32_t &len, uint8_t *data);
	int TCP_Write(uint8_t typ, uint8_t cmd, uint32_t len=0,
		uint8_t *data=NULL);
	int TCP_Read(uint8_t &status, uint32_t &len, uint8_t *data);

protected:
	int TCPSendAndCheck(uint8_t cmd, uint32_t len=0, uint8_t *data=NULL,
		bool ackonly=false);
	virtual int Write(uint8_t typ, uint8_t cmd, uint32_t len=0,
		uint8_t *data=NULL);
	virtual int Read(uint8_t &status, uint32_t &len, uint8_t *data);
	virtual int ParseParams(uint32_t len, uint8_t *data,
		TParamList &pl);
	virtual uint16_t GetWord(uint8_t *x) { return x[1]<<8 | x[0]; };

public:
	CRemoteZ_HID() {};
	virtual ~CRemoteZ_HID() {};
	int UpdateConfig(const uint32_t len, const uint8_t *wr,
		lc_callback cb, void *cb_arg, uint32_t cb_stage,
		uint32_t xml_size=0, uint8_t *xml=NULL);
	int IsUSBNet() {return false;}
	virtual int ReadRegion(uint8_t region, uint32_t &len, uint8_t *rd,
		lc_callback cb, void *cb_arg, uint32_t cb_stage);
};

// 1000, 1000i
class CRemoteZ_USBNET : public CRemoteZ_Base
{
protected:
	int TCPSendAndCheck(uint8_t cmd, uint32_t len=0, uint8_t *data=NULL);
	virtual int Write(uint8_t typ, uint8_t cmd, uint32_t len=0,
		uint8_t *data=NULL);
	virtual int Read(uint8_t &status, uint32_t &len, uint8_t *data);
	virtual int ParseParams(uint32_t len, uint8_t *data,
		TParamList &pl);
	virtual uint16_t GetWord(uint8_t *x) { return x[0]<<8 | x[1]; };
	virtual uint32_t GetWord32(uint8_t *x) { return x[0]<<24 | x[1]<<16
		| x[2]<<8 | x[3]; };
	virtual int ReadRegion(uint8_t region, uint32_t &len, uint8_t *rd,
		lc_callback cb, void *cb_arg, uint32_t cb_stage);

public:
	CRemoteZ_USBNET() {};
	virtual ~CRemoteZ_USBNET() {};
	int UpdateConfig(const uint32_t len, const uint8_t *wr,
		lc_callback cb, void *cb_arg, uint32_t cb_stage,
		uint32_t xml_size=0, uint8_t *xml=NULL);
	int GetTime(const TRemoteInfo &ri, THarmonyTime &ht);
	int SetTime(const TRemoteInfo &ri, const THarmonyTime &ht,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0);
	int IsUSBNet() {return true;}
};

class CRemoteMH : public CRemoteBase	// "My Harmony" remotes
{
private:
	int ReadMiscByte(uint8_t addr, uint32_t len, uint8_t kind,
		uint8_t *rd);
	int ReadMiscWord(uint16_t addr, uint32_t len, uint8_t kind,
		uint16_t *rd);
	int WriteMiscByte(uint8_t addr, uint32_t len, uint8_t kind,
		uint8_t *wr);
	int WriteMiscWord(uint16_t addr, uint32_t len, uint8_t kind,
		uint16_t *wr);

public:
	CRemoteMH() {};
	virtual ~CRemoteMH() {};
	int Reset(uint8_t kind);
	int GetIdentity(struct TRemoteInfo &ri, struct THIDINFO &hid,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0);

	int ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd,
		unsigned int protocol, bool verify=false,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0);
	int InvalidateFlash(lc_callback cb=NULL, void *cb_arg=NULL,
		uint32_t cb_stage=0);
	int EraseFlash(uint32_t addr, uint32_t len, const TRemoteInfo &ri,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0);
	int WriteFlash(uint32_t addr, const uint32_t len, const uint8_t *wr,
		unsigned int protocol, lc_callback cb=NULL, void *cb_arg=NULL,
		uint32_t cb_stage=0);
	int WriteRam(uint32_t addr, const uint32_t len, uint8_t *wr);
	int ReadRam(uint32_t addr, const uint32_t len, uint8_t *rd);
	int PrepFirmware(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	int FinishFirmware(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	int PrepConfig(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	int FinishConfig(const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	virtual int UpdateConfig(const uint32_t len,
		const uint8_t *wr, lc_callback cb, void *cb_arg,
		uint32_t cb_stage=0) {};
	virtual int UpdateConfig(const uint32_t len, const uint8_t *wr,
		lc_callback cb,	void *cb_arg, uint32_t cb_stage=0,
		uint32_t xml_size=0, uint8_t *xml=NULL);

	int GetTime(const TRemoteInfo &ri, THarmonyTime &ht);
	int SetTime(const TRemoteInfo &ri, const THarmonyTime &ht,
		lc_callback cb=NULL, void *cb_arg=NULL, uint32_t cb_stage=0);

	int LearnIR(uint32_t *freq, uint32_t **ir_signal, 
		uint32_t *ir_signal_length, lc_callback cb=NULL,
		void *cb_arg=NULL, uint32_t cb_stage=0);
	int IsZRemote() {return false;}
	int IsUSBNet() {return false;}
	int IsMHRemote() {return true;}
};

#endif //REMOTE_H
