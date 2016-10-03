/*
 * Texas Instruments Keystone SerDes driver
 * Authors: WingMan Kwok <w-kwok2@ti.com>
 *
 * This is the SerDes driver for Keystone devices. This is
 * required to support gbe and 10gbe found on these devices.
 *
 * Revision History:
 *    3.3.0.2c
 *	- Full update based on CSL version 3.3.0.2c
 *
 *    1.0.0
 *	- Initial revision.
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <errno.h>
#include <common.h>
#include <linux/compat.h>
#include <asm/processor.h>
#include <asm/ti-common/keystone_serdes.h>

/*
 * All firmware file names end up here. List the firmware file names below.
 * Newest first. Search starts from the 0-th array entry until a firmware
 * file is found.
 */
static const char * const ks2_gbe_serdes_firmwares[] = {"ks2_gbe_serdes.bin"};
static const char * const ks2_xgbe_serdes_firmwares[] = {"ks2_xgbe_serdes.bin"};
static const char * const ks2_pcie_serdes_firmwares[] = {"ks2_pcie_serdes.bin"};

#define MAX_VERSION		64
#define INIT_FW_MAGIC_1		0xfaceface
#define INIT_FW_MAGIC_2		0xcafecafe

static char *compatible_init_fw_version[] = {
	"3.3.0.2c",
	NULL,
};

struct serdes_cfg_header {
	u32 magic_1;
	char version[MAX_VERSION];
	u32 magic_2;
};

struct serdes_cfg {
	u32 ofs;
	u32 msb;
	u32 lsb;
	u32 val;
};

static inline int next_set_bit(unsigned long *mask, int i, int size)
{
	int j = i;

	while (++j < size) {
		if (*mask & BIT(j))
			return j;
	}
	return j;
}

#define for_each_set_bit(bit, addr, size)			\
	for ((bit) = -1;					\
	     (bit) = next_set_bit((addr), (bit), size), (bit) < (size);)

#define usleep_range(min, max)	udelay(min)

struct firmware {
	struct serdes_cfg_header hdr;
	struct serdes_cfg *data;
	u32 size;
};

struct serdes_cfg cfg_156p25mhz_10bit_5gbps_3_3_0_2c[] = {
	{0x1ff4,	17, 16,		0x03},

	{0x1ff4,	31, 29,		0x04},

	{0x1fe0,	31, 29,		0x04},
	{0x1fe0,	15, 13,		0x04},
	{0x1fe4,	31, 29,		0x04},
	{0x1fe4,	15, 13,		0x04},
	{0x1fe8,	31, 29,		0x04},
	{0x1fe8,	15, 13,		0x04},
	{0x1fec,	31, 29,		0x04},
	{0x1fec,	15, 13,		0x04},

	{0x0000,	23, 16,		0x80},
	{0x0000,	31, 24,		0x00},
	{0x0014,	 7,  0,		0x82},
	{0x0014,	15,  8,		0x82},
	{0x0060,	 7,  0,		0x38},
	{0x0060,	15,  8,		0x24},
	{0x0060,	23, 16,		0x14},
	{0x0064,	15,  8,		0xc7},
	{0x0064,	23, 16,		0xc3},
	{0x0078,	15,  8,		0xc0},

	{0x0204,	 7,  0,		0x80},
	{0x0204,	31, 24,		0x78},
	{0x0208,	 7,  0,		0x20},
	{0x0208,	23, 16,		0x01},
	{0x020c,	31, 24,		0x02},
	{0x0210,	31, 24,		0x1b},
	{0x0214,	 7,  0,		0xb8},
	{0x0214,	15,  8,		0x6f},
	{0x0218,	 7,  0,		0xe4},
	{0x0218,	23, 16,		0x80},
	{0x0218,	31, 24,		0x7a},
	{0x02ac,	15,  8,		0x44},
	{0x022c,	15,  8,		0x08},
	{0x022c,	23, 16,		0x30},
	{0x0280,	 7,  0,		0x82},
	{0x0280,	23, 16,		0x82},
	{0x0284,	 7,  0,		0x85},
	{0x0284,	15,  8,		0x03},
	{0x0284,	23, 16,		0x0f},
	{0x0284,	31, 24,		0x1d},

	{0x0404,	 7,  0,		0x80},
	{0x0404,	31, 24,		0x78},
	{0x0408,	 7,  0,		0x20},
	{0x0408,	23, 16,		0x01},
	{0x040c,	31, 24,		0x02},
	{0x0410,	31, 24,		0x1b},
	{0x0414,	 7,  0,		0xb8},
	{0x0414,	15,  8,		0x6f},
	{0x0418,	 7,  0,		0xe4},
	{0x0418,	23, 16,		0x80},
	{0x0418,	31, 24,		0x7a},
	{0x04ac,	15,  8,		0x44},
	{0x042c,	15,  8,		0x08},
	{0x042c,	23, 16,		0x30},
	{0x0480,	 7,  0,		0x82},
	{0x0480,	23, 16,		0x82},
	{0x0484,	 7,  0,		0x85},
	{0x0484,	15,  8,		0x03},
	{0x0484,	23, 16,		0x0f},
	{0x0484,	31, 24,		0x1d},

	{0x0604,	 7,  0,		0x80},
	{0x0604,	31, 24,		0x78},
	{0x0608,	 7,  0,		0x20},
	{0x0608,	23, 16,		0x01},
	{0x060c,	31, 24,		0x02},
	{0x0610,	31, 24,		0x1b},
	{0x0614,	 7,  0,		0xb8},
	{0x0614,	15,  8,		0x6f},
	{0x0618,	 7,  0,		0xe4},
	{0x0618,	23, 16,		0x80},
	{0x0618,	31, 24,		0x7a},
	{0x06ac,	15,  8,		0x44},
	{0x062c,	15,  8,		0x08},
	{0x062c,	23, 16,		0x30},
	{0x0680,	 7,  0,		0x82},
	{0x0680,	23, 16,		0x82},
	{0x0684,	 7,  0,		0x85},
	{0x0684,	15,  8,		0x03},
	{0x0684,	23, 16,		0x0f},
	{0x0684,	31, 24,		0x1d},

	{0x0804,	 7,  0,		0x80},
	{0x0804,	31, 24,		0x78},
	{0x0808,	 7,  0,		0x20},
	{0x0808,	23, 16,		0x01},
	{0x080c,	31, 24,		0x02},
	{0x0810,	31, 24,		0x1b},
	{0x0814,	 7,  0,		0xb8},
	{0x0814,	15,  8,		0x6f},
	{0x0818,	 7,  0,		0xe4},
	{0x0818,	23, 16,		0x80},
	{0x0818,	31, 24,		0x7a},
	{0x08ac,	15,  8,		0x44},
	{0x082c,	15,  8,		0x08},
	{0x082c,	23, 16,		0x30},
	{0x0880,	 7,  0,		0x82},
	{0x0880,	23, 16,		0x82},
	{0x0884,	 7,  0,		0x85},
	{0x0884,	15,  8,		0x03},
	{0x0884,	23, 16,		0x0f},
	{0x0884,	31, 24,		0x1d},

	{0x0a00,	15,  8,		0x08},
	{0x0a08,	23, 16,		0xa2},
	{0x0a08,	31, 24,		0x38},
	{0x0a30,	15,  8,		0x8a},
	{0x0a30,	23, 16,		0x8a},
	{0x0a84,	15,  8,		0x07},
	{0x0a94,	31, 24,		0x10},
	{0x0aa0,	31, 24,		0x81},
	{0x0abc,	31, 24,		0xff},
	{0x0ac0,	 7,  0,		0x8b},
	{0x0b08,	23, 16,		0x3f},
	{0x0b08,	31, 24,		0x58},
	{0x0b0c,	 7,  0,		0x4e},
	{0x0a48,	15,  8,		0x8c},
	{0x0a48,	23, 16,		0xfd},
	{0x0a54,	 7,  0,		0x72},
	{0x0a54,	15,  8,		0xec},
	{0x0a54,	23, 16,		0x2f},
	{0x0a58,	15,  8,		0x21},
	{0x0a58,	23, 16,		0xf9},
	{0x0a58,	31, 24,		0x00},
	{0x0a5c,	 7,  0,		0x60},
	{0x0a5c,	15,  8,		0x00},
	{0x0a5c,	23, 16,		0x04},
	{0x0a5c,	31, 24,		0x00},
	{0x0a60,	 7,  0,		0x00},
	{0x0a60,	15,  8,		0x80},
	{0x0a60,	23, 16,		0x00},
	{0x0a60,	31, 24,		0x00},
	{0x0a64,	 7,  0,		0x20},
	{0x0a64,	15,  8,		0x12},
	{0x0a64,	23, 16,		0x58},
	{0x0a64,	31, 24,		0x0c},
	{0x0a68,	 7,  0,		0x02},
	{0x0a68,	15,  8,		0x06},
	{0x0a68,	23, 16,		0x3b},
	{0x0a68,	31, 24,		0xe1},
	{0x0a6c,	 7,  0,		0xc1},
	{0x0a6c,	15,  8,		0x4c},
	{0x0a6c,	23, 16,		0x07},
	{0x0a6c,	31, 24,		0xb8},
	{0x0a70,	 7,  0,		0x89},
	{0x0a70,	15,  8,		0xe9},
	{0x0a70,	23, 16,		0x02},
	{0x0a70,	31, 24,		0x3f},
	{0x0a74,	 7,  0,		0x01},
	{0x0b20,	23, 16,		0x37},
	{0x0b1c,	31, 24,		0x37},
	{0x0b20,	 7,  0,		0x5d},

	{0x0000,	 7,  0,		0x03},
	{0x0a00,	 7,  0,		0x5f},
};

struct firmware ks2_gbe_serdes = {
	.hdr = {
		.magic_1 = INIT_FW_MAGIC_1,
		.version = {'3', '.', '3', '.', '0', '.', '2', 'c'},
		.magic_2 = INIT_FW_MAGIC_2,
	},
	.data = &cfg_156p25mhz_10bit_5gbps_3_3_0_2c[0],
	.size = sizeof(cfg_156p25mhz_10bit_5gbps_3_3_0_2c),
};

struct serdes_cfg cfg_phyb_10p3125g_156p25mhz_16bit_3_3_0_2c[] = {
	{0x1ff4,	17, 16,		0x03},

	{0x1ff4,	31, 29,		0x04},
	{0x1ff4,	27, 25,		0x04},

	{0x1fe0,	31, 29,		0x04},
	{0x1fe0,	15, 13,		0x04},
	{0x1fe4,	31, 29,		0x04},
	{0x1fe4,	15, 13,		0x04},

	{0x0000,	 7,  0,		0x02},
	{0x0000,	23, 16,		0x80},
	{0x0014,	 7,  0,		0x38},
	{0x0014,	15,  8,		0x38},
	{0x0060,	 7,  0,		0x38},
	{0x0060,	15,  8,		0xe4},
	{0x0060,	23, 16,		0x44},
	{0x0060,	31, 24,		0x1c},
	{0x0064,	 7,  0,		0x00},
	{0x0064,	15,  8,		0x84},
	{0x0064,	23, 16,		0xc1},
	{0x0068,	15,  8,		0x82},
	{0x0068,	23, 16,		0x07},
	{0x0068,	31, 24,		0x17},
	{0x006c,	 7,  0,		0x14},
	{0x0078,	15,  8,		0xc0},
	{0x0000,	 7,  0,		0x03},

