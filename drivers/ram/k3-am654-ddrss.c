/*
 * Texas Instruments' AM654 DDRSS driver
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <ram.h>
#include <asm/io.h>
#include <power-domain.h>
#include <dm.h>
#include <asm/arch/k3-am654-ddrss.h>
#include <asm/arch/sys_proto.h>

#define LDELAY 10000

/**
 * struct am654_ddrss_desc - Description of ddrss integration.
 * @dev:		DDRSS device pointer
 * @ddrss_ss_cfg:	DDRSS wrapper logic region base address
 * @ddrss_ctl_cfg:	DDRSS controller region base address
 * @ddrss_phy_cfg:	DDRSS PHY region base address
 * @ddrss_clk:		DDRSS clock description
 * @ddrss_pwrdmn:	DDRSS power domain description
 * @cfg:		Pointer to the SDRAM configuration
 */
struct am654_ddrss_desc {
	struct udevice *dev;
	void __iomem *ddrss_ss_cfg;
	void __iomem *ddrss_ctl_cfg;
	void __iomem *ddrss_phy_cfg;
	struct clk ddrss_clk;
	struct power_domain ddrcfg_pwrdmn;
	struct power_domain ddrdata_pwrdmn;
	struct am654_ddrss_config *cfg;
};

static inline u32 ddrss_readl(void __iomem *addr, unsigned int offset)
{
	return readl(addr + offset);
}

static inline void ddrss_writel(void __iomem *addr, unsigned int offset,
				u32 data)
{
	debug("%s: addr = 0x%p, value = 0x%x\n", __func__, addr + offset, data);
	writel(data, addr + offset);
}

/**
 * wait_on_value() - Wait for a value to reflect in a register
 * @read_bit_mask:	Mask within the read register
 * @match_value:	value to match from the mask bits
 * @read_addr:		Register address from which value is read
 * @bound:		no. of times the value is read from the register
 *
 */
static u32 wait_on_value(u32 read_bit_mask, u32 match_value, void *read_addr,
			 u32 bound)
{
	u32 i = 0, val;

	do {
		++i;
		val = readl((u32)read_addr) & read_bit_mask;
		if (val == match_value)
			return -ETIMEDOUT;
		if (i == bound)
			return 0;
	} while (1);
}

#define ddrss_ctl_writel(off, val) ddrss_writel(ddrss->ddrss_ctl_cfg, off, val)
#define ddrss_ctl_readl(off) ddrss_readl(ddrss->ddrss_ctl_cfg, off)

/**
 * am654_ddrss_ctrl_configuration() - Configure Controller specific registers
 * @dev:		corresponding ddrss device
 */
