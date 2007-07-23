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
	uint8_t			fw_ver;
	uint8_t			fw_type;
	uint8_t			hw_ver;
	uint8_t			flash_mfg;
	uint8_t			flash_id;
	const TFlash	*flash;
	uint8_t			architecture;
	const TArchInfo	*arch;
	uint8_t			skin;
	uint8_t			protocol;
	const TModel	*model;
	string			serial[3];
	bool			valid_config;
	uint32_t		config_bytes_used;
	uint32_t		max_config_size;
};

int ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd, unsigned int protocol, bool verify=false);
int EraseFlash(uint32_t addr, uint32_t len, const uint32_t *sectors);
int WriteFlash(uint32_t addr, const uint32_t len, const uint8_t *wr, unsigned int protocol);
int LearnIR(string *learn_string=NULL);