	{0x0c00,	 7,  0,		0x02},
	{0x0c00,	23, 16,		0x03},
	{0x0c14,	 7,  0,		0x52},
	{0x0c14,	15,  8,		0x52},
	{0x0c28,	31, 24,		0x80},
	{0x0c2c,	 7,  0,		0xf6},
	{0x0c3c,	 7,  0,		0x05},
	{0x0c3c,	15,  8,		0x04},
	{0x0c3c,	31, 24,		0x04},
	{0x0c40,	23, 16,		0x80},
	{0x0c40,	31, 24,		0xc0},
	{0x0c44,	 7,  0,		0x62},
	{0x0c44,	15,  8,		0x20},
	{0x0c44,	23, 16,		0x20},
	{0x0c44,	31, 24,		0x5a},
	{0x0c48,	 7,  0,		0x24},
	{0x0c48,	15,  8,		0x04},
	{0x0c48,	23, 16,		0x04},
	{0x0c48,	31, 24,		0x40},
	{0x0c4c,	 7,  0,		0x02},
	{0x0c4c,	15,  8,		0x40},
	{0x0c50,	15,  8,		0x1c},
	{0x0c50,	31, 24,		0x19},
	{0x0c54,	15,  8,		0x21},
	{0x0c58,	 7,  0,		0x60},
	{0x0c60,	 7,  0,		0x7c},
	{0x0c60,	15,  8,		0x1e},
	{0x0c60,	23, 16,		0x13},
	{0x0c60,	31, 24,		0x80},
	{0x0c64,	 7,  0,		0x02},
	{0x0c64,	15,  8,		0xcb},
	{0x0c64,	31, 24,		0x84},
	{0x0c68,	15,  8,		0x82},
	{0x0c68,	23, 16,		0x07},
	{0x0c68,	31, 24,		0x17},
	{0x0c6c,	 7,  0,		0x16},
	{0x0c74,	15,  8,		0x04},
	{0x0c78,	15,  8,		0xc0},
	{0x0c00,	 7,  0,		0x03},

	{0x0204,	 7,  0,		0x80},
	{0x0208,	 7,  0,		0x0d},
	{0x0208,	15,  8,		0x92},
	{0x0204,	31, 24,		0xfc},
	{0x0208,	 7,  0,		0x04},
	{0x0208,	15,  8,		0x91},
	{0x0208,	23, 16,		0x01},
	{0x0210,	31, 24,		0x1a},
	{0x0214,	 7,  0,		0x5c},
	{0x0214,	15,  8,		0x6b},
	{0x0214,	23, 16,		0x00},
	{0x0218,	 7,  0,		0x84},
	{0x0218,	23, 16,		0x80},
	{0x0218,	31, 24,		0x7a},
	{0x022c,	23, 16,		0x30},
	{0x0230,	15,  8,		0x08},
	{0x024c,	23, 16,		0x80},
	{0x0250,	31, 24,		0x30},
	{0x0260,	 7,  0,		0x02},
	{0x0264,	 7,  0,		0x57},
	{0x0268,	15,  8,		0x57},
	{0x0268,	23, 16,		0x57},
	{0x0278,	31, 24,		0xff},
	{0x0280,	 7,  0,		0x50},
	{0x0280,	23, 16,		0x50},
	{0x0284,	 7,  0,		0x15},
	{0x0284,	15,  8,		0x1f},
	{0x028c,	15,  8,		0x6f},
	{0x0294,	15,  8,		0x00},
	{0x0294,	23, 16,		0x00},
	{0x0294,	31, 24,		0x01},
	{0x0298,	 7,  0,		0x41},
	{0x0298,	15,  8,		0x26},
	{0x0298,	31, 24,		0x00},
	{0x029c,	 7,  0,		0x03},
	{0x02a4,	 7,  0,		0x13},
	{0x02a4,	15,  8,		0x0f},
	{0x02a8,	15,  8,		0xb6},
	{0x02a8,	23, 16,		0x01},
	{0x0380,	 7,  0,		0x30},
	{0x03c0,	15,  8,		0x02},
	{0x03cc,	 7,  0,		0x18},
	{0x03cc,	 7,  0,		0x00},
	{0x0380,	 4,  4,		0x00},
	{0x03c0,	 9,  9,		0x00},

	{0x0404,	 7,  0,		0x80},
	{0x0408,	 7,  0,		0x0d},
	{0x0408,	15,  8,		0x92},
	{0x0404,	31, 24,		0xfc},
	{0x0408,	 7,  0,		0x04},
	{0x0408,	15,  8,		0x91},
	{0x0408,	23, 16,		0x01},
	{0x0410,	31, 24,		0x1a},
	{0x0414,	 7,  0,		0x5c},
	{0x0414,	15,  8,		0x6b},
	{0x0414,	23, 16,		0x00},
	{0x0418,	 7,  0,		0x84},
	{0x0418,	23, 16,		0x80},
	{0x0418,	31, 24,		0x7a},
	{0x042c,	23, 16,		0x30},
	{0x0430,	15,  8,		0x08},
	{0x044c,	23, 16,		0x80},
	{0x0450,	31, 24,		0x30},
	{0x0460,	 7,  0,		0x02},
	{0x0464,	 7,  0,		0x57},
	{0x0468,	15,  8,		0x57},
	{0x0468,	23, 16,		0x57},
	{0x0478,	31, 24,		0xff},
	{0x0480,	 7,  0,		0x50},
	{0x0480,	23, 16,		0x50},
	{0x0484,	 7,  0,		0x15},
	{0x0484,	15,  8,		0x1f},
	{0x048c,	15,  8,		0x6f},
	{0x0494,	15,  8,		0x00},
	{0x0494,	23, 16,		0x00},
	{0x0494,	31, 24,		0x01},
	{0x0498,	 7,  0,		0x41},
	{0x0498,	15,  8,		0x26},
	{0x0498,	31, 24,		0x00},
	{0x049c,	 7,  0,		0x03},
	{0x04a4,	 7,  0,		0x13},
	{0x04a4,	15,  8,		0x0f},
	{0x04a8,	15,  8,		0xb6},
	{0x04a8,	23, 16,		0x01},
	{0x0580,	 7,  0,		0x30},
	{0x05c0,	15,  8,		0x02},
	{0x05cc,	 7,  0,		0x18},
	{0x05cc,	 7,  0,		0x00},
	{0x0580,	 4,  4,		0x00},
	{0x05c0,	 9,  9,		0x00},

	{0x0a00,	15,  8,		0x08},
	{0x0a84,	 7,  0,		0x00},
	{0x0a8c,	23, 16,		0x13},
	{0x0a90,	23, 16,		0xa0},
	{0x0a90,	31, 24,		0x77},
	{0x0a94,	 7,  0,		0x77},
	{0x0a94,	15,  8,		0x77},
	{0x0b08,	23, 16,		0x04},
	{0x0b08,	31, 24,		0x00},
	{0x0b0c,	 7,  0,		0x00},
	{0x0b0c,	15,  8,		0x00},
	{0x0b0c,	23, 16,		0x0f},
	{0x0b10,	31, 24,		0xbe},
	{0x0b14,	 7,  0,		0xff},
	{0x0b18,	 7,  0,		0x14},
	{0x0b5c,	23, 16,		0x1b},
	{0x0b5c,	31, 24,		0x98},
	{0x0b64,	15,  8,		0x11},
	{0x0b78,	15,  8,		0x0c},
	{0x0abc,	31, 24,		0xe0},
	{0x0ac0,	 7,  0,		0x8b},
};

struct firmware ks2_xgbe_serdes = {
	.hdr = {
		.magic_1 = INIT_FW_MAGIC_1,
		.version = {'3', '.', '3', '.', '0', '.', '2', 'c'},
		.magic_2 = INIT_FW_MAGIC_2,
	},
	.data = &cfg_phyb_10p3125g_156p25mhz_16bit_3_3_0_2c[0],
	.size = sizeof(cfg_phyb_10p3125g_156p25mhz_16bit_3_3_0_2c)
};

static inline void *devm_kzalloc(struct device *dev, unsigned int size,
				 unsigned int flags)
{
	return kzalloc(size, flags);
}

static int
request_firmware(const struct firmware **firmware_p, const char *name,
		 struct device *device)
{
	if (!strcmp(name, "ks2_gbe_serdes.bin")) {
		*firmware_p = &ks2_gbe_serdes;
		return 0;
	} else if (!strcmp(name, "ks2_xgbe_serdes.bin")) {
		*firmware_p = &ks2_xgbe_serdes;
		return 0;
	}

	return -EINVAL;
}

static inline void release_firmware(const struct firmware *fw)
{
}

static bool time_after(unsigned long checks, unsigned long timeout_checks)
{
	if (checks > timeout_checks)
		return true;

	udelay(1000);
	return false;
}

/* val MUST be properly positioned according to mask.
 * Eg. if mask = 0x000000c0 and want to write 0x2 to
 * bit[7:6], then val = (0x2 << 6) = 0x80
 */
static inline int regmap_update_bits(void __iomem *base, unsigned int reg,
				     unsigned int mask, unsigned int val)
{
	reg_rmw(base + reg, val, mask);
	return 0;
}

static inline int regmap_read(void __iomem *base, unsigned int reg, u32 *val)
{
	*val = readl(base + reg);
	return 0;
}

static inline int next_enable_lane(struct kserdes_config *sc, int i)
{
	int j = i;

	while (++j < sc->lanes) {
		if (sc->lane[j].enable)
			return j;
	}
	return j;
}

#define for_each_lane(sc, i) \
	for (i = 0; i < (sc)->lanes; i++)

#define for_each_enable_lane(sc, i) \
	for (i = -1; i = next_enable_lane(sc, i), i < sc->lanes; )

static inline u32 kserdes_readl(void __iomem *base, u32 offset)
{
	return readl(base + offset);
}

static inline void kserdes_writel(void __iomem *base, u32 offset, u32 value)
{
	writel(value, base + offset);
}

static void kserdes_do_config(void __iomem *base,
			      struct serdes_cfg *cfg, u32 size)
{
	u32 i;

	for (i = 0; i < size; i++)
		FINSR(base, cfg[i].ofs, cfg[i].msb, cfg[i].lsb, cfg[i].val);
}

static bool is_init_fw_compatible(struct kserdes_config *sc,
				  struct serdes_cfg_header *hdr)
{
	int i = 0;

	if ((hdr->magic_1 != INIT_FW_MAGIC_1) ||
	    (hdr->magic_2 != INIT_FW_MAGIC_2)) {
		dev_err(sc->dev, "incompatible fw %s\n", sc->init_fw);
		return false;
	}

	while (compatible_init_fw_version[i]) {
		if (!strcmp(compatible_init_fw_version[i], hdr->version)) {
			dev_dbg(sc->dev, "init fw %s: version %s\n",
				sc->init_fw, hdr->version);
			return true;
		}

		++i;
	}

	dev_err(sc->dev, "incompatible fw %s: version %s\n",
		sc->init_fw, hdr->version);
	return false;
}

static int kserdes_load_init_fw(struct kserdes_config *sc,
				const char * const *a_firmwares,
				int n_firmwares)
{
	const struct firmware *fw;
	bool found = false;
	int ret, i;
	struct serdes_cfg_header hdr;
	int hdr_sz;

	for (i = 0; i < n_firmwares; i++) {
		if (a_firmwares[i]) {
			ret = request_firmware(&fw, a_firmwares[i], sc->dev);
			if (!ret) {
				found = true;
				break;
			}
		}
	}

	if (!found) {
		dev_err(sc->dev, "can't get any serdes init fw");
		return -ENODEV;
	}

	sc->init_fw = a_firmwares[i];

	memcpy((void *)&hdr, &fw->hdr, sizeof(hdr));
	hdr_sz = 0;
	hdr.version[MAX_VERSION - 1] = 0;
	if (!is_init_fw_compatible(sc, &hdr))
		return -EINVAL;

	sc->init_cfg = devm_kzalloc(sc->dev, fw->size - hdr_sz, GFP_KERNEL);
	memcpy((void *)sc->init_cfg, fw->data + hdr_sz, fw->size - hdr_sz);
	sc->init_cfg_len = fw->size - hdr_sz;
	release_firmware(fw);

	kserdes_do_config(sc->regs, sc->init_cfg,
			  sc->init_cfg_len / sizeof(struct serdes_cfg));

	return 0;
}

