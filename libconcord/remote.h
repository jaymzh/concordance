/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  (C) Copyright Kevin Timmerman 2007
 *  (C) Copyright Phil Dibowitz 2007
 */

#ifndef REMOTE_H
#define REMOTE_H

#define FLASH_EEPROM_ADDR 0x10
#define FLASH_SERIAL_ADDR 0x000110
#define FLASH_SIZE 48
#define MAX_PULSE_COUNT 1000

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

struct TArchInfo {
	uint32_t	flash_base;
	uint32_t	firmware_base;
	uint32_t	config_base;
	uint32_t	firmware_update_base;
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

class CRemoteBase			// Base class for all remotes
{
public:
	CRemoteBase() {};
	virtual ~CRemoteBase() {};
	virtual int Reset(uint8_t kind)=0;
	virtual int GetIdentity(TRemoteInfo &ri, struct THIDINFO &hid,
		lc_callback cb=NULL, void *cb_arg=NULL)=0;

	virtual int ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd,
		unsigned int protocol, bool verify=false,
		lc_callback cb=NULL, void *cb_arg=NULL)=0;
	virtual int InvalidateFlash(void)=0;
	virtual int EraseFlash(uint32_t addr, uint32_t len,
		const TRemoteInfo &ri, lc_callback cb=NULL,
		void *cb_arg=NULL)=0;
	virtual int WriteFlash(uint32_t addr, const uint32_t len,
		const uint8_t *wr, unsigned int protocol,
		lc_callback cb=NULL, void *cb_arg=NULL)=0;
	virtual int WriteRam(uint32_t addr, const uint32_t len, uint8_t *wr)=0;
	virtual int ReadRam(uint32_t addr, const uint32_t len, uint8_t *rd)=0;

	virtual int GetTime(const TRemoteInfo &ri, THarmonyTime &ht)=0;
	virtual int SetTime(const TRemoteInfo &ri, const THarmonyTime &ht)=0;

	virtual int LearnIR(string *learn_string=NULL)=0;
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
		lc_callback cb=NULL, void *cb_arg=NULL);

	int ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd,
		unsigned int protocol, bool verify=false,
		lc_callback cb=0, void *cb_arg=NULL);
	int InvalidateFlash(void);
	int EraseFlash(uint32_t addr, uint32_t len, const TRemoteInfo &ri,
		lc_callback cb=NULL, void *cb_arg=NULL);
	int WriteFlash(uint32_t addr, const uint32_t len, const uint8_t *wr,
		unsigned int protocol, lc_callback cb=NULL,
		void *cb_arg=NULL);
	int WriteRam(uint32_t addr, const uint32_t len, uint8_t *wr);
	int ReadRam(uint32_t addr, const uint32_t len, uint8_t *rd);

	int GetTime(const TRemoteInfo &ri, THarmonyTime &ht);
	int SetTime(const TRemoteInfo &ri, const THarmonyTime &ht);

	int LearnIR(string *learn_string=NULL);
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

public:
	CRemoteZ_Base() {};
	virtual ~CRemoteZ_Base() {};
	int Reset(uint8_t kind);
	int GetIdentity(struct TRemoteInfo &ri, struct THIDINFO &hid,
		lc_callback cb=NULL, void *cb_arg=NULL);

	int ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd,
		unsigned int protocol, bool verify=false,
		lc_callback cb=0, void *cb_arg=NULL);
	int InvalidateFlash(void);
	int EraseFlash(uint32_t addr, uint32_t len, const TRemoteInfo &ri,
		lc_callback cb=NULL, void *cb_arg=NULL);
	int WriteFlash(uint32_t addr, const uint32_t len, const uint8_t *wr,
		unsigned int protocol, lc_callback cb=NULL,
		void *cb_arg=NULL);
	int WriteRam(uint32_t addr, const uint32_t len, uint8_t *wr);
	int ReadRam(uint32_t addr, const uint32_t len, uint8_t *rd);

	int GetTime(const TRemoteInfo &ri, THarmonyTime &ht);
	int SetTime(const TRemoteInfo &ri, const THarmonyTime &ht);

	int LearnIR(string *learn_string=NULL);
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
	virtual int Write(uint8_t typ, uint8_t cmd, uint32_t len=0,
		uint8_t *data=NULL);
	virtual int Read(uint8_t &status, uint32_t &len, uint8_t *data);
	virtual int ParseParams(uint32_t len, uint8_t *data,
		TParamList &pl);
	virtual uint16_t GetWord(uint8_t *x) { return x[1]<<8 | x[0]; };

public:
	CRemoteZ_HID() {};
	virtual ~CRemoteZ_HID() {};
};

// 1000, 1000i
class CRemoteZ_TCP : public CRemoteZ_Base
{
protected:
	virtual int Write(uint8_t typ, uint8_t cmd, uint32_t len=0,
		uint8_t *data=NULL);
	virtual int Read(uint8_t &status, uint32_t &len, uint8_t *data);
	virtual int ParseParams(uint32_t len, uint8_t *data,
		TParamList &pl);
	virtual uint16_t GetWord(uint8_t *x) { return x[0]<<8 | x[1]; };

public:
	CRemoteZ_TCP() {};
	virtual ~CRemoteZ_TCP() {};
};

#endif //REMOTE_H