static void am654_ddrss_ctrl_configuration(struct am654_ddrss_desc *ddrss)
{
	u32 val;

	debug("%s: DDR controller register configuration started\n", __func__);

	ddrss_ctl_writel(DDRSS_DDRCTL_MSTR, 0x41040010);
	ddrss_ctl_writel(DDRSS_DDRCTL_RFSHCTL0, 0x00210070);
	ddrss_ctl_writel(DDRSS_DDRCTL_RFSHTMG, 0x00510075);

	ddrss_ctl_writel(DDRSS_DDRCTL_ECCCFG0, 0x0);
	ddrss_ctl_writel(DDRSS_DDRCTL_CRCPARCTL1, 0x1A000000);
	ddrss_ctl_writel(DDRSS_DDRCTL_CRCPARCTL2, 0x0048051E);

	ddrss_ctl_writel(DDRSS_DDRCTL_INIT0, 0x400100A3);
	ddrss_ctl_writel(DDRSS_DDRCTL_INIT1, 0x00420000);
	ddrss_ctl_writel(DDRSS_DDRCTL_INIT3, 0x00100501);
	ddrss_ctl_writel(DDRSS_DDRCTL_INIT4, 0x00000020);
	ddrss_ctl_writel(DDRSS_DDRCTL_INIT5, 0x00100000);
	ddrss_ctl_writel(DDRSS_DDRCTL_INIT6, 0x00000480);
	ddrss_ctl_writel(DDRSS_DDRCTL_INIT7, 0x00000097);

	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG0, 0x0b0a160b);
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG1, 0x00020310);
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG2, 0x0506040a);
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG3, 0x0000400C);
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG4, 0x05020205);
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG5, 0x04040302);
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG8, 0x02020C04);
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG9, 0x00020208);
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG11, 0x1005010E);
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG12, 0x00000008);
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG13, 0x00000000);
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG15, 0x00000035);
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG17, 0x00000000);

	ddrss_ctl_writel(DDRSS_DDRCTL_ZQCTL0, 0x21000040);
	ddrss_ctl_writel(DDRSS_DDRCTL_ZQCTL1, 0x00027bc8);

	ddrss_ctl_writel(DDRSS_DDRCTL_DFITMG0, 0x04878206);
	ddrss_ctl_writel(DDRSS_DDRCTL_DFITMG1, 0x00060606);
	ddrss_ctl_writel(DDRSS_DDRCTL_DFITMG2, 0x00000504);

	ddrss_ctl_writel(DDRSS_DDRCTL_ADDRMAP0, 0x001F1F1F);
	ddrss_ctl_writel(DDRSS_DDRCTL_ADDRMAP1, 0x003f0808);
	ddrss_ctl_writel(DDRSS_DDRCTL_ADDRMAP2, 0x00000000);
	ddrss_ctl_writel(DDRSS_DDRCTL_ADDRMAP3, 0x00000000);
	ddrss_ctl_writel(DDRSS_DDRCTL_ADDRMAP4, 0x00001f1f);
	ddrss_ctl_writel(DDRSS_DDRCTL_ADDRMAP5, 0x08080808);
	ddrss_ctl_writel(DDRSS_DDRCTL_ADDRMAP6, 0x08080808);
	ddrss_ctl_writel(DDRSS_DDRCTL_ADDRMAP7, 0x00000f0f);
	ddrss_ctl_writel(DDRSS_DDRCTL_ADDRMAP8, 0x00000a0a);
	ddrss_ctl_writel(DDRSS_DDRCTL_ADDRMAP9, 0x0);
	ddrss_ctl_writel(DDRSS_DDRCTL_ADDRMAP10, 0x0);
	ddrss_ctl_writel(DDRSS_DDRCTL_ADDRMAP11, 0x001f1f00);

	ddrss_ctl_writel(DDRSS_DDRCTL_ODTCFG, 0x06000608);
	ddrss_ctl_writel(DDRSS_DDRCTL_ODTMAP, 0x00000001);

	/* Disable refreshes */
	val = ddrss_ctl_readl(DDRSS_DDRCTL_RFSHCTL3);
	val |= 0x01;
	ddrss_ctl_writel(DDRSS_DDRCTL_RFSHCTL3, val);

	debug("%s: DDR controller configuration completed\n", __func__);
}

#define ddrss_phy_writel(off, val)					\
	do {								\
		ddrss_writel(ddrss->ddrss_phy_cfg, off, val);		\
		sdelay(10);	/* Delay at least 20 clock cycles */	\
	} while (0)

#define ddrss_phy_readl(off)						\
	({								\
		u32 val = ddrss_readl(ddrss->ddrss_phy_cfg, off);	\
		sdelay(10);	/* Delay at least 20 clock cycles */	\
		val;							\
	})

/**
 * am654_ddrss_phy_configuration() - Configure PHY specific registers
 * @dev:		corresponding ddrss device
 */