static inline u32 _kserdes_read_tbus_val(void __iomem *sregs)
{
	u32 tmp;

	if (PHY_A(sregs)) {
		tmp  = ((kserdes_readl(sregs, CMU0_REG(0xec))) >> 24) & 0x0ff;
		tmp |= ((kserdes_readl(sregs, CMU0_REG(0xfc))) >> 16) & 0xf00;
	} else {
		tmp  = ((kserdes_readl(sregs, CMU0_REG(0xf8))) >> 16) & 0xfff;
	}

	return tmp;
}

static void _kserdes_write_tbus_addr(void __iomem *sregs, int select, int ofs)
{
	if (select && !FOUR_LANE(sregs))
		++select;

	if (PHY_A(sregs))
		FINSR(sregs, CMU0_REG(0x8), 31, 24, ((select << 5) + ofs));
	else
		FINSR(sregs, CMU0_REG(0xfc), 26, 16, ((select << 8) + ofs));
}

static u32 _kserdes_read_select_tbus(void __iomem *sregs, int select, int ofs)
{
	_kserdes_write_tbus_addr(sregs, select, ofs);
	return _kserdes_read_tbus_val(sregs);
}

static inline void kserdes_set_tx_idle(struct kserdes_config *sc, u32 lane)
{
	if (sc->phy_type != KSERDES_PHY_XGE)
		FINSR(sc->regs, LANEX_REG(lane, 0xb8), 17, 16, 3);

	FINSR(sc->regs, LANE_CTRL_STS_REG(lane), 25, 24, 3);
	FINSR(sc->regs, LANEX_REG(lane, 0x28), 21, 20, 0);
}

static inline void kserdes_clr_tx_idle(struct kserdes_config *sc, u32 lane)
{
	if (sc->phy_type != KSERDES_PHY_XGE)
		FINSR(sc->regs, LANEX_REG(lane, 0xb8), 17, 16, 0);

	FINSR(sc->regs, LANE_CTRL_STS_REG(lane), 25, 24, 0);
	FINSR(sc->regs, LANEX_REG(lane, 0x28), 21, 20, 0);
}

static void kserdes_set_lane_ov(struct kserdes_config *sc, u32 lane)
{
	u32 val_0, val_1, val;

	val_0 = _kserdes_read_select_tbus(sc->regs, lane + 1, 0);
	val_1 = _kserdes_read_select_tbus(sc->regs, lane + 1, 1);

	val = 0;
	val |= ((val_1 >> 9) & 0x3) << 1;
	val |= (val_0 & 0x3) << 3;
	val |= ((val_0 >> 2) & 0x1ff) << 5;
	val |= (1 << 14);
	val &= ~0x60;

	FINSR(sc->regs, LANEX_REG(lane, 0x028), 29, 15, val);
}

static inline void kserdes_assert_reset(struct kserdes_config *sc)
{
	int lane;

	for_each_enable_lane(sc, lane)
		kserdes_set_lane_ov(sc, lane);
}

static inline void kserdes_config_c1_c2_cm(struct kserdes_config *sc, u32 lane)
{
	u32 c1, c2, cm;

	c1 = sc->lane[lane].tx_coeff.c1;
	c2 = sc->lane[lane].tx_coeff.c2;
	cm = sc->lane[lane].tx_coeff.cm;

	if (sc->phy_type == KSERDES_PHY_XGE) {
		FINSR(sc->regs, LANEX_REG(lane, 0x8), 11,  8, (cm & 0xf));
		FINSR(sc->regs, LANEX_REG(lane, 0x8),  4,  0, (c1 & 0x1f));
		FINSR(sc->regs, LANEX_REG(lane, 0x8),  7,  5, (c2 & 0x7));
		FINSR(sc->regs, LANEX_REG(lane, 0x4),
		      18, 18, ((c2 >> 3) & 0x1));
	} else {
		FINSR(sc->regs, LANEX_REG(lane, 0x8), 15, 12, (cm & 0xf));
		FINSR(sc->regs, LANEX_REG(lane, 0x8),  4,  0, (c1 & 0x1f));
		FINSR(sc->regs, LANEX_REG(lane, 0x8), 11,  8, (c2 & 0xf));
	}
}

static inline void kserdes_config_att_boost(struct kserdes_config *sc, u32 lane)
{
	u32 att, boost;

	att = sc->lane[lane].rx_force.att;
	boost = sc->lane[lane].rx_force.boost;

	if (sc->phy_type == KSERDES_PHY_XGE) {
		FINSR(sc->regs, LANEX_REG(lane, 0x98), 13, 13, 0);
		FINSR(sc->regs, LANEX_REG(lane, 0x8c), 15, 12, boost);
		FINSR(sc->regs, LANEX_REG(lane, 0x8c), 11, 8, att);
	} else {
		if (att != -1) {
			FINSR(sc->regs, CML_REG(0x84), 0, 0, 0);
			FINSR(sc->regs, CML_REG(0x8c), 24, 24, 0);
			FINSR(sc->regs, LANEX_REG(lane, 0x8c), 11, 8, att);
		}
		if (boost != -1) {
			FINSR(sc->regs, CML_REG(0x84), 1, 1, 0);
			FINSR(sc->regs, CML_REG(0x8c), 25, 25, 0);
			FINSR(sc->regs, LANEX_REG(lane, 0x8c), 15, 12, boost);
		}
	}
}

static void kserdes_set_tx_rx_fir_coeff(struct kserdes_config *sc, u32 lane)
{
	struct kserdes_tx_coeff *tc = &sc->lane[lane].tx_coeff;

	if (sc->phy_type == KSERDES_PHY_XGE) {
		FINSR(sc->regs, LANEX_REG(lane, 0x004), 29, 26, tc->att);
		FINSR(sc->regs, LANEX_REG(lane, 0x0a4), 2, 0, tc->vreg);
	} else {
		FINSR(sc->regs, LANEX_REG(lane, 0x004), 28, 25, tc->att);
		FINSR(sc->regs, LANEX_REG(lane, 0x084), 7, 5, tc->vreg);
	}

	kserdes_config_c1_c2_cm(sc, lane);

	if (sc->rx_force_enable)
		kserdes_config_att_boost(sc, lane);
}

static inline void
_kserdes_force_signal_detect_low(void __iomem *sregs, u32 lane)
{
	FINSR(sregs, LANEX_REG(lane, 0x004), 2, 1, 0x2);
}

static inline void
kserdes_force_signal_detect_low(struct kserdes_config *sc, u32 lane)
{
	_kserdes_force_signal_detect_low(sc->regs, lane);
}

static inline void
_kserdes_force_signal_detect_high(void __iomem *sregs, u32 lane)
{
	FINSR(sregs, LANEX_REG(lane, 0x004), 2, 1, 0x0);
}

static inline void
kserdes_force_signal_detect_high(struct kserdes_config *sc, u32 lane)
{
	_kserdes_force_signal_detect_high(sc->regs, lane);
}

static int kserdes_deassert_reset_poll_others(struct kserdes_config *sc)
{
	unsigned long timeout = 500;
	unsigned long time_check = 0;
	u32 lanes_not_ok = 0;
	u32 ofs = 28;
	u32 ret, i;

	for_each_enable_lane(sc, i)
		lanes_not_ok |= (1 << i);

	if (!FOUR_LANE(sc->regs))
		ofs = 29;

	do {
		++time_check;
		for_each_enable_lane(sc, i) {
			if (!(lanes_not_ok & (1 << i)))
				continue;

			ret = kserdes_readl(sc->regs, CML_REG(0x1f8));

			if (ret & BIT(ofs + i))
				lanes_not_ok &= ~BIT(i);
		}

		if (!lanes_not_ok)
			return 0;

		if (time_after(time_check, timeout))
			return -ETIMEDOUT;

		cpu_relax();
	} while (true);
}

static int kserdes_deassert_reset_poll_pcie(struct kserdes_config *sc)
{
	unsigned long timeout = 500;
	unsigned long time_check = 0;
	u32 lanes_not_ok = 0;
	u32 ret, i;

	for_each_enable_lane(sc, i)
		lanes_not_ok |= (1 << i);

	do {
		++time_check;
		for_each_enable_lane(sc, i) {
			if (!(lanes_not_ok & BIT(i)))
				continue;

			ret = _kserdes_read_select_tbus(sc->regs, i + 1, 0x02);

			if (!(ret & BIT(4)))
				lanes_not_ok &= ~BIT(i);
		}

		if (!lanes_not_ok)
			return 0;

		if (time_after(time_check, timeout))
			return -ETIMEDOUT;

		cpu_relax();
	} while (true);
}

static inline void _kserdes_lane_reset(void __iomem *serdes,
				       u32 lane, u32 reset)
{
	FINSR(serdes, LANEX_REG(lane, 0x28), 29, 29, !!reset);
}

static inline void kserdes_release_reset(struct kserdes_config *sc, u32 lane)
{
	if (sc->phy_type == KSERDES_PHY_XGE)
		FINSR(sc->regs, LANEX_REG(lane, 0x60), 0, 0, 0x1);

	_kserdes_lane_reset(sc->regs, lane, 0);
}

static int kserdes_deassert_reset(struct kserdes_config *sc, u32 poll)
{
	int ret = 0, lane;

	for_each_enable_lane(sc, lane)
		kserdes_release_reset(sc, lane);

	if (!poll)
		goto done;

	if (sc->phy_type == KSERDES_PHY_PCIE)
		ret = kserdes_deassert_reset_poll_pcie(sc);
	else
		ret = kserdes_deassert_reset_poll_others(sc);

done:
	return ret;
}

static inline void _kserdes_lane_enable(void __iomem *sregs, u32 lane)
{
	FINSR(sregs, LANE_CTRL_STS_REG(lane), 31, 29, 0x7);
	FINSR(sregs, LANE_CTRL_STS_REG(lane), 15, 13, 0x7);
}

static inline int _kserdes_set_lane_ctrl_rate(void __iomem *sregs, u32 lane,
					      enum kserdes_lane_ctrl_rate rate)
{
	u32 rate_mode;

	switch (rate) {
	case KSERDES_FULL_RATE:
		rate_mode = 0x4;
		break;
	case KSERDES_QUARTER_RATE:
		rate_mode = 0x6;
		break;
	case KSERDES_HALF_RATE:
		rate_mode = 0x5;
		break;
	default:
		return -EINVAL;
	}

	FINSR(sregs, LANE_CTRL_STS_REG(lane), 28, 26, rate_mode);
	FINSR(sregs, LANE_CTRL_STS_REG(lane), 12, 10, rate_mode);
	return 0;
}

static inline void _kserdes_set_lane_loopback(void __iomem *sregs, u32 lane,
					      enum kserdes_phy_type phy_type)
{
	if (phy_type == KSERDES_PHY_XGE) {
		FINSR(sregs, LANEX_REG(lane, 0x0), 7, 0, 0x4);
		FINSR(sregs, LANEX_REG(lane, 0x4), 2, 1, 0x3);
	} else {
		FINSR(sregs, LANEX_REG(lane, 0x0), 31, 24, 0x40);
	}
}

