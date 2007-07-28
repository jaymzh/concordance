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
 */

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
	string			serial[3];
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
	int				utc_offset;
	string			timezone;
};


void setup_ri_pointers(TRemoteInfo &ri);
void make_serial(uint8_t *ser, TRemoteInfo &ri);

class CRemoteBase
{
public:
	virtual int Reset(uint8_t kind)=0;
	virtual int GetIdentity(TRemoteInfo &ri)=0;

	virtual int ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd, unsigned int protocol, bool verify=false)=0;
	virtual int InvalidateFlash(void)=0;
	virtual int EraseFlash(uint32_t addr, uint32_t len, const TRemoteInfo &ri)=0;
	virtual int WriteFlash(uint32_t addr, const uint32_t len, const uint8_t *wr, unsigned int protocol)=0;

	virtual int GetTime(const TRemoteInfo &ri, THarmonyTime &ht)=0;
	virtual int SetTime(const TRemoteInfo &ri, const THarmonyTime &ht)=0;

	virtual int LearnIR(string *learn_string=NULL)=0;
};

class CRemote : public CRemoteBase
{
private:
	int ReadMiscByte(uint8_t addr, unsigned int len, uint8_t kind, uint8_t *rd);
	int ReadMiscWord(uint16_t addr, unsigned int len, uint8_t kind, uint16_t *rd);
	int WriteMiscByte(uint8_t addr, unsigned int len, uint8_t kind, uint8_t *wr);
	int WriteMiscWord(uint16_t addr, unsigned int len, uint8_t kind, uint16_t *wr);

public:
	int Reset(uint8_t kind);
	int GetIdentity(TRemoteInfo &ri);

	int ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd, unsigned int protocol, bool verify=false);
	int InvalidateFlash(void);
	int EraseFlash(uint32_t addr, uint32_t len, const TRemoteInfo &ri);
	int WriteFlash(uint32_t addr, const uint32_t len, const uint8_t *wr, unsigned int protocol);

	int GetTime(const TRemoteInfo &ri, THarmonyTime &ht);
	int SetTime(const TRemoteInfo &ri, const THarmonyTime &ht);

	int LearnIR(string *learn_string=NULL);
};

class CRemoteZ_Base : public CRemoteBase
{
protected:
	virtual int UDP_Write(uint8_t typ, uint8_t cmd, unsigned int len=0, uint8_t *data=NULL)=0;
	virtual int UDP_Read(uint8_t &status, unsigned int &len, uint8_t *data)=0;
	virtual int TCP_Write(uint8_t typ, uint8_t cmd, unsigned int len=0, uint8_t *data=NULL)=0;
	virtual int TCP_Read(uint8_t &status, unsigned int &len, uint8_t *data)=0;

public:
	int Reset(uint8_t kind);
	int GetIdentity(TRemoteInfo &ri);

	int ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd, unsigned int protocol, bool verify=false);
	int InvalidateFlash(void);
	int EraseFlash(uint32_t addr, uint32_t len, const TRemoteInfo &ri);
	int WriteFlash(uint32_t addr, const uint32_t len, const uint8_t *wr, unsigned int protocol);

	int GetTime(const TRemoteInfo &ri, THarmonyTime &ht);
	int SetTime(const TRemoteInfo &ri, const THarmonyTime &ht);

	int LearnIR(string *learn_string=NULL);
};

class CRemoteZ_HID : public CRemoteZ_Base	// 890, 890Pro, AVL-300, RF Extender
{
private:
	uint8_t m_tx_seq;
	uint8_t m_rx_seq;

protected:
	int UDP_Write(uint8_t typ, uint8_t cmd, unsigned int len=0, uint8_t *data=NULL);
	int UDP_Read(uint8_t &status, unsigned int &len, uint8_t *data);
	int TCP_Write(uint8_t typ, uint8_t cmd, unsigned int len=0, uint8_t *data=NULL);
	int TCP_Read(uint8_t &status, unsigned int &len, uint8_t *data);
};

class CRemoteZ_TCP : public CRemoteZ_Base	// 1000, 1000i
{
protected:
	int UDP_Write(uint8_t typ, uint8_t cmd, unsigned int len=0, uint8_t *data=NULL);
	int UDP_Read(uint8_t &status, unsigned int &len, uint8_t *data);
	int TCP_Write(uint8_t typ, uint8_t cmd, unsigned int len=0, uint8_t *data=NULL);
	int TCP_Read(uint8_t &status, unsigned int &len, uint8_t *data);
};