static void am654_ddrss_phy_configuration(struct am654_ddrss_desc *ddrss)
{
	debug("%s: DDR phy register configuration started\n", __func__);

	ddrss_phy_writel(DDRSS_DDRPHY_PGCR1, 0x020046C0);
	ddrss_phy_writel(DDRSS_DDRPHY_PGCR2, 0x00F09f60);
	ddrss_phy_writel(DDRSS_DDRPHY_PGCR3, 0x55AA0080);
	ddrss_phy_writel(DDRSS_DDRPHY_PGCR6, 0x00013001);

	ddrss_phy_writel(DDRSS_DDRPHY_PTR3, 0x00061A80);
	ddrss_phy_writel(DDRSS_DDRPHY_PTR4, 0x000000E0);
	ddrss_phy_writel(DDRSS_DDRPHY_PTR5, 0x00027100);
	ddrss_phy_writel(DDRSS_DDRPHY_PTR6, 0x04000320);

	ddrss_phy_writel(DDRSS_DDRPHY_PLLCR0, 0x021c4000);

	ddrss_phy_writel(DDRSS_DDRPHY_DXCCR, 0x00000038);
	ddrss_phy_writel(DDRSS_DDRPHY_DSGCR, 0x02A0C129);

	ddrss_phy_writel(DDRSS_DDRPHY_DCR, 0x0000040C);

	ddrss_phy_writel(DDRSS_DDRPHY_DTPR0, 0x04160905);
	ddrss_phy_writel(DDRSS_DDRPHY_DTPR1, 0x28140000);
	ddrss_phy_writel(DDRSS_DDRPHY_DTPR2, 0x00040300);
	ddrss_phy_writel(DDRSS_DDRPHY_DTPR3, 0x02800000);
	ddrss_phy_writel(DDRSS_DDRPHY_DTPR4, 0x00ea0704);
	ddrss_phy_writel(DDRSS_DDRPHY_DTPR5, 0x001f0905);
	ddrss_phy_writel(DDRSS_DDRPHY_DTPR6, 0x00000505);

	ddrss_phy_writel(DDRSS_DDRPHY_ZQCR, 0x008A2A58);
	ddrss_phy_writel(DDRSS_DDRPHY_ZQ0PR0, 0x000077DD);
	ddrss_phy_writel(DDRSS_DDRPHY_ZQ1PR0, 0x000077DD);
	ddrss_phy_writel(DDRSS_DDRPHY_ZQ2PR0, 0x000077DD);
	ddrss_phy_writel(DDRSS_DDRPHY_ZQ3PR0, 0x000077DD);

	ddrss_phy_writel(DDRSS_DDRPHY_MR0, 0x00000010);
	ddrss_phy_writel(DDRSS_DDRPHY_MR1, 0x00000501);

	ddrss_phy_writel(DDRSS_DDRPHY_MR2, 0x00000000);
	ddrss_phy_writel(DDRSS_DDRPHY_MR3, 0x00000020);
	ddrss_phy_writel(DDRSS_DDRPHY_MR4, 0x00000000);
	ddrss_phy_writel(DDRSS_DDRPHY_MR5, 0x00000480);
	ddrss_phy_writel(DDRSS_DDRPHY_MR6, 0x0000097);

	ddrss_phy_writel(DDRSS_DDRPHY_VTCR0, 0xF3C32017);

	ddrss_phy_writel(DDRSS_DDRPHY_DX8SL0PLLCR0, 0x021c4000);
	ddrss_phy_writel(DDRSS_DDRPHY_DX8SL1PLLCR0, 0x021c4000);
	ddrss_phy_writel(DDRSS_DDRPHY_DX8SL2PLLCR0, 0x021c4000);

	ddrss_phy_writel(DDRSS_DDRPHY_DTCR0, 0x8000B1c7);
	ddrss_phy_writel(DDRSS_DDRPHY_DTCR1, 0x00010236);

	ddrss_phy_writel(DDRSS_DDRPHY_ACIOCR5, 0x04800000);

	ddrss_phy_writel(DDRSS_DDRPHY_IOVCR0, 0x0F0C0C0C);

	ddrss_phy_writel(DDRSS_DDRPHY_DX4GCR0, 0x40703260);
	ddrss_phy_writel(DDRSS_DDRPHY_DX4GCR1, 0x55556000);
	ddrss_phy_writel(DDRSS_DDRPHY_DX4GCR2, 0xaaaa0000);
	ddrss_phy_writel(DDRSS_DDRPHY_DX4GCR3, 0xffe18587);

	ddrss_phy_writel(DDRSS_DDRPHY_DX0GCR4, 0x0E00c93C);
	ddrss_phy_writel(DDRSS_DDRPHY_DX1GCR4, 0x0E00c93C);
	ddrss_phy_writel(DDRSS_DDRPHY_DX2GCR4, 0x0E00c93C);
	ddrss_phy_writel(DDRSS_DDRPHY_DX3GCR4, 0x0E00c93C);

	ddrss_phy_writel(DDRSS_DDRPHY_PGCR5, 0x01010004);
	ddrss_phy_writel(DDRSS_DDRPHY_DX0GCR5, 0x00000049);
	ddrss_phy_writel(DDRSS_DDRPHY_DX1GCR5, 0x00000049);
	ddrss_phy_writel(DDRSS_DDRPHY_DX2GCR5, 0x00000049);
	ddrss_phy_writel(DDRSS_DDRPHY_DX3GCR5, 0x00000049);

	ddrss_phy_writel(DDRSS_DDRPHY_RANKIDR, 0);

	ddrss_phy_writel(DDRSS_DDRPHY_DX0GTR0, 0x00020002);
	ddrss_phy_writel(DDRSS_DDRPHY_DX1GTR0, 0x00020002);
	ddrss_phy_writel(DDRSS_DDRPHY_DX2GTR0, 0x00020002);
	ddrss_phy_writel(DDRSS_DDRPHY_DX3GTR0, 0x00020002);
	ddrss_phy_writel(DDRSS_DDRPHY_ODTCR, 0x00010000);

	ddrss_phy_writel(DDRSS_DDRPHY_DX8SL0IOCR, 0x04800000);
	ddrss_phy_writel(DDRSS_DDRPHY_DX8SL1IOCR, 0x04800000);
	ddrss_phy_writel(DDRSS_DDRPHY_DX8SL2IOCR, 0x04800000);

	ddrss_phy_writel(DDRSS_DDRPHY_DX8SL0DXCTL2, 0x00141830);
	ddrss_phy_writel(DDRSS_DDRPHY_DX8SL1DXCTL2, 0x00141830);
	ddrss_phy_writel(DDRSS_DDRPHY_DX8SL2DXCTL2, 0x00141830);

	debug("%s: DDR phy register configuration completed\n", __func__);
}