static inline void _kserdes_clear_wait_after(void __iomem *sregs)
{
	FINSR(sregs, PLL_CTRL_REG, 17, 16, 0);
}

static inline void _kserdes_clear_lane_wait_after(void __iomem *sregs, u32 lane)
{
	FINSR(sregs, PLL_CTRL_REG, lane + 12, lane + 12, 1);
	FINSR(sregs, PLL_CTRL_REG, lane + 4, lane + 4, 1);
}

static inline void _kserdes_pll_enable(void __iomem *sregs)
{
	FINSR(sregs, PLL_CTRL_REG, 31, 29, 0x7);
}

static inline void _kserdes_pll2_enable(void __iomem *sregs)
{
	FINSR(sregs, PLL_CTRL_REG, 27, 25, 0x7);
}

static inline void _kserdes_pll_disable(void __iomem *sregs)
{
	FINSR(sregs, PLL_CTRL_REG, 31, 29, 0x4);
}

static inline void _kserdes_pll2_disable(void __iomem *sregs)
{
	FINSR(sregs, PLL_CTRL_REG, 27, 25, 0x4);
}

static inline u32 _kserdes_get_pll_status(void __iomem *sregs)
{
	return FEXTR(kserdes_readl(sregs, PLL_CTRL_REG), 28, 28);
}

static inline u32 _kserdes_get_pll2_status(void __iomem *sregs)
{
	return FEXTR(kserdes_readl(sregs, PLL_CTRL_REG), 24, 24);
}

static inline void kserdes_lane_enable_loopback(void __iomem *serdes, u32 lane)
{
	FINSR(serdes, LANEX_REG(lane, 0), 31, 24, 0x40);
}

static inline u32 _kserdes_get_lane_status(void __iomem *sregs, u32 lane,
					   enum kserdes_phy_type phy_type)
{
	int d = ((phy_type == KSERDES_PHY_PCIE) ? 0 : 8);

	return FEXTR(kserdes_readl(sregs, PLL_CTRL_REG), lane + d, lane + d);
}

static u32 kserdes_get_pll_lanes_status(struct kserdes_config *sc)
{
	u32 val, i;

	val = _kserdes_get_pll_status(sc->regs);
	if (!val)
		goto done;

	if (sc->phy_type == KSERDES_PHY_XGE) {
		val = _kserdes_get_pll2_status(sc->regs);
		if (!val)
			goto done;
	}

	for_each_enable_lane(sc, i)
		val &= _kserdes_get_lane_status(sc->regs, i, sc->phy_type);

done:
	return val;
}

static int kserdes_get_status(struct kserdes_config *sc)
{
	unsigned long timeout = 500;
	unsigned long time_check = 0;

	do {
		++time_check;
		if (kserdes_get_pll_lanes_status(sc))
			break;

		if (time_after(time_check, timeout))
			return -ETIMEDOUT;

		cpu_relax();
	} while (true);

	return 0;
}

static inline u32 _kserdes_get_tx_termination(void __iomem *sregs,
					      enum kserdes_phy_type phy_type)
{
	return _kserdes_read_select_tbus(sregs, 1,
					 ((phy_type == KSERDES_PHY_XGE) ?
					 0x1a : 0x1b)) & 0xff;
}

static void kserdes_set_tx_terminations(struct kserdes_config *sc, u32 term)
{
	int i;

	for_each_lane(sc, i) {
		FINSR(sc->regs, LANEX_REG(i, 0x7c), 31, 24, term);
		FINSR(sc->regs, LANEX_REG(i, 0x7c), 20, 20, 0x1);
	}
}

static void
_kserdes_write_ofs_xge(void __iomem *sregs, u32 lane, u32 cmp,
		       struct kserdes_cmp_coef_ofs *ofs)
{
	FINSR(sregs, CML_REG(0x8c), 23, 21, cmp);

	FINSR(sregs, CMU0_REG(0xfc), 26, 16, ((lane + 2) << 8) + 0x11);
	ofs->cmp = (_kserdes_read_tbus_val(sregs) & 0x0ff0) >> 4;

	FINSR(sregs, CMU0_REG(0xfc), 26, 16, ((lane + 2) << 8) + 0x11);
	ofs->coef1 = (_kserdes_read_tbus_val(sregs) & 0x000f) << 3;

	FINSR(sregs, CMU0_REG(0xfc), 26, 16, ((lane + 2) << 8) + 0x12);
	ofs->coef1 |= (_kserdes_read_tbus_val(sregs) & 0x0e00) >> 9;
	ofs->coef2  = (_kserdes_read_tbus_val(sregs) & 0x01f8) >> 3;
	ofs->coef3  = (_kserdes_read_tbus_val(sregs) & 0x0007) << 3;

	FINSR(sregs, CMU0_REG(0xfc), 26, 16, ((lane + 2) << 8) + 0x13);
	ofs->coef3 |= (_kserdes_read_tbus_val(sregs) & 0x0e00) >> 9;
	ofs->coef4  = (_kserdes_read_tbus_val(sregs) & 0x01f8) >> 3;
	ofs->coef5  = (_kserdes_read_tbus_val(sregs) & 0x0007) << 3;

	FINSR(sregs, CMU0_REG(0xfc), 26, 16, ((lane + 2) << 8) + 0x14);
	ofs->coef5 |= (_kserdes_read_tbus_val(sregs) & 0x0e00) >> 9;
}

static void kserdes_add_ofs_xge(struct kserdes_config *sc,
				struct kserdes_ofs *sofs)
{
	struct kserdes_cmp_coef_ofs *ctofs;
	struct kserdes_cmp_coef_ofs sample;
	struct kserdes_lane_ofs *lofs;
	u32 lane, cmp;

	for_each_enable_lane(sc, lane) {
		lofs = &sofs->lane_ofs[lane];
		for_each_cmp(cmp) {
			ctofs = &lofs->ct_ofs[cmp];

			_kserdes_write_ofs_xge(sc->regs, lane, cmp, &sample);

			ctofs->cmp  += sample.cmp;
			ctofs->coef1 += sample.coef1;
			ctofs->coef2 += sample.coef2;
			ctofs->coef3 += sample.coef3;
			ctofs->coef4 += sample.coef4;
			ctofs->coef5 += sample.coef5;
		}
	}
}

static void
kserdes_get_cmp_coef_ofs_non_xge(void __iomem *sregs, u32 lane, u32 cmp,
				 struct kserdes_cmp_coef_ofs *ofs)
{
	FINSR(sregs, CML_REG(0x8c), 23, 21, cmp);
	FINSR(sregs, CMU0_REG(0x8), 31, 24, ((lane + 1) << 5) + 0x12);
	ofs->cmp = (_kserdes_read_tbus_val(sregs) & 0x0ff0) >> 4;
}

static void kserdes_add_ofs_non_xge(struct kserdes_config *sc,
				    struct kserdes_ofs *sofs)
{
	struct kserdes_cmp_coef_ofs *ctofs;
	struct kserdes_cmp_coef_ofs sample;
	struct kserdes_lane_ofs *lofs;
	u32 lane, cmp;

	for_each_enable_lane(sc, lane) {
		lofs = &sofs->lane_ofs[lane];
		for_each_cmp(cmp) {
			ctofs = &lofs->ct_ofs[cmp];

			kserdes_get_cmp_coef_ofs_non_xge(sc->regs, lane,
							 cmp, &sample);

			ctofs->cmp  += sample.cmp;
		}
	}
}

static void kserdes_get_average_ofs(struct kserdes_config *sc, u32 samples,
				    struct kserdes_ofs *sofs)
{
	struct kserdes_cmp_coef_ofs *ctofs;
	struct kserdes_lane_ofs *lofs;
	u32 i, lane, cmp;
	int ret;

	memset(sofs, 0, sizeof(*sofs));

	for (i = 0; i < samples; i++) {
		kserdes_assert_reset(sc);
		ret = kserdes_deassert_reset(sc, 1);
		if (ret) {
			dev_err(sc->dev,
				"kserdes_get_average_ofs: reset failed %d\n",
				ret);
			return;
		}

		if (sc->phy_type == KSERDES_PHY_XGE)
			kserdes_add_ofs_xge(sc, sofs);
		else
			kserdes_add_ofs_non_xge(sc, sofs);
	}

	for_each_enable_lane(sc, lane) {
		lofs = &sofs->lane_ofs[lane];
		for_each_cmp(cmp) {
			ctofs = &lofs->ct_ofs[cmp];
			if (sc->phy_type == KSERDES_PHY_XGE) {
				ctofs->cmp  /= samples;
				ctofs->coef1 /= samples;
				ctofs->coef2 /= samples;
				ctofs->coef3 /= samples;
				ctofs->coef4 /= samples;
				ctofs->coef5 /= samples;
			} else {
				ctofs->cmp  /= samples;
			}
		}
	}
}

static void _kserdes_set_ofs(void __iomem *sregs, u32 lane, u32 cmp,
			     struct kserdes_cmp_coef_ofs *ofs)
{
	FINSR(sregs, CML_REG(0xf0), 27, 26, (lane + 1));
	FINSR(sregs, CML_REG(0x98), 24, 24, 0x1);
	FINSR(sregs, LANEX_REG(lane, 0x2c), 2, 2, 0x1);
	FINSR(sregs, LANEX_REG(lane, 0x30), 7, 5, cmp);
	FINSR(sregs, LANEX_REG(lane, 0x5c), 31, 31, 0x1);
	FINSR(sregs, CML_REG(0x9c), 7, 0, ofs->cmp);
	FINSR(sregs, LANEX_REG(lane, 0x58), 30, 24, ofs->coef1);
	FINSR(sregs, LANEX_REG(lane, 0x5c),  5,  0, ofs->coef2);
	FINSR(sregs, LANEX_REG(lane, 0x5c), 13,  8, ofs->coef3);
	FINSR(sregs, LANEX_REG(lane, 0x5c), 21, 16, ofs->coef4);
	FINSR(sregs, LANEX_REG(lane, 0x5c), 29, 24, ofs->coef5);

	FINSR(sregs, LANEX_REG(lane, 0x2c), 10, 10, 0x1);
	FINSR(sregs, LANEX_REG(lane, 0x2c), 10, 10, 0x0);

	FINSR(sregs, CML_REG(0x98), 24, 24, 0x0);
	FINSR(sregs, LANEX_REG(lane, 0x2c), 2, 2, 0x0);
	FINSR(sregs, LANEX_REG(lane, 0x5c), 31, 31, 0x0);
}

static inline void _kserdes_set_cmp_ofs_phyb(void __iomem *sregs, u32 lane,
					     u32 cmp, u32 cmp_ofs)
{
	FINSR(sregs, LANEX_REG(lane, 0x58), 18, 18, 0x1);
	FINSR(sregs, LANEX_REG(lane, 0x4c), 5, 2, (0x1 << (cmp - 1)));
	FINSR(sregs, LANEX_REG(lane, 0x48), 24, 17, cmp_ofs);
	FINSR(sregs, LANEX_REG(lane, 0x48), 29, 29, 0x1);
	FINSR(sregs, LANEX_REG(lane, 0x48), 29, 29, 0x0);
	FINSR(sregs, LANEX_REG(lane, 0x58), 18, 18, 0x0);
}

static inline void _kserdes_set_coef_ofs(void __iomem *sregs, u32 lane,
					 u32 coef, u32 width, u32 coef_ofs)
{
	FINSR(sregs, LANEX_REG(lane, 0x58), 23, 19, (0x1 << (coef - 1)));
	FINSR(sregs, LANEX_REG(lane, 0x48), 17 + (width - 1), 17, coef_ofs);
	FINSR(sregs, LANEX_REG(lane, 0x48), 29, 29, 0x1);
	FINSR(sregs, LANEX_REG(lane, 0x48), 29, 29, 0x0);
}

