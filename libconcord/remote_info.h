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
 * (C) Copyright Phil Dibowitz 2008
 */

#ifndef REMOTE_INFO_H
#define REMOTE_INFO_H

static const char *MFG_UNK="Unknown";
static const char *MFG_HAR="Logitech";
static const char *MFG_COOL="Logicool";
static const char *MFG_HK="Harman Kardon";
static const char *MFG_MON="Monster";

static const TModel ModelList[]={
	{ MFG_UNK,	"0",				NULL },				// 0
	{ MFG_UNK,	"1",				NULL },
	{ MFG_HAR,	"Harmony 745",		NULL },
	{ MFG_HAR,	"Harmony 768",		NULL },
	{ MFG_UNK,	"4",				NULL },
	{ MFG_UNK,	"5",				NULL },
	{ MFG_UNK,	"6",				NULL },
	{ MFG_HAR,	"Harmony 748",		NULL },
	{ MFG_UNK,	"8",				NULL },
	{ MFG_HAR,	"Harmony 659",		NULL },
	{ MFG_HAR,	"Harmony 688",		NULL },				// 10
	{ MFG_HAR,	"Harmony 655",		NULL },
	{ MFG_HAR,	"Harmony 676",		NULL },
	{ MFG_HAR,	"Harmony 628",		NULL },
	{ MFG_HAR,	"Harmony 680",		NULL },
	{ MFG_HAR,	"Harmony 880",		"Espresso" },
	{ MFG_HAR,	"Harmony 675",		NULL },
	{ MFG_HAR,	"Harmony 885",		"Espresso" },
	{ MFG_HAR,	"Harmony 520",		"Mocha Decaf" },
	{ MFG_HAR,	"Harmony 890",		"Cappuccino" },
	{ MFG_HAR,	"Harmony 891",		"Whisky" },			// 20
	{ MFG_HAR,	"Harmony 892",		"Sugar" },
	{ MFG_HAR,	"Harmony 525",		"Mocha Decaf" },
	{ MFG_HAR,	"Harmony 895",		"Cappuccino" },
	{ MFG_HAR,	"Harmony 896",		"Whisky" },
	{ MFG_HAR,	"Harmony 897",		"Sugar" },
	{ MFG_MON,	"AVL-300",			"Godzilla" },
	{ MFG_MON,	"AVL-300",			"Whisky" },
	{ MFG_MON,	"AVL-300",			"Sugar" },
	{ MFG_MON,	"AVL-300",			"Godzilla" },
	{ MFG_MON,	"AVL-300",			"Whisky" },			// 30
	{ MFG_MON,	"AVL-300",			"Sugar" },
	{ MFG_MON,	"AVL-200",			"Godzilla" },
	{ MFG_MON,	"AVL-200",			"Godzilla" },
	{ MFG_HK,	"TC-30",			NULL },
	{ MFG_HK,	"TC-30",			NULL },
	{ MFG_HAR,	"Harmony XBOX360", "Mountain Dew" },
	{ MFG_MON,	"AV-100",			NULL },
	{ MFG_MON,	"AV-100",			NULL },
	{ MFG_HAR,	"Harmony 880 Pro", "Espresso" },
	{ MFG_HAR,	"Harmony 890 Pro", "Cappuccino" },		// 40
	{ MFG_HAR,	"Harmony 550",		"Mocha Grande" },
	{ MFG_HK,	"TC-30",			NULL },
	{ MFG_HK,	"TC-30",			NULL },
	{ MFG_HAR,	"Harmony 720",		"Corona" },
	{ MFG_HAR,	"Harmony 785",		"Corona" },
	{ MFG_HAR,	"Harmony 522",		"Mocha Decaf" },
	{ MFG_COOL,	"Harmony 882",		"Espresso" },
	{ MFG_HAR,	"Harmony 555",		"Mocha Grande" },
	{ MFG_HAR,	"Harmony 1000",		"Cognac" },
	{ MFG_HAR,	"Harmony 670",		NULL },				// 50
	{ MFG_COOL,	"Harmony 552",		"Mocha Grande" },
	{ MFG_HAR,	"Harmony 1000i",	"Cognac" },
	{ MFG_UNK,	"Unknown",			NULL }
};

static const unsigned int max_model=sizeof(ModelList)/sizeof(TModel)-1;