static int __phy_builtin_init_routine(struct am654_ddrss_desc *ddrss,
				      u32 init_value, u32 sts_mask,
				      u32 err_mask)
{
	int ret;

	ddrss_phy_writel(DDRSS_DDRPHY_PIR, init_value | PIR_INIT_MASK);

	sdelay(5);	/* Delay at least 10 clock cycles */

	if (!wait_on_value(sts_mask, sts_mask,
			   ddrss->ddrss_phy_cfg + DDRSS_DDRPHY_PGSR0, LDELAY))
		return -ETIMEDOUT;

	sdelay(16);	/* Delay at least 32 clock cycles */

	ret = ddrss_phy_readl(DDRSS_DDRPHY_PGSR0);
	debug("%s: PGSR0 val = 0x%x\n", __func__, ret);
	if (ret & err_mask)
		return -EINVAL;

	return 0;
}

int write_leveling(struct am654_ddrss_desc *ddrss)
{
	int ret;

	debug("%s: Write leveling started\n", __func__);

	ret = __phy_builtin_init_routine(ddrss, 0x200, PGSR0_WLDONE_MASK,
					 PGSR0_WLERR_MASK);
	if (ret) {
		if (ret == -ETIMEDOUT)
			printf("%s: ERROR: Write leveling timedout\n",
			       __func__);
		else
			printf("%s:ERROR: Write leveling failed\n", __func__);
		return ret;
	}

	debug("%s: Write leveling completed\n", __func__);
	return 0;
}