static void _kserdes_set_ofs_phyb(void __iomem *sregs, u32 lane, u32 cmp,
				  struct kserdes_cmp_coef_ofs *ofs)
{
	FINSR(sregs, LANEX_REG(lane, 0x58), 16, 16, 0x1);
	FINSR(sregs, LANEX_REG(lane, 0x48), 16, 16, 0x1);

	_kserdes_set_cmp_ofs_phyb(sregs, lane, cmp, ofs->cmp);

	FINSR(sregs, LANEX_REG(lane, 0x58), 17, 17, 0x1);

	_kserdes_set_coef_ofs(sregs, lane, 1, 7, ofs->coef1);
	_kserdes_set_coef_ofs(sregs, lane, 2, 6, ofs->coef2);
	_kserdes_set_coef_ofs(sregs, lane, 3, 6, ofs->coef3);
	_kserdes_set_coef_ofs(sregs, lane, 4, 6, ofs->coef4);
	_kserdes_set_coef_ofs(sregs, lane, 5, 6, ofs->coef5);

	FINSR(sregs, LANEX_REG(lane, 0x58), 16, 16, 0x0);
	FINSR(sregs, LANEX_REG(lane, 0x48), 16, 16, 0x0);
	FINSR(sregs, LANEX_REG(lane, 0x58), 18, 18, 0x0);
	FINSR(sregs, LANEX_REG(lane, 0x58), 17, 17, 0x0);
}

static void kserdes_set_ofs_xge(struct kserdes_config *sc,
				struct kserdes_ofs *sofs)
{
	struct kserdes_cmp_coef_ofs *ctofs;
	struct kserdes_lane_ofs *lofs;
	int lane, cmp;

	for_each_enable_lane(sc, lane) {
		lofs = &sofs->lane_ofs[lane];
		for_each_cmp(cmp) {
			ctofs = &lofs->ct_ofs[cmp];
			_kserdes_set_ofs(sc->regs, lane, cmp, ctofs);
			_kserdes_set_ofs_phyb(sc->regs, lane, cmp, ctofs);
		}
	}
}

static void kserdes_set_ofs_non_xge(struct kserdes_config *sc,
				    struct kserdes_ofs *sofs)
{
	struct kserdes_cmp_coef_ofs *ctofs;
	struct kserdes_lane_ofs *lofs;
	u32 lane, cmp;

	for_each_enable_lane(sc, lane) {
		lofs = &sofs->lane_ofs[lane];
		for_each_cmp(cmp) {
			ctofs = &lofs->ct_ofs[cmp];
			_kserdes_set_ofs(sc->regs, lane, cmp, ctofs);
		}
	}
}

static void kserdes_set_average_ofs(struct kserdes_config *sc,
				    struct kserdes_ofs *sofs)
{
	if (sc->phy_type == KSERDES_PHY_XGE)
		kserdes_set_ofs_xge(sc, sofs);
	else
		kserdes_set_ofs_non_xge(sc, sofs);
}

static void kserdes_phyb_init_config(struct kserdes_config *sc,
				     struct kserdes_ofs *sofs)
{
	int lane;

	for_each_enable_lane(sc, lane)
		kserdes_force_signal_detect_low(sc, lane);

	usleep_range(10, 20);

	kserdes_get_average_ofs(sc, OFFSET_SAMPLES, sofs);
	kserdes_set_average_ofs(sc, sofs);
	usleep_range(10, 20);

	for_each_enable_lane(sc, lane)
		kserdes_force_signal_detect_high(sc, lane);

	usleep_range(10, 20);
}

static int kserdes_wait_lane_rx_valid(struct kserdes_config *sc, u32 lane)
{
	unsigned long timeout = 500;
	unsigned long time_check = 0;
	u32 status;

	do {
		++time_check;
		status = _kserdes_read_select_tbus(sc->regs, lane + 1, 0x02);

		if (status & 0x20)
			return 0;

		if (time_after(time_check, timeout))
			return -ETIMEDOUT;

		cpu_relax();
	} while (true);
}

static inline void _kserdes_reset(void __iomem *sregs)
{
	FINSR(sregs, CPU_CTRL_REG, 29, 29, 0x1);
	usleep_range(10, 20);
	FINSR(sregs, CPU_CTRL_REG, 29, 29, 0x0);
	usleep_range(10, 20);
}

static inline void kserdes_xge_pll_enable(struct kserdes_config *sc)
{
	if (!sc->firmware)
		FINSR(sc->regs, CML_REG(0), 7, 0, 0x1f);

	if (sc->link_rate == KSERDES_LINK_RATE_10P3125G) {
		_kserdes_pll_enable(sc->regs);
		_kserdes_pll2_enable(sc->regs);
	} else if (sc->link_rate == KSERDES_LINK_RATE_1P25G) {
		kserdes_writel(sc->regs, PLL_CTRL_REG, PLL_ENABLE_1P25G);
	}
}

static inline void _kserdes_enable_xgmii_port(void __iomem *peripheral_regmap,
					      u32 port)
{
	regmap_update_bits(peripheral_regmap, XGE_CTRL_OFFSET,
			   GENMASK(port, port), BIT(port));
}

static inline void _kserdes_reset_rx(void __iomem *sregs, int lane)
{
	_kserdes_force_signal_detect_low(sregs, lane);
	usleep_range(1000, 2000);
	_kserdes_force_signal_detect_high(sregs, lane);
}

static int kserdes_check_link_status(struct kserdes_config *sc,
				     u32 *current_lane_state,
				     u32 lanes_chk_mask,
				     u32 *lanes_up_mask)
{
	u32 pcsr_rx_stat, blk_lock, blk_errs;
	int loss, i, link_up = 1;
	int ret;
	unsigned long lmask = (unsigned long)lanes_chk_mask;

	for_each_set_bit(i, &lmask, 8) {
		loss = (kserdes_readl(sc->regs, LANE_CTRL_STS_REG(i))) & 0x01;

		ret = regmap_read(sc->pcsr_regmap, PCSR_RX_STATUS(i),
				  &pcsr_rx_stat);

		if (ret)
			return ret;

		blk_lock = (pcsr_rx_stat >> 30) & 0x1;
		blk_errs = (pcsr_rx_stat >> 16) & 0x0ff;

		if (blk_errs)
			blk_lock = 0;

		switch (current_lane_state[i]) {
		case 0:
			if (!loss && blk_lock) {
				dev_dbg(dev, "XGE PCSR Linked Lane: %d\n", i);
				FINSR(sc->regs, LANEX_REG(i, 0x04), 2, 1, 0);
				current_lane_state[i] = 1;
			} else if (!blk_lock) {
					dev_dbg(dev,
						"XGE PCSR Recover Lane: %d\n",
						i);

					_kserdes_reset_rx(sc->regs, i);
			}
			break;
		case 1:
			if (!blk_lock)
				current_lane_state[i] = 2;

			break;
		case 2:
			if (blk_lock) {
				current_lane_state[i] = 1;
			} else {
				_kserdes_reset_rx(sc->regs, i);
				current_lane_state[i] = 0;
			}
			break;
		default:
			dev_info(dev,
				 "XGE: unknown current_lane_state[%d] %d\n",
				 i, current_lane_state[i]);
			break;
		}

		if (blk_errs) {
			regmap_update_bits(sc->pcsr_regmap, PCSR_RX_CTL(i),
					   GENMASK(7, 0), 0x19);
			regmap_update_bits(sc->pcsr_regmap, PCSR_RX_CTL(i),
					   GENMASK(7, 0), 0x00);
		}

		if (current_lane_state[i] == 1) {
			*lanes_up_mask |= BIT(i);
		} else {
			*lanes_up_mask &= ~BIT(i);
			link_up = 0;
		}
	}

	return link_up;
}

static int kserdes_wait_link_up(struct kserdes_config *sc,
				u32 lanes_chk_mask,
				u32 *lanes_up_mask)
{
	u32 current_state[KSERDES_MAX_LANES];
	unsigned long timeout = 50;
	unsigned long time_check = 0;
	int i, link_up, ret = 0;

	memset(current_state, 0, sizeof(current_state));

	do {
		++time_check;
		usleep_range(10000, 20000);
		link_up = kserdes_check_link_status(sc, current_state,
						    lanes_chk_mask,
						    lanes_up_mask);

		if (link_up)
			break;

		for_each_enable_lane(sc, i) {
			if (!(*lanes_up_mask & BIT(i))) {
				dev_dbg(sc->dev,
					"XGE: detected lane %d down\n", i);
			}
		}

		if (time_after(time_check, timeout)) {
			ret = -ETIMEDOUT;
			break;
		}

	} while (1);

	return ret;
}

static inline void kserdes_xfw_get_lane_params(struct kserdes_config *sc,
					       int lane)
{
	struct kserdes_fw_config *fw = &sc->fw;
	u32 tx_ctrl, val_0, val_1;
	u32 phy_a = PHY_A(sc->regs);

	val_0 = kserdes_readl(sc->regs, LANEX_REG(lane, 0x04));
	val_1 = kserdes_readl(sc->regs, LANEX_REG(lane, 0x08));

	tx_ctrl = ((((val_0 >> 18) & 0x1)    << 24) |
		   (((val_1 >> 0)  & 0xffff) <<  8) |
		   (((val_0 >> 24) & 0xff)   <<  0));

	if (phy_a) {
		fw->cm = (val_1 >> 12) & 0xf;
		fw->c1 = (val_1 >> 0) & 0x1f;
		fw->c2 = (val_1 >> 8) & 0xf;
	} else {
		fw->cm = (tx_ctrl >> 16) & 0xf;
		fw->c1 = (tx_ctrl >> 8) & 0x1f;
		fw->c2 = (tx_ctrl >> 13) & 0x7;
		fw->c2 = fw->c2 | (((tx_ctrl >> 24) & 0x1) << 3);
	}

	val_0 = _kserdes_read_select_tbus(sc->regs, lane + 1,
					  (phy_a ? 0x11 : 0x10));
	fw->attn = (val_0 >> 4) & 0xf;
	fw->boost = (val_0 >> 8) & 0xf;

	val_0 = _kserdes_read_select_tbus(sc->regs, lane + 1, 0x5);
	fw->dlpf = (val_0 >> 2) & 0x3ff;

	val_0 = _kserdes_read_select_tbus(sc->regs, lane + 1, 0x6);
	fw->rxcal = (val_0 >> 3) & 0xff;
}

static inline void kserdes_xfw_mem_init(struct kserdes_config *sc)
{
	struct kserdes_fw_config *fw = &sc->fw;
	u32 i, lane_config = 0;

	for_each_lane(sc, i)
		lane_config = (lane_config << 8) | (fw->lane_config[i] & 0xff);

	lane_config <<= 8;

	kserdes_writel(sc->regs, MEM_ADR_REG, KSERDES_XFW_CONFIG_START_ADDR);

	for (i = KSERDES_XFW_CONFIG_START_ADDR;
	     i < KSERDES_XFW_PARAM_START_ADDR; i += 4)
		kserdes_writel(sc->regs, MEM_DATINC_REG, 0x00000000);

	kserdes_writel(sc->regs, MEM_DATINC_REG, XFM_FLUSH_CMD);
	kserdes_writel(sc->regs, MEM_DATINC_REG, fw->fast_train);
	kserdes_writel(sc->regs, MEM_DATINC_REG, 0x00000000);
	kserdes_writel(sc->regs, MEM_DATINC_REG, fw->lane_seeds);
	kserdes_writel(sc->regs, MEM_DATINC_REG, lane_config);
}

