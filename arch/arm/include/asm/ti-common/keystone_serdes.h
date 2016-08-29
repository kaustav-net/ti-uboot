/*
 * Texas Instruments Keystone SerDes driver
 * Authors: WingMan Kwok <w-kwok2@ti.com>
 *
 * This is the SerDes Phy driver for Keystone devices. This is
 * required to support PCIe RC functionality based on designware
 * PCIe hardware, gbe and 10gbe found on these devices.
 *
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
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
#ifndef __TI_KEYSTONE_SERDES_H__
#define __TI_KEYSTONE_SERDES_H__

#define KSERDES_SS_OFFSET	0x1fc0
#define MOD_VER_REG		(KSERDES_SS_OFFSET + 0x00)
#define MEM_ADR_REG		(KSERDES_SS_OFFSET + 0x04)
#define MEM_DAT_REG		(KSERDES_SS_OFFSET + 0x08)
#define MEM_DATINC_REG		(KSERDES_SS_OFFSET + 0x0c)
#define CPU_CTRL_REG		(KSERDES_SS_OFFSET + 0x10)
#define LANE_CTRL_STS_REG(x)	(KSERDES_SS_OFFSET + 0x20 + (x * 0x04))
#define LINK_LOSS_WAIT_REG	(KSERDES_SS_OFFSET + 0x30)
#define PLL_CTRL_REG		(KSERDES_SS_OFFSET + 0x34)

#define CMU0_SS_OFFSET		0x0000
#define CMU0_REG(x)		(CMU0_SS_OFFSET + x)

#define LANE0_SS_OFFSET		0x0200
#define LANEX_SS_OFFSET(x)	(LANE0_SS_OFFSET * (x + 1))
#define LANEX_REG(x, y)		(LANEX_SS_OFFSET(x) + y)

#define CML_SS_OFFSET		0x0a00
#define CML_REG(x)		(CML_SS_OFFSET + x)

#define CMU1_SS_OFFSET		0x0c00
#define CMU1_REG(x)		(CMU1_SS_OFFSET + x)

#define PCSR_OFFSET(x)		(x * 0x80)

#define PCSR_TX_CTL(x)		(PCSR_OFFSET(x) + 0x00)
#define PCSR_TX_STATUS(x)	(PCSR_OFFSET(x) + 0x04)
#define PCSR_RX_CTL(x)		(PCSR_OFFSET(x) + 0x08)
#define PCSR_RX_STATUS(x)	(PCSR_OFFSET(x) + 0x0C)

#define XGE_CTRL_OFFSET		0x0c
#define PCIE_PL_GEN2_OFFSET	0x180c

#define reg_rmw(addr, value, mask) \
	writel(((readl(addr) & (~(mask))) | (value & (mask))), (addr))

#define FINSR(base, offset, msb, lsb, val) \
	reg_rmw((base) + (offset), ((val) << (lsb)), GENMASK((msb), (lsb)))

#define FEXTR(val, msb, lsb) \
	(((val) >> (lsb)) & ((1 << ((msb) - (lsb) + 1)) - 1))

#define MOD_VER(serdes) \
	((kserdes_readl(serdes, MOD_VER_REG) >> 16) & 0xffff)

#define PHY_A(serdes) (MOD_VER(serdes) != 0x4eba)

#define FOUR_LANE(serdes) \
	((MOD_VER(serdes) == 0x4eb9) || (MOD_VER(serdes) == 0x4ebd))

#define KSERDES_MAX_LANES	4
#define MAX_COEFS		5
#define MAX_CMP			5
#define OFFSET_SAMPLES		100

#define for_each_cmp(i)	\
	for (i = 1; i < MAX_CMP; i++)

#define CPU_EN			BIT(31)
#define CPU_GO			BIT(30)
#define POR_EN			BIT(29)
#define CPUREG_EN		BIT(28)
#define AUTONEG_CTL		BIT(27)
#define DATASPLIT		BIT(26)
#define LNKTRN_SIG_DET		BIT(8)

#define PLL_ENABLE_1P25G	0xe0000000
#define LANE_CTRL_1P25G		0xf800f8c0
#define XFM_FLUSH_CMD		0x00009c9c

#define ANEG_LINK_CTL_10GKR_MASK	GENMASK(21, 20)
#define ANEG_LINK_CTL_1GKX_MASK		GENMASK(17, 16)
#define ANEG_LINK_CTL_1G10G_MASK \
	(ANEG_LINK_CTL_10GKR_MASK | ANEG_LINK_CTL_1GKX_MASK)

#define ANEG_1G_10G_OPT_MASK		GENMASK(7, 5)

#define SERDES_REG_INDEX		0

#define KSERDES_XFW_MEM_SIZE		SZ_64K
#define KSERDES_XFW_CONFIG_MEM_SIZE	SZ_64
#define KSERDES_XFW_NUM_PARAMS		5

#define KSERDES_XFW_CONFIG_START_ADDR \
	(KSERDES_XFW_MEM_SIZE - KSERDES_XFW_CONFIG_MEM_SIZE)

#define KSERDES_XFW_PARAM_START_ADDR \
	(KSERDES_XFW_MEM_SIZE - (KSERDES_XFW_NUM_PARAMS * 4))

#define LANE_ENABLE(sc, n) ((sc)->lane[n].enable)

enum kserdes_link_rate {
	KSERDES_LINK_RATE_1P25G		=  1250000,
	KSERDES_LINK_RATE_3P125G	=  3125000,
	KSERDES_LINK_RATE_4P9152G	=  4915200,
	KSERDES_LINK_RATE_5G		=  5000000,
	KSERDES_LINK_RATE_6P144G	=  6144000,
	KSERDES_LINK_RATE_6P25G		=  6250000,
	KSERDES_LINK_RATE_7P3728G	=  7372800,
	KSERDES_LINK_RATE_9P8304G	=  9830400,
	KSERDES_LINK_RATE_10G		= 10000000,
	KSERDES_LINK_RATE_10P3125G	= 10312500,
	KSERDES_LINK_RATE_12P5G		= 12500000,
};

enum kserdes_lane_ctrl_rate {
	KSERDES_FULL_RATE,
	KSERDES_HALF_RATE,
	KSERDES_QUARTER_RATE,
};

enum kserdes_phy_type {
	KSERDES_PHY_SGMII,
	KSERDES_PHY_XGE,
	KSERDES_PHY_PCIE,
	KSERDES_PHY_HYPERLINK,
};

struct kserdes_tx_coeff {
	u32	c1;
	u32	c2;
	u32	cm;
	u32	att;
	u32	vreg;
};

struct kserdes_equalizer {
	u32	att;
	u32	boost;
};

struct kserdes_lane_config {
	bool				enable;
	u32				ctrl_rate;
	struct kserdes_tx_coeff		tx_coeff;
	struct kserdes_equalizer	rx_start;
	struct kserdes_equalizer	rx_force;
	bool				loopback;
};

struct kserdes_fw_config {
	bool				on;
	u32				rate;
	u32				link_loss_wait;
	u32				lane_seeds;
	u32				fast_train;
	u32				active_lane;
	u32				c1, c2, cm, attn, boost, dlpf, rxcal;
	u32				lane_config[KSERDES_MAX_LANES];
};

struct kserdes_lane_dlev_out {
	u32 delay;
	int coef_vals[MAX_COEFS];
};

struct kserdes_dlev_out {
	struct kserdes_lane_dlev_out lane_dlev_out[KSERDES_MAX_LANES];
};

struct kserdes_cmp_coef_ofs {
	u32 cmp;
	u32 coef1;
	u32 coef2;
	u32 coef3;
	u32 coef4;
	u32 coef5;
};

struct kserdes_lane_ofs {
	struct kserdes_cmp_coef_ofs ct_ofs[MAX_CMP];
};

struct kserdes_ofs {
	struct kserdes_lane_ofs lane_ofs[KSERDES_MAX_LANES];
};

struct kserdes_config {
	struct device			*dev;
	enum kserdes_phy_type		phy_type;
	u32				lanes;
	void __iomem			*regs;
	struct regmap			*peripheral_regmap;
	struct regmap			*pcsr_regmap;
	const char			*init_fw;
	struct serdes_cfg		*init_cfg;
	int				init_cfg_len;
	enum kserdes_link_rate		link_rate;
	bool				rx_force_enable;
	struct kserdes_lane_config	lane[KSERDES_MAX_LANES];
	struct kserdes_ofs		sofs;
	bool				firmware;
	struct kserdes_fw_config	fw;
};

struct kserdes_phy {
	u32 lane;
};

struct kserdes_dev {
	struct device *dev;
	u32 nphys;
	struct kserdes_phy phys[KSERDES_MAX_LANES];
	struct kserdes_config sc;
};

int kserdes_phy_reset(struct kserdes_dev *sd, u32 lane);
int kserdes_phy_enable_rx(struct kserdes_dev *sd, u32 lane);
int kserdes_provider_init(struct kserdes_dev *sd);

#endif /* __TI_KEYSTONE_SERDES_H__ */