int read_dqs_training(struct am654_ddrss_desc *ddrss)
{
	int ret;

	debug("%s: Read DQS training started\n", __func__);

	ret = __phy_builtin_init_routine(ddrss, 0x400, PGSR0_QSGDONE_MASK,
					 PGSR0_QSGERR_MASK);
	if (ret) {
		if (ret == -ETIMEDOUT)
			printf("%s: ERROR: Read DQS timedout\n", __func__);
		else
			printf("%s:ERROR: Read DQS Gate training failed\n",
			       __func__);
		return ret;
	}

	debug("%s: Read DQS training completed\n", __func__);
	return 0;
}

int rest_training(struct am654_ddrss_desc *ddrss)
{
	int ret;
	u32 val;
	u32 dgsl0, dgsl1, dgsl2, dgsl3, rddly, rd2wr_wr2rd;

	debug("%s: Rest of the training started\n", __func__);

	debug("%s: Write Leveling adjustment\n", __func__);
	ret = __phy_builtin_init_routine(ddrss, 0x800, PGSR0_WLADONE_MASK,
					 PGSR0_WLAERR_MASK);
	if (ret) {
		if (ret == -ETIMEDOUT)
			printf("%s:ERROR: Write Leveling adjustment timedout\n",
			       __func__);
		else
			printf("%s: ERROR: Write Leveling adjustment failed\n",
			       __func__);
		return ret;
	}

	debug("%s: Read Deskew adjustment\n", __func__);
	ret = __phy_builtin_init_routine(ddrss, 0x1000, PGSR0_RDDONE_MASK,
					 PGSR0_RDERR_MASK);
	if (ret) {
		if (ret == -ETIMEDOUT)
			printf("%s: ERROR: Read Deskew timedout\n", __func__);
		else
			printf("%s: ERROR: Read Deskew failed\n", __func__);
		return ret;
	}

	debug("%s: Write Deskew adjustment\n", __func__);
	ret = __phy_builtin_init_routine(ddrss, 0x2000, PGSR0_WDDONE_MASK,
					 PGSR0_WDERR_MASK);
	if (ret) {
		if (ret == -ETIMEDOUT)
			printf("%s: ERROR: Write Deskew timedout\n", __func__);
		else
			printf("%s: ERROR: Write Deskew failed\n", __func__);
		return ret;
	}

	debug("%s: Read Eye training\n", __func__);
	ret = __phy_builtin_init_routine(ddrss, 0x4000, PGSR0_REDONE_MASK,
					 PGSR0_REERR_MASK);
	if (ret) {
		if (ret == -ETIMEDOUT)
			printf("%s: ERROR: Read Eye training timedout\n",
			       __func__);
		else
			printf("%s: ERROR: Read Eye training failed\n",
			       __func__);
		return ret;
	}

	debug("%s: Write Eye training\n", __func__);
	ret = __phy_builtin_init_routine(ddrss, 0x8000, PGSR0_WEDONE_MASK,
					 PGSR0_WEERR_MASK);
	if (ret) {
		if (ret == -ETIMEDOUT)
			printf("%s: ERROR: Write Eye training timedout\n",
			       __func__);
		else
			printf("%s: ERROR: Write Eye training failed\n",
			       __func__);
		return ret;
	}

	debug("%s: VREF training\n", __func__);
	ret = __phy_builtin_init_routine(ddrss, 0x20000, PGSR0_VDONE_MASK,
					 PGSR0_VERR_MASK);
	if (ret) {
		if (ret == -ETIMEDOUT)
			printf("%s: ERROR: VREF training timedout\n", __func__);
		else
			printf("%s: ERROR: VREF training failed\n", __func__);
		return ret;
	}

	ddrss_phy_writel(DDRSS_DDRPHY_RANKIDR, 0x00000000);
	dgsl0 = (ddrss_phy_readl(DDRSS_DDRPHY_DX0GTR0) & 0x1F) >> 2;
	dgsl1 = (ddrss_phy_readl(DDRSS_DDRPHY_DX1GTR0) & 0x1F) >> 2;
	dgsl2 = (ddrss_phy_readl(DDRSS_DDRPHY_DX2GTR0) & 0x1F) >> 2;
	dgsl3 = (ddrss_phy_readl(DDRSS_DDRPHY_DX3GTR0) & 0x1F) >> 2;

	rddly = dgsl0;
	if (dgsl1 < rddly)
		rddly = dgsl1;
	if (dgsl2 < rddly)
		rddly = dgsl2;
	if (dgsl3 < rddly)
		rddly = dgsl3;

	rddly += 5;

	/* Update rddly based on dgsl values */
	val = (ddrss_phy_readl(DDRSS_DDRPHY_DX0GCR0) & ~0xF00000);
	val |= (rddly << 20);
	ddrss_phy_writel(DDRSS_DDRPHY_DX0GCR0, val);

	val = (ddrss_phy_readl(DDRSS_DDRPHY_DX1GCR0) & ~0xF00000);
	val |= (rddly << 20);
	ddrss_phy_writel(DDRSS_DDRPHY_DX1GCR0, val);

	val = (ddrss_phy_readl(DDRSS_DDRPHY_DX2GCR0) & ~0xF00000);
	val |= (rddly << 20);
	ddrss_phy_writel(DDRSS_DDRPHY_DX2GCR0, val);

	val = (ddrss_phy_readl(DDRSS_DDRPHY_DX3GCR0) & ~0xF00000);
	val |= (rddly << 20);
	ddrss_phy_writel(DDRSS_DDRPHY_DX3GCR0, val);

	/*
	 * Add system latency derived from training back into rd2wr and wr2rd
	 * rd2wr = RL + BL/2 + 1 + WR_PREAMBLE - WL + max(DXnGTR0.DGSL) / 2
	 * wr2rd = CWL + PL + BL/2 + tWTR_L + max(DXnGTR0.DGSL) / 2
	 */

	/* Select rank 0 */
	ddrss_phy_writel(DDRSS_DDRPHY_RANKIDR, 0x00000000);

	dgsl0 = (ddrss_phy_readl(DDRSS_DDRPHY_DX0GTR0) & 0x1F);
	dgsl1 = (ddrss_phy_readl(DDRSS_DDRPHY_DX1GTR0) & 0x1F);
	dgsl2 = (ddrss_phy_readl(DDRSS_DDRPHY_DX2GTR0) & 0x1F);
	dgsl3 = (ddrss_phy_readl(DDRSS_DDRPHY_DX3GTR0) & 0x1F);

	/* Find maximum value across all bytes */
	rd2wr_wr2rd = dgsl0;
	if (dgsl1 > rd2wr_wr2rd)
		rd2wr_wr2rd = dgsl1;
	if (dgsl2 > rd2wr_wr2rd)
		rd2wr_wr2rd = dgsl2;
	if (dgsl3 > rd2wr_wr2rd)
		rd2wr_wr2rd = dgsl3;

	rd2wr_wr2rd >>= 1;

	/* Now add in adjustment to DRAMTMG2 bit fields for rd2wr and wr2rd */
	/* Clear VSWCTL.sw_done */
	ddrss_ctl_writel(DDRSS_DDRCTL_SWCTL,
			 ddrss_ctl_readl(DDRSS_DDRCTL_SWCTL) & ~0x1);
	/* Adjust rd2wr */
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG2,
			 ddrss_ctl_readl(DDRSS_DDRCTL_DRAMTMG2) +
			 (rd2wr_wr2rd << 8));
	/* Adjust wr2rd */
	ddrss_ctl_writel(DDRSS_DDRCTL_DRAMTMG2,
			 ddrss_ctl_readl(DDRSS_DDRCTL_DRAMTMG2) +
			 rd2wr_wr2rd);
	/* Set VSWCTL.sw_done */
	ddrss_ctl_writel(DDRSS_DDRCTL_SWCTL,
			 ddrss_ctl_readl(DDRSS_DDRCTL_SWCTL) | 0x1);
	/* Wait until settings are applied */
	while (!(ddrss_ctl_readl(DDRSS_DDRCTL_SWSTAT) & 0x1)) {
		/* Do nothing */
	};

	debug("%s: Rest of the training completed\n", __func__);
	return 0;
}