static int kserdes_pcie_lanes_enable(struct kserdes_config *sc)
{
	int ret, i;
	u32 lanes_enable = 0;

	for_each_enable_lane(sc, i)
		lanes_enable |= BIT(i);

	for_each_lane(sc, i) {
		kserdes_release_reset(sc, i);

		if (sc->lane[i].loopback)
			_kserdes_set_lane_loopback(sc->regs, i, sc->phy_type);
	}

	ret = kserdes_get_status(sc);
	if (ret)
		return ret;

	return lanes_enable;
}

static void kserdes_clear_wait_after(struct kserdes_config *sc,
				     unsigned long lanes_mask)
{
	u32 lane;

	if (!sc->rx_force_enable) {
		for_each_set_bit(lane, &lanes_mask, 8) {
			if (!LANE_ENABLE(sc, lane))
				continue;

			_kserdes_clear_lane_wait_after(sc->regs, lane);
		}
	} else {
		_kserdes_clear_wait_after(sc->regs);
	}
}

static int kserdes_check_lanes_status(struct kserdes_config *sc)
{
	unsigned long timeout = 500;
	unsigned long time_check = 0;
	u32 val, i;

	do {
		++time_check;
		val = 1;

		for_each_enable_lane(sc, i)
			val &= _kserdes_get_lane_status(sc->regs, i,
							sc->phy_type);

		if (val)
			break;

		if (time_after(time_check, timeout))
			return -ETIMEDOUT;

		cpu_relax();
	} while (true);

	return 0;
}

static void kserdes_phya_init_config(struct kserdes_config *sc, u32 lane)
{
	u32 coef1val, coef2val, coef3val, coef4val, coef5val;
	void __iomem *sregs = sc->regs;
	u32 cmp, coef1_ofs;

	for_each_cmp(cmp) {
		if (!(cmp & 0x1))
			continue;

		FINSR(sregs, CML_REG(0x8c), 23, 21, cmp);

		FINSR(sregs, CMU0_REG(0x8), 31, 24, ((lane + 1) << 5) + 0x12);
		coef1_ofs = (_kserdes_read_tbus_val(sregs) & 0x000f) << 3;

		FINSR(sregs, CMU0_REG(0x8), 31, 24, ((lane + 1) << 5) + 0x13);
		coef1_ofs |= (_kserdes_read_tbus_val(sregs) & 0x0e00) >> 9;

		coef1val = coef1_ofs - 14;
		coef2val = 31;
		coef3val = 31;
		coef4val = 31;
		coef5val = 31;

		FINSR(sregs, CML_REG(0xf0), 27, 26, lane + 1);
		FINSR(sregs, LANEX_REG(lane, 0x2c), 2, 2, 0x1);
		FINSR(sregs, LANEX_REG(lane, 0x30), 7, 5, cmp);
		FINSR(sregs, LANEX_REG(lane, 0x5c), 31, 31, 0x1);

		FINSR(sregs, LANEX_REG(lane, 0x58), 30, 24, coef1val);
		FINSR(sregs, LANEX_REG(lane, 0x5c),  6,  0, coef2val);
		FINSR(sregs, LANEX_REG(lane, 0x5c), 13,  8, coef3val);
		FINSR(sregs, LANEX_REG(lane, 0x5c), 21, 16, coef4val);
		FINSR(sregs, LANEX_REG(lane, 0x5c), 29, 24, coef5val);

		FINSR(sregs, LANEX_REG(lane, 0x2c), 10, 10, 0x1);
		FINSR(sregs, LANEX_REG(lane, 0x2c), 10, 10, 0x0);

		FINSR(sregs, LANEX_REG(lane, 0x2c), 2, 2, 0x0);
		FINSR(sregs, LANEX_REG(lane, 0x5c), 31, 31, 0x0);

		FINSR(sregs, LANEX_REG(lane, 0x58), 16, 16, 0x1);
		FINSR(sregs, LANEX_REG(lane, 0x48), 16, 16, 0x1);

		FINSR(sregs, LANEX_REG(lane, 0x4c), 5, 2, (0x1 << (cmp - 1)));
		FINSR(sregs, LANEX_REG(lane, 0x58), 17, 17, 0x1);

		_kserdes_set_coef_ofs(sregs, lane, 1, 7, coef1val);
		_kserdes_set_coef_ofs(sregs, lane, 2, 6, coef2val);
		_kserdes_set_coef_ofs(sregs, lane, 3, 6, coef3val);
		_kserdes_set_coef_ofs(sregs, lane, 4, 6, coef4val);
		_kserdes_set_coef_ofs(sregs, lane, 5, 6, coef5val);

		FINSR(sregs, LANEX_REG(lane, 0x58), 16, 16, 0x0);
		FINSR(sregs, LANEX_REG(lane, 0x48), 16, 16, 0x0);
		FINSR(sregs, LANEX_REG(lane, 0x58), 17, 17, 0x0);
	}
}

static int kserdes_check_pll_status(struct kserdes_config *sc)
{
	unsigned long timeout = 500;
	unsigned long time_check = 0;
	u32 val;

	do {
		++time_check;
		val = _kserdes_get_pll_status(sc->regs);

		if (sc->phy_type == KSERDES_PHY_XGE)
			val &= _kserdes_get_pll2_status(sc->regs);

		if (val)
			break;

		if (time_after(time_check, timeout))
			return -ETIMEDOUT;

		cpu_relax();
	} while (true);

	return 0;
}

static void kserdes_enable_common_set_lane_rate(struct kserdes_config *sc,
						u32 lane)
{
	int ret;

	ret = _kserdes_set_lane_ctrl_rate(sc->regs, lane,
					  sc->lane[lane].ctrl_rate);
	if (ret) {
		dev_err(sc->dev, "set_lane_rate FAILED: lane = %d err = %d\n",
			lane, ret);
		return;
	}

	switch (sc->phy_type) {
	case KSERDES_PHY_SGMII:
		FINSR(sc->regs, LANE_CTRL_STS_REG(lane),  7,  6, 0x3);
		FINSR(sc->regs, LANE_CTRL_STS_REG(lane), 23, 21, 0x4);
		FINSR(sc->regs, LANE_CTRL_STS_REG(lane),  5,  3, 0x4);
		break;

	case KSERDES_PHY_XGE:
		FINSR(sc->regs, LANE_CTRL_STS_REG(lane), 23, 21, 0x7);
		FINSR(sc->regs, LANE_CTRL_STS_REG(lane),  5,  3, 0x7);
		if (sc->link_rate == KSERDES_LINK_RATE_10P3125G) {
			FINSR(sc->regs, LANE_CTRL_STS_REG(lane), 16, 16, 0x1);
			FINSR(sc->regs, LANE_CTRL_STS_REG(lane), 19, 19, 0x1);
		}
		break;

	case KSERDES_PHY_PCIE:
		break;

	default:
		FINSR(sc->regs, LANE_CTRL_STS_REG(lane), 23, 21, 0x6);
		FINSR(sc->regs, LANE_CTRL_STS_REG(lane),  5,  3, 0x6);
		break;
	}

	if (sc->lane[lane].loopback)
		_kserdes_set_lane_loopback(sc->regs, lane, sc->phy_type);

	if (sc->phy_type != KSERDES_PHY_XGE) {
		FINSR(sc->regs, LANEX_REG(lane, 0x30), 11, 11, 0x1);
		FINSR(sc->regs, LANEX_REG(lane, 0x30), 13, 12, 0x0);
	}
}

static inline void kserdes_set_lane_rx_starts(struct kserdes_config *sc,
					      u32 lane)
{
	FINSR(sc->regs, LANEX_REG(lane, 0x8c), 11, 8,
	      sc->lane[lane].rx_start.att);
	FINSR(sc->regs, LANEX_REG(lane, 0x8c), 15, 12,
	      sc->lane[lane].rx_start.boost);

	FINSR(sc->regs, LANEX_REG(lane, 0x84), 27, 24,
	      sc->lane[lane].rx_start.att);
	FINSR(sc->regs, LANEX_REG(lane, 0x84), 31, 28,
	      sc->lane[lane].rx_start.boost);

	FINSR(sc->regs, LANEX_REG(lane, 0x84), 19, 16,
	      sc->lane[lane].rx_start.att);
	FINSR(sc->regs, LANEX_REG(lane, 0x84), 23, 20,
	      sc->lane[lane].rx_start.boost);
}

static void kserdes_hs_init_config(struct kserdes_config *sc)
{
	int i;

	if (sc->phy_type != KSERDES_PHY_XGE) {
		if (sc->link_rate >= KSERDES_LINK_RATE_9P8304G)
			FINSR(sc->regs, CML_REG(0xbc), 28, 24, 0x1e);
	}

	for_each_enable_lane(sc, i)
		kserdes_set_tx_idle(sc, i);

	if (sc->link_rate >= KSERDES_LINK_RATE_9P8304G) {
		if (sc->phy_type != KSERDES_PHY_XGE) {
			for_each_enable_lane(sc, i)
				kserdes_force_signal_detect_low(sc, i);

			for_each_enable_lane(sc, i)
				FINSR(sc->regs, LANEX_REG(i, 0x78),
				      30, 24, 0x7f);
		} else {
			FINSR(sc->regs, CML_REG(0x10c), 7, 0, 0xff);
		}
	}
}

static int kserdes_lanes_enable_common(struct kserdes_config *sc,
				       struct kserdes_ofs *sofs)
{
	u32 val, lane_mask = 0;
	int i, ret;

	for_each_lane(sc, i) {
		if (sc->lane[i].enable)
			lane_mask |= BIT(i);
		else
			sc->lane[i].enable = 1;
	}

	if (sc->phy_type == KSERDES_PHY_PCIE) {
		dev_err(sc->dev, "kserdes_lanes_enable_common: pcie TBD.\n");
		return -EINVAL;
	}

	kserdes_hs_init_config(sc);

	for_each_enable_lane(sc, i)
		kserdes_set_lane_rx_starts(sc, i);

	kserdes_assert_reset(sc);

	for_each_enable_lane(sc, i)
		kserdes_set_tx_rx_fir_coeff(sc, i);

	for_each_enable_lane(sc, i)
		kserdes_force_signal_detect_low(sc, i);

	ret = kserdes_deassert_reset(sc, 0);
	if (ret) {
		dev_err(sc->dev, "kserdes_deassert_reset FAILED %d\n", ret);
		return ret;
	}

	for_each_enable_lane(sc, i)
		kserdes_enable_common_set_lane_rate(sc, i);

	if (sc->phy_type == KSERDES_PHY_XGE)
		kserdes_xge_pll_enable(sc);
	else
		_kserdes_pll_enable(sc->regs);

	ret = kserdes_check_pll_status(sc);
	if (ret) {
		dev_err(sc->dev,
			"common init: check pll status FAILED %d\n", ret);
		return ret;
	}

	for_each_enable_lane(sc, i)
		_kserdes_lane_enable(sc->regs, i);

	ret = kserdes_check_lanes_status(sc);
	if (ret) {
		dev_err(sc->dev,
			"common init: check lanes status FAILED %d\n", ret);
		return ret;
	}

	usleep_range(5, 10);

	val = _kserdes_get_tx_termination(sc->regs, sc->phy_type);

	kserdes_set_tx_terminations(sc, val);

	if (sc->phy_type == KSERDES_PHY_XGE)
		kserdes_phyb_init_config(sc, sofs);
	else if (sc->link_rate >= KSERDES_LINK_RATE_9P8304G)
		for_each_enable_lane(sc, i)
			kserdes_phya_init_config(sc, i);