// 01:37 AMD 29LV008B (Bottom Boot Block)
static const uint32_t sectors1[]={ 0x004000, 0x006000, 0x008000, 0x010000, 0x020000,
	0x030000, 0x040000, 0x050000, 0x060000, 0x070000, 0x080000, 0x090000,
	0x0A0000, 0x0B0000, 0x0C0000, 0x0D0000, 0x0E0000, 0x0F0000, 0x100000, 0 };

// 01:49 AMD Am29LV160BB (Bottom Boot Block)
// 01:4C AMD Am29LV116DB (Bottom Boot Block)
static const uint32_t sectors2[]={ 0x004000, 0x006000, 0x008000, 0x010000, 0x020000,
	0x030000, 0x040000, 0x050000, 0x060000, 0x070000, 0x080000, 0x090000,
	0x0A0000, 0x0B0000, 0x0C0000, 0x0D0000, 0x0E0000, 0x0F0000, 0x100000,
	0x110000, 0x120000, 0x130000, 0x140000, 0x150000, 0x160000, 0x170000,
	0x180000, 0x190000, 0x1A0000, 0x1B0000, 0x1C0000, 0x1D0000, 0x1E0000,
	0x1F0000, 0x200000, 0 };

// ??:??:11 Saifun SA25F020
static const uint32_t sectors3[]={ 0x010000, 0x020000,	0x030000, 0x040000, 0 };

// 01:02:12 Spansion S25FL004A, 01:02:12 Spansion S25FL040Axxxxx00 (Uniform Sectors)
static const uint32_t sectors4[]={ 0x010000, 0x020000,	0x030000, 0x040000, 0x050000,
	0x060000, 0x070000, 0x080000, 0 };

static const TFlash FlashList[]={
	{ 0x01,		0x37,	1024,	8,	sectors1,	"AMD Am29LV008B" },
	{ 0x01,		0x49,	2048,	16,	sectors2,	"AMD Am29LV160BB" },
	{ 0x01,		0x4C,	2048,	8,	sectors2,	"AMD Am29LV116DB" },
	{ 0xFF,		0x11,	256,	1,	sectors3,	"25F020" },
	{ 0xFF,		0x12,	512,	1,	sectors4,	"25F040" } ,
	{ 0,		0,		0,		0,	NULL,		"" }
};

static const TArchInfo ArchList[11]={
//        fl_base,  fw_base,    config_base,    fw_up_base,	fw_4847_off,	cookie,		ck_sz,	endvec, micro,		fl_sz,	ram_sz, ee_sz, usb
// 0
	{ 0,        0,		0,		0,		0,		0,		0,	0,	"",		0,	0,	0,	"" },
// 1
	{ 0,	    0,		0,		0,		0,		0,		0,	0,	"",		0,	0,	0,	"" },
// 2 - 745
	{ 0x000000, 0,		0x006000,	0,		0,		0x03A5,		2,	2,	"PIC16LF877",	8,	368,	256,	"USBN9603" },
// 3 - 748, 768
	{ 0x000000, 0x010000,	0x020000,	0x020000,	2,		0x0369,		2,	2,	"PIC18LC801",	0,	1536,	0,	"USBN9604" },
// 4
	{ 0,	    0,		0,		0,		0,		0,		0,	0,	"",		0,	0,	0,	"" },
// 5
	{ 0,	    0,		0,		0,		0,		0,		0,	0,	"",		0,	0,	0,	"" },
// 6
	{ 0,	    0,		0,		0,		0,		0,		0,	0,	"",		0,	0,	0,	"" },
// 7 - 6xx
	{ 0x000000, 0x010000,	0x020000,	0x020000,	2,		0x4D424D42,	4,	5,	"PIC18LC801",	0,	1536,	0,	"USBN9604" },
// 8 - 880
	{ 0x000000, 0x010000,	0x020000,	0x1D0000,	4,		0x50545054,	4,	4,	"PIC18LC801",	0,	1536,	0,	"USBN9604" },
// 9 - 360, 52x, 55x
	{ 0x800000, 0x810000,	0x820000,	0x810000,	4,		0x4D434841,	4,	4,	"PIC18LF4550",	16,	2048,	256,	"Internal" },
// 10 - 890
	{ 0x000000, 0x010000,	0x020000,	0,		0,		0x1, /*hack*/ 	4,	4,	"PIC18LC801",	0,	1536,	0,	"USBN9604" },
};

#endif