/**
 * am654_ddrss_init() - Initialization sequence for enabling the SDRAM
 *			device attached to ddrss.
 * @dev:		corresponding ddrss device
 *
 * Does all the initialization sequence that is required to get attached
 * ddr in a working state. After this point, ddr should be accessible.
 * Return: 0 if all went ok, else corresponding error message.
 */
static int am654_ddrss_init(struct am654_ddrss_desc *ddrss)
{
	int ret;

	debug("%s(ddrss=%p)\n", __func__, ddrss);

	ddrss_writel(ddrss->ddrss_ss_cfg, DDRSS_V2H_CTL_REG, 0x000073FF);

	am654_ddrss_ctrl_configuration(ddrss);

	/* Release the reset to the controller */
	clrbits_le32(ddrss->ddrss_ss_cfg + DDRSS_SS_CTL_REG,
		     SS_CTL_REG_CTL_ARST_MASK);

	am654_ddrss_phy_configuration(ddrss);

	ret = __phy_builtin_init_routine(ddrss, PIR_PHY_INIT, 0x1, 0);
	if (ret) {
		dev_err(ddrss->dev, "PHY initialization failed %d\n", ret);
		return ret;
	}

	ret = __phy_builtin_init_routine(ddrss, PIR_DRAM_INIT,
					 PGSR0_DRAM_INIT_MASK, 0);
	if (ret) {
		dev_err(ddrss->dev, "DRAM initialization failed %d\n", ret);
		return ret;
	}

	ret = write_leveling(ddrss);
	if (ret)
		return ret;

	ret = read_dqs_training(ddrss);
	if (ret)
		return ret;

	ret = rest_training(ddrss);
	if (ret)
		return ret;

	/* Enabling refreshes after training is done */
	ddrss_ctl_writel(DDRSS_DDRCTL_RFSHCTL3,
			 ddrss_ctl_readl(DDRSS_DDRCTL_RFSHCTL3) & ~0x1);

	/* Disable PUBMODE after training is done */
	ddrss_phy_writel(DDRSS_DDRPHY_PGCR1,
			 ddrss_phy_readl(DDRSS_DDRPHY_PGCR1) & ~0x40);

	return 0;
}