	for_each_enable_lane(sc, i)
		kserdes_clr_tx_idle(sc, i);

	for_each_lane(sc, i)
		sc->lane[i].enable = (lane_mask & BIT(i)) >> i;

	return 0;
}

static inline u32 _kserdes_get_lane_sd(void __iomem *sregs, u32 lane)
{
	return FEXTR(kserdes_readl(sregs, PLL_CTRL_REG), lane, lane);
}

static int _kserdes_wait_lane_sd(void __iomem *sregs, u32 lane)
{
	unsigned long timeout = 500;
	unsigned long time_check = 0;

	do {
		++time_check;

		if (_kserdes_get_lane_sd(sregs, lane))
			break;

		if (time_after(time_check, timeout))
			return -ETIMEDOUT;

		cpu_relax();
	} while (true);

	return 0;
}

static void kserdes_rx_att_boost_config_phyb(struct kserdes_config *sc,
					     u32 lane)
{
	u32 tbus_ofs, rxeq_init_reg_ofs, rxeq_ln_reg_ofs, rxeq_ln_force_bit;
	void __iomem *sregs = sc->regs;
	u32 att_start, att_read, boost_read;
	int ret;

	if (sc->phy_type == KSERDES_PHY_XGE) {
		tbus_ofs = 0x10;
		rxeq_init_reg_ofs = 0x9c;
		rxeq_ln_reg_ofs = 0x98;
		rxeq_ln_force_bit = 14;
	} else {
		tbus_ofs = 0x11;
		rxeq_init_reg_ofs = 0x84;
		rxeq_ln_reg_ofs = 0xac;
		rxeq_ln_force_bit = 11;
	}

	att_start = kserdes_readl(sregs, LANEX_REG(lane, 0x8c));
	att_start = (att_start >> 8) & 0xf;

	att_read = _kserdes_read_select_tbus(sregs, lane + 1, tbus_ofs);
	att_read = (att_read >> 4) & 0xf;

	FINSR(sregs, LANEX_REG(lane, 0x8c), 11, 8, att_read);
	FINSR(sregs, LANEX_REG(lane, rxeq_init_reg_ofs), 0, 0, 0x0);
	FINSR(sregs, CML_REG(0x8c), 24, 24, 0x0);

	FINSR(sregs, LANEX_REG(lane, rxeq_ln_reg_ofs),
	      rxeq_ln_force_bit, rxeq_ln_force_bit, 0x1);
	FINSR(sregs, LANEX_REG(lane, rxeq_ln_reg_ofs),
	      rxeq_ln_force_bit, rxeq_ln_force_bit, 0x0);

	ret = kserdes_wait_lane_rx_valid(sc, lane);
	if (ret) {
		dev_dbg(sc->dev, "kserdes_wait_lane_rx_valid %d FAILED: %d\n",
			lane, ret);
	}
	usleep_range(300, 600);

	boost_read = _kserdes_read_select_tbus(sregs, lane + 1, tbus_ofs);
	boost_read = (boost_read >> 8) & 0xf;

	if (!boost_read) {
		FINSR(sregs, LANEX_REG(lane, 0x2c),  2,  2, 0x1);
		FINSR(sregs, LANEX_REG(lane, 0x2c), 18, 12, 0x2);
		FINSR(sregs, LANEX_REG(lane, 0x2c),  9,  3, 0x1);
		FINSR(sregs, LANEX_REG(lane, 0x2c), 10, 10, 0x1);
		FINSR(sregs, LANEX_REG(lane, 0x2c), 10, 10, 0x0);
		FINSR(sregs, LANEX_REG(lane, 0x2c),  2,  2, 0x0);
		FINSR(sregs, LANEX_REG(lane, 0x2c), 18, 12, 0x0);
		FINSR(sregs, LANEX_REG(lane, 0x2c),  9,  3, 0x0);
	}

	FINSR(sregs, LANEX_REG(lane, 0x8c), 11, 8, att_start);
	FINSR(sregs, LANEX_REG(lane, rxeq_init_reg_ofs), 0, 0, 0x1);
	FINSR(sregs, CML_REG(0x8c), 24, 24, 0x1);
}

static int kserdes_set_dlev_patt_adapt(struct kserdes_config *sc,
				       u32 lane, u32 pattern,
				       struct kserdes_lane_ofs *lofs)
{
	struct kserdes_cmp_coef_ofs *ctofs = &lofs->ct_ofs[4];
	void __iomem *sregs = sc->regs;
	u32 dlevp, dlevn, dlevavg;
	int ret;

	FINSR(sregs, CML_REG(0x158), 14, 8, pattern);
	FINSR(sregs, LANEX_REG(lane, 0x98), 14, 14, 1);
	FINSR(sregs, LANEX_REG(lane, 0x98), 14, 14, 0);

	ret = kserdes_wait_lane_rx_valid(sc, lane);
	if (ret) {
		dev_dbg(sc->dev,
			"set dlev patt: wait_lane_rx_valid FAILED %d\n", ret);
		return ret;
	}

	dlevp = _kserdes_read_select_tbus(sregs, lane + 1, 0x44);
	dlevp = (dlevp >> 4) & 0xff;

	dlevn = _kserdes_read_select_tbus(sregs, lane + 1, 0x45);
	dlevn &= 0xff;

	if (ctofs->cmp <= 120)
		dlevavg = ctofs->cmp - dlevn;
	else if (ctofs->cmp >= 134)
		dlevavg = dlevp - ctofs->cmp;
	else
		dlevavg = (dlevp - dlevn) / 2;

	return dlevavg;
}

static u32 kserdes_eye_monitor_dll_ovr(struct kserdes_config *sc,
				       u32 lane, u32 phase_num, u32 t_offset,
				       u32 phase_shift)
{
	void __iomem *sregs = sc->regs;
	u32 tbus_data, delay, partial_eye, i;
	u32 start_bin = 0, end_bin = 0;
	u32 eye_scan_errors_array[128];
	u32 error_free = 0, val_0, val_1;
	u32 max_dly = 128;
	bool not_phy_xge = (sc->phy_type != KSERDES_PHY_XGE);

	if (t_offset == 0)
		t_offset++;

	if (phase_num == 1) {
		val_0 = 0x00400000;
		val_1 = 0x00000011;
	} else {
		val_0 = 0x00800000;
		val_1 = 0x00000009;
	}
	reg_rmw(sregs + LANEX_REG(lane, 0x2c), val_0, GENMASK(23, 22));
	reg_rmw(sregs + LANEX_REG(lane, 0x30), val_1, GENMASK(4, 0));

	reg_rmw(sregs + CML_REG(0xb8), 0x00004000, GENMASK(15, 8));

	if (phase_num == 1)
		val_0 = 0xffef0000;
	else
		val_0 = 0xfff60000;

	reg_rmw(sregs + CML_REG(0xb8), val_0, GENMASK(31, 16));

	reg_rmw(sregs + CML_REG(0xbc), 0x000fffff, GENMASK(19, 0));
	tbus_data = _kserdes_read_select_tbus(sregs, lane + 1, 0x02);
	tbus_data = tbus_data;
	usleep_range(250, 500);

	for (i = 0; i < max_dly; i = i + t_offset) {
		reg_rmw(sregs + LANEX_REG(lane, 0x2c),
			(i & 0xff) << 24, GENMASK(31, 24));

		reg_rmw(sregs + LANEX_REG(lane, 0x30),
			phase_shift, GENMASK(1, 0));
		usleep_range(5, 10);
		reg_rmw(sregs + CML_REG(0xb8), 0x0000c000, GENMASK(15, 8));
		usleep_range(500, 1000);

		val_0 = _kserdes_read_select_tbus(sregs, lane + 1,
						  not_phy_xge ?  0x1a : 0x19);
		val_0 <<= 4;

		val_1 = _kserdes_read_select_tbus(sregs, lane + 1,
						  not_phy_xge ?  0x1b : 0x1a);
		val_1 = (val_1 >> 8) & 0xf;

		eye_scan_errors_array[i] = (val_0 | val_1);

		reg_rmw(sregs + CML_REG(0xb8), 0x00004000, GENMASK(15, 8));
	}

	partial_eye = 0;
	error_free = 0;

	for (i = 0; i < max_dly; i = i + t_offset) {
		if (i == 0) {
			if (eye_scan_errors_array[i] < 16384)
				partial_eye = 1;
		} else {
			if (eye_scan_errors_array[i] > 16384 + 3000)
				partial_eye = 0;
		}

		if ((eye_scan_errors_array[i] < 16384) &&
		    (partial_eye == 0) &&
		    (error_free  == 0)) {
			if (!((eye_scan_errors_array[i - 1] > 16384) &&
			      (eye_scan_errors_array[i + 1] > 16384))) {
				error_free = 1;
				start_bin = i;
			}
		} else if ((eye_scan_errors_array[i] > 16384) &&
			   (partial_eye == 0) &&
			   (error_free  == 1)) {
			if (!((eye_scan_errors_array[i - 1] < 16384) &&
			      (eye_scan_errors_array[i + 1] < 16384))) {
				end_bin = i;
				break;
			}
		}
	}

	delay = (end_bin - start_bin) / 4 + start_bin;
	reg_rmw(sregs + LANEX_REG(lane, 0x30), 0x00000000, GENMASK(7, 0));
	reg_rmw(sregs + LANEX_REG(lane, 0x2c), 0x00000003, GENMASK(7, 0));
	reg_rmw(sregs + CML_REG(0x98), 0x00000000, GENMASK(31, 0));
	reg_rmw(sregs + CML_REG(0xb8), 0x00000000, GENMASK(15, 14));
	reg_rmw(sregs + LANEX_REG(lane, 0x2c), 0x00000000, GENMASK(23, 22));
	return delay;
}

static void kserdes_rx_calibration_phyb(struct kserdes_config *sc, u32 lane,
					struct kserdes_lane_ofs *lofs,
					struct kserdes_lane_dlev_out *ldlevo)
{
	struct kserdes_cmp_coef_ofs *ctofs, *ctofs_temp;
	struct kserdes_lane_ofs lofs_temp;
	void __iomem *sregs = sc->regs;
	u32 att, boost, comp_no, att_start, boost_start;
	u32 delay_ovr = 0;
	int dlevavg_temp[6];

	delay_ovr = kserdes_eye_monitor_dll_ovr(sc, lane, 0, 1, 0);
	ldlevo->delay = delay_ovr;

	FINSR(sregs, CML_REG(0x164), 15, 15, 1);

	FINSR(sregs, CML_REG(0x164), 16, 16, 1);
	FINSR(sregs, CML_REG(0x164), 31, 26, (128 + delay_ovr) & 0x3f);
	FINSR(sregs, CML_REG(0x168),  2,  0, (128 + delay_ovr) >> 6);
	FINSR(sregs, LANEX_REG(lane, 0x9c), 1, 1, 0);
	FINSR(sregs, LANEX_REG(lane, 0x9c), 0, 0, 0);

	att_start = (kserdes_readl(sregs, LANEX_REG(lane, 0x8c)) >> 8) & 0xf;
	boost_start = (kserdes_readl(sregs, LANEX_REG(lane, 0x8c)) >> 12) & 0xf;

	att = _kserdes_read_select_tbus(sregs, lane + 1, 0x10);
	boost = (att >> 8) & 0xf;
	att = (att >> 4) & 0xf;

	FINSR(sregs, LANEX_REG(lane, 0x8c), 11, 8, att);
	FINSR(sregs, LANEX_REG(lane, 0x8c), 15, 12, boost);

	dlevavg_temp[0] = kserdes_set_dlev_patt_adapt(sc, lane, 0x71, lofs);
	dlevavg_temp[1] = kserdes_set_dlev_patt_adapt(sc, lane, 0x61, lofs);
	dlevavg_temp[2] = kserdes_set_dlev_patt_adapt(sc, lane, 0x79, lofs);
	dlevavg_temp[3] = kserdes_set_dlev_patt_adapt(sc, lane, 0x75, lofs);
	dlevavg_temp[4] = kserdes_set_dlev_patt_adapt(sc, lane, 0x73, lofs);
	dlevavg_temp[5] = kserdes_set_dlev_patt_adapt(sc, lane, 0x70, lofs);

	ldlevo->coef_vals[0] = (dlevavg_temp[0] - dlevavg_temp[1]) /  2;
	ldlevo->coef_vals[1] = (dlevavg_temp[0] - dlevavg_temp[2]) / -2;
	ldlevo->coef_vals[2] = (dlevavg_temp[0] - dlevavg_temp[3]) / -2;
	ldlevo->coef_vals[3] = (dlevavg_temp[0] - dlevavg_temp[4]) / -2;
	ldlevo->coef_vals[4] = (dlevavg_temp[0] - dlevavg_temp[5]) /  2;

	ldlevo->coef_vals[0] = ldlevo->coef_vals[0] -
					ldlevo->coef_vals[0] / 3;

	for (comp_no = 1; comp_no < 5; comp_no++) {
		ctofs = &lofs->ct_ofs[comp_no];
		ctofs_temp = &lofs_temp.ct_ofs[comp_no];

		ctofs_temp->cmp = ctofs->cmp;

		if ((comp_no == 1) || (comp_no == 3)) {
			ctofs_temp->coef1 = ldlevo->coef_vals[0] + ctofs->coef1;
			ctofs_temp->coef2 = ldlevo->coef_vals[1] + ctofs->coef2;
			ctofs_temp->coef3 = ldlevo->coef_vals[2] + ctofs->coef3;
			ctofs_temp->coef4 = ldlevo->coef_vals[3] + ctofs->coef4;
			ctofs_temp->coef5 = ldlevo->coef_vals[4] + ctofs->coef5;
		} else {
			ctofs_temp->coef1 = ctofs->coef1;
			ctofs_temp->coef2 = ctofs->coef2;
			ctofs_temp->coef3 = ctofs->coef3;
			ctofs_temp->coef4 = ctofs->coef4;
			ctofs_temp->coef5 = ctofs->coef5;
		}

		_kserdes_set_ofs(sregs, lane, comp_no, ctofs_temp);
	}

	FINSR(sregs, LANEX_REG(lane, 0x8c), 11, 8, att_start);
	FINSR(sregs, LANEX_REG(lane, 0x8c), 15, 12, boost_start);
	FINSR(sregs, LANEX_REG(lane, 0x9c), 1, 1, 1);
	FINSR(sregs, LANEX_REG(lane, 0x9c), 0, 0, 1);
}

static int kserdes_rx_boost_config_phya(struct kserdes_config *sc, u32 lane)
{
	u32 boost_read;
	int ret;
	bool phy_xge = (sc->phy_type == KSERDES_PHY_XGE);

	ret = kserdes_wait_lane_rx_valid(sc, lane);
	if (ret) {
		dev_err(sc->dev,
			"config_phya: wait_lane_rx_valid FAILED %d\n", ret);
		return ret;
	}

	boost_read = _kserdes_read_select_tbus(sc->regs, lane + 1,
					       phy_xge ?  0x10 : 0x11);

	boost_read = (boost_read >> 8) & 0xf;

	if (!boost_read) {
		FINSR(sc->regs, LANEX_REG(lane, 0x2c),  2,  2, 0x1);
		FINSR(sc->regs, LANEX_REG(lane, 0x2c), 18, 12, 0x2);
		FINSR(sc->regs, LANEX_REG(lane, 0x2c),  9,  3, 0x1);
		FINSR(sc->regs, LANEX_REG(lane, 0x2c), 10, 10, 0x1);
		FINSR(sc->regs, LANEX_REG(lane, 0x2c), 10, 10, 0x0);
		FINSR(sc->regs, LANEX_REG(lane, 0x2c),  2,  2, 0x0);
		FINSR(sc->regs, LANEX_REG(lane, 0x2c), 18, 12, 0x0);
		FINSR(sc->regs, LANEX_REG(lane, 0x2c),  9,  3, 0x0);
	}
	return 0;
}

static int kserdes_enable_lane_rx(struct kserdes_config *sc, u32 lane,
				  struct kserdes_lane_ofs *lofs,
				  struct kserdes_lane_dlev_out *ldlevo)
{
	int ret = 0;

	_kserdes_force_signal_detect_high(sc->regs, lane);

	if (!sc->rx_force_enable) {
		ret = _kserdes_wait_lane_sd(sc->regs, lane);
		if (ret) {
			dev_dbg(sc->dev,
				"%p %u init_lane_rx wait sd valid FAILED %d\n",
				sc->regs, lane, ret);
			return ret;
		}

		if ((sc->phy_type == KSERDES_PHY_XGE) ||
		    (sc->link_rate >= KSERDES_LINK_RATE_5G)) {
			if (sc->lane[lane].ctrl_rate == KSERDES_FULL_RATE) {
				ret = kserdes_wait_lane_rx_valid(sc, lane);
				if (ret) {
					dev_err(sc->dev,
						"init_lane_rx wait rx valid FAILED %d\n",
					       ret);
					return ret;
				}
			}
		}

		if (sc->phy_type == KSERDES_PHY_XGE) {
			kserdes_rx_att_boost_config_phyb(sc, lane);
		} else if ((sc->link_rate >= KSERDES_LINK_RATE_5G) &&
			   (sc->lane[lane].ctrl_rate == KSERDES_FULL_RATE)) {
			kserdes_rx_boost_config_phya(sc, lane);
		}
	}

	if (sc->phy_type == KSERDES_PHY_XGE)
		kserdes_rx_calibration_phyb(sc, lane, lofs, ldlevo);

	return ret;
}

static int kserdes_recover_lane_rx(struct kserdes_config *sc, u32 lane,
				   struct kserdes_lane_ofs *lofs,
				   struct kserdes_lane_dlev_out *ldlevo)
{
	int ret;

	_kserdes_force_signal_detect_high(sc->regs, lane);

	if (!sc->rx_force_enable) {
		ret = _kserdes_wait_lane_sd(sc->regs, lane);
		if (ret) {
			dev_dbg(sc->dev,
				"init_lane_rx wait sd valid FAILED %d\n", ret);
			return ret;
		}
		dev_dbg(sc->dev, "recover_lane_rx sig detcected\n");

		if ((sc->phy_type == KSERDES_PHY_XGE) ||
		    (sc->link_rate >= KSERDES_LINK_RATE_5G)) {
			if (sc->lane[lane].ctrl_rate == KSERDES_FULL_RATE) {
				ret = kserdes_wait_lane_rx_valid(sc, lane);
				if (ret) {
					dev_err(sc->dev,
						"init_lane_rx wait rx valid FAILED %d\n",
					       ret);
					return ret;
				}
				dev_dbg(sc->dev, "recover_lane_rx rx valid\n");
			}
		}

		if (sc->phy_type == KSERDES_PHY_XGE) {
			kserdes_rx_att_boost_config_phyb(sc, lane);
		} else if ((sc->link_rate >= KSERDES_LINK_RATE_5G) &&
			   (sc->lane[lane].ctrl_rate == KSERDES_FULL_RATE)) {
			kserdes_rx_boost_config_phya(sc, lane);
		}
	}

	if (sc->phy_type == KSERDES_PHY_XGE)
		kserdes_rx_calibration_phyb(sc, lane, lofs, ldlevo);

	return 0;
}

static int kserdes_sgmii_init(struct kserdes_config *sc)
{
	return kserdes_load_init_fw(sc, ks2_gbe_serdes_firmwares,
				    ARRAY_SIZE(ks2_gbe_serdes_firmwares));
}

static int kserdes_xge_init(struct kserdes_config *sc)
{
	_kserdes_reset(sc->regs);
	return kserdes_load_init_fw(sc, ks2_xgbe_serdes_firmwares,
				    ARRAY_SIZE(ks2_xgbe_serdes_firmwares));
}

static int kserdes_pcie_init(struct kserdes_config *sc)
{
	return kserdes_load_init_fw(sc, ks2_pcie_serdes_firmwares,
				    ARRAY_SIZE(ks2_pcie_serdes_firmwares));
}

int kserdes_phy_enable_rx(struct kserdes_dev *sd, u32 lane)
{
	struct kserdes_config *sc = &sd->sc;
	struct kserdes_ofs *sofs = &sc->sofs;
	struct kserdes_dlev_out dlevo;
	u32 lanes_up_map = 0;
	u32 i = lane;
	int ret;

	ret = kserdes_enable_lane_rx(sc, i, &sofs->lane_ofs[i],
				     &dlevo.lane_dlev_out[i]);
	if (ret)
		dev_dbg(sc->dev, "lane enable rx FAILED %d\n", ret);

	kserdes_clear_wait_after(sc, BIT(i));

	if (sc->phy_type == KSERDES_PHY_XGE) {
		_kserdes_enable_xgmii_port(sc->peripheral_regmap, i);
		kserdes_wait_link_up(sc, BIT(i), &lanes_up_map);
	}

	return 0;
}

int kserdes_phy_reset(struct kserdes_dev *sd, u32 lane)
{
	struct kserdes_config *sc = &sd->sc;
	struct kserdes_ofs *sofs = &sc->sofs;
	struct kserdes_dlev_out dlevo;
	u32 i = lane;
	u32 lanes_up_map = 0;
	int ret;

	ret = kserdes_recover_lane_rx(sc, i, &sofs->lane_ofs[i],
				      &dlevo.lane_dlev_out[i]);

	kserdes_clear_wait_after(sc, BIT(i));

	_kserdes_enable_xgmii_port(sc->peripheral_regmap, i);

	kserdes_wait_link_up(sc, BIT(i), &lanes_up_map);

	dev_dbg(sc->dev, "phy reset: recover lane %u rx\n", i);

	return ret;
}

static int kserdes_provider_lanes_enable_common(struct kserdes_config *sc)
{
	struct kserdes_ofs *sofs = &sc->sofs;
	unsigned long lanes_needed = 0;
	int ret, i;

	if (sc->firmware)
		return 0;

	for_each_enable_lane(sc, i)
		lanes_needed |= BIT(i);

	if (sc->phy_type == KSERDES_PHY_PCIE) {
		kserdes_pcie_lanes_enable(sc);
		return lanes_needed;
	}

	ret = kserdes_lanes_enable_common(sc, sofs);
	if (ret)
		dev_err(sc->dev, "provider lanes enable: FAILED %d\n", ret);

	return ret;
}

int kserdes_provider_init(struct kserdes_dev *sd)
{
	struct kserdes_config *sc = &sd->sc;
	int ret;

	switch (sc->phy_type) {
	case KSERDES_PHY_SGMII:
		ret = kserdes_sgmii_init(sc);
		break;
	case KSERDES_PHY_XGE:
		ret = kserdes_xge_init(sc);
		break;
	case KSERDES_PHY_PCIE:
		ret = kserdes_pcie_init(sc);
		break;
	default:
		ret = -EINVAL;
	}

	if (ret < 0) {
		dev_err(dev, "serdes provider init failed %d\n", ret);
		return ret;
	}

	return kserdes_provider_lanes_enable_common(sc);
}