/**
 * am654_ddrss_power_on() - Enable power and clocks for ddrss
 * @dev:	corresponding ddrss device
 *
 * Tries to enable all the corresponding clocks to the ddrss and sets it
 * to the right frequency and then power on the ddrss.
 * Return: 0 if all went ok, else corresponding error message.
 */
static int am654_ddrss_power_on(struct am654_ddrss_desc *ddrss)
{
	int ret;

	debug("%s(ddrss=%p)\n", __func__, ddrss);

	ret = clk_enable(&ddrss->ddrss_clk);
	if (ret) {
		dev_err(ddrss->dev, "clk_enable() failed: %d\n", ret);
		return ret;
	}

	ret = power_domain_on(&ddrss->ddrcfg_pwrdmn);
	if (ret) {
		dev_err(ddrss->dev, "power_domain_on() failed: %d\n", ret);
		return ret;
	}

	ret = power_domain_on(&ddrss->ddrdata_pwrdmn);
	if (ret) {
		dev_err(ddrss->dev, "power_domain_on() failed: %d\n", ret);
		return ret;
	}

	/* VTT enable */
	writel(0x68ef3491, 0x4301D008);
	writel(0xD172BC5A, 0x4301D00C);

	writel(0x20007, 0x4301c040);

	ret = readl(0x42110010);
	ret &= ~0x10000000;
	writel(ret, 0x42110010);
	writel(0x10000000, 0x42110014);

	debug("VTT regulator enabled\n");

	return 0;
}

/**
 * am654_ddrss_ofdata_to_priv() - generate private data from device tree
 * @dev:	corresponding ddrss device
 *
 * Return: 0 if all went ok, else corresponding error message.
 */
static int am654_ddrss_ofdata_to_priv(struct udevice *dev)
{
	struct am654_ddrss_desc *ddrss = dev_get_priv(dev);
	phys_addr_t reg;
	int ret;

	debug("%s(dev=%p)\n", __func__, dev);

	ret = clk_get_by_index(dev, 0, &ddrss->ddrss_clk);
	if (ret) {
		dev_err(dev, "clk_get failed: %d\n", ret);
		return ret;
	}

	ret = power_domain_get_by_index(dev, &ddrss->ddrcfg_pwrdmn, 0);
	if (ret) {
		dev_err(dev, "power_domain_get() failed: %d\n", ret);
		return ret;
	}

	ret = power_domain_get_by_index(dev, &ddrss->ddrdata_pwrdmn, 1);
	if (ret) {
		dev_err(dev, "power_domain_get() failed: %d\n", ret);
		return ret;
	}

	reg = devfdt_get_addr_name(dev, "ss");
	if (reg == FDT_ADDR_T_NONE) {
		dev_err(dev, "No reg property for DDRSS wrapper logic\n");
		return -EINVAL;
	}
	ddrss->ddrss_ss_cfg = (void *)reg;

	reg = devfdt_get_addr_name(dev, "ctl");
	if (reg == FDT_ADDR_T_NONE) {
		dev_err(dev, "No reg property for Controller region\n");
		return -EINVAL;
	}
	ddrss->ddrss_ctl_cfg = (void *)reg;

	reg = devfdt_get_addr_name(dev, "phy");
	if (reg == FDT_ADDR_T_NONE) {
		dev_err(dev, "No reg property for PHY region\n");
		return -EINVAL;
	}
	ddrss->ddrss_phy_cfg = (void *)reg;

	return 0;
}

/**
 * am654_ddrss_probe() - Basic probe
 * @dev:	corresponding ddrss device
 *
 * Return: 0 if all went ok, else corresponding error message
 */
static int am654_ddrss_probe(struct udevice *dev)
{
	struct am654_ddrss_desc *ddrss = dev_get_priv(dev);
	int ret;

	debug("%s(dev=%p)\n", __func__, dev);

	ret = am654_ddrss_ofdata_to_priv(dev);
	if (ret)
		return ret;

	ddrss->dev = dev;
	ret = am654_ddrss_power_on(ddrss);
	if (ret)
		return ret;

	ret = am654_ddrss_init(ddrss);

	return ret;
}

static int am654_ddrss_get_info(struct udevice *dev, struct ram_info *info)
{
	return 0;
}

static struct ram_ops am654_ddrss_ops = {
	.get_info = am654_ddrss_get_info,
};

static const struct udevice_id am654_ddrss_ids[] = {
	{ .compatible = "ti,am654-ddrss" },
	{ }
};

U_BOOT_DRIVER(am654_ddrss) = {
	.name = "am654_ddrss",
	.id = UCLASS_RAM,
	.of_match = am654_ddrss_ids,
	.ops = &am654_ddrss_ops,
	.probe = am654_ddrss_probe,
	.priv_auto_alloc_size = sizeof(struct am654_ddrss_desc),
};
