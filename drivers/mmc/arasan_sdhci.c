/*
 * (C) Copyright 2013 - 2015 Xilinx, Inc.
 *
 * Xilinx Zynq SD Host Controller Interface
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <clk.h>
#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <generic-phy.h>
#include <libfdt.h>
#include <malloc.h>
#include "mmc_private.h"
#include <power-domain.h>
#include <sdhci.h>

#define SDHCI_HOST_CTRL2	0x3E
#define SDHCI_CTRL2_MODE_MASK	0x7
#define SDHCI_18V_SIGNAL	0x8
#define SDHCI_CTRL_EXEC_TUNING	0x0040
#define SDHCI_CTRL_TUNED_CLK	0x80
#define SDHCI_TUNING_LOOP_COUNT	40

#define PHY_CLK_TOO_SLOW_HZ 25000000

DECLARE_GLOBAL_DATA_PTR;

struct arasan_sdhci_plat {
	struct mmc_config cfg;
	struct mmc mmc;
	unsigned int f_max;
	struct phy phy;
	bool phy_is_on;
};

static int arasan_sdhci_execute_tuning(struct mmc *mmc, u8 opcode)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	u32 ctrl;
	struct sdhci_host *host;
	char tuning_loop_counter = SDHCI_TUNING_LOOP_COUNT;

	debug("%s\n", __func__);

	host = mmc->priv;

	ctrl = sdhci_readw(host, SDHCI_HOST_CTRL2);
	ctrl |= SDHCI_CTRL_EXEC_TUNING;
	sdhci_writew(host, ctrl, SDHCI_HOST_CTRL2);

	sdhci_writel(host, SDHCI_INT_DATA_AVAIL, SDHCI_INT_ENABLE);
	sdhci_writel(host, SDHCI_INT_DATA_AVAIL, SDHCI_SIGNAL_ENABLE);

	do {
		cmd.cmdidx = opcode;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 0;

		data.blocksize = 64;
		data.blocks = 1;
		data.flags = MMC_DATA_READ;

		if (tuning_loop_counter-- == 0)
			break;

		if (cmd.cmdidx == MMC_CMD_SEND_TUNING_BLOCK_HS200 &&
		    mmc->bus_width == 8)
			data.blocksize = 128;

		sdhci_writew(host, SDHCI_MAKE_BLKSZ(SDHCI_DEFAULT_BOUNDARY_ARG,
						    data.blocksize),
			     SDHCI_BLOCK_SIZE);
		sdhci_writew(host, data.blocks, SDHCI_BLOCK_COUNT);
		sdhci_writew(host, SDHCI_TRNS_READ, SDHCI_TRANSFER_MODE);

		mmc_send_cmd(mmc, &cmd, NULL);

		ctrl = sdhci_readw(host, SDHCI_HOST_CTRL2);

		if (cmd.cmdidx == MMC_CMD_SEND_TUNING_BLOCK)
			udelay(1);

	} while (ctrl & SDHCI_CTRL_EXEC_TUNING);

	if (tuning_loop_counter < 0) {
		ctrl &= ~SDHCI_CTRL_TUNED_CLK;
		sdhci_writel(host, ctrl, SDHCI_HOST_CTRL2);
	}

	if (!(ctrl & SDHCI_CTRL_TUNED_CLK)) {
		printf("%s:Tuning failed\n", __func__);
		return -1;
	}

	/* Enable only interrupts served by the SD controller */
	sdhci_writel(host, SDHCI_INT_DATA_MASK | SDHCI_INT_CMD_MASK,
		     SDHCI_INT_ENABLE);
	/* Mask all sdhci interrupt sources */
	sdhci_writel(host, 0x0, SDHCI_SIGNAL_ENABLE);

	return 0;
}

static void arasan_sdhci_set_control_reg(struct sdhci_host *host)
{
	struct mmc *mmc = (struct mmc *)host->mmc;
	u32 reg;

	reg = sdhci_readw(host, SDHCI_HOST_CTRL2);
	reg &= ~SDHCI_CTRL2_MODE_MASK;

	if (IS_SD(host->mmc) &&
	    mmc->signal_voltage == MMC_SIGNAL_VOLTAGE_180) {
		reg = sdhci_readw(host, SDHCI_HOST_CTRL2);
		reg |= SDHCI_18V_SIGNAL;
		sdhci_writew(host, reg, SDHCI_HOST_CTRL2);
	}

	switch (mmc->selected_mode) {
	case UHS_SDR50:
	case MMC_HS_52:
		reg |= UHS_SDR50_BUS_SPEED;
		break;
	case UHS_DDR50:
	case MMC_DDR_52:
		reg |= UHS_DDR50_BUS_SPEED;
		break;
	case UHS_SDR104:
	case MMC_HS_200:
		reg |= UHS_SDR104_BUS_SPEED;
		break;
	case MMC_HS_400:
		reg |= UHS_HS400_BUS_SPEED;
		break;
	default:
		reg |= UHS_SDR12_BUS_SPEED;
	}

	sdhci_writew(host, reg, SDHCI_HOST_CTRL2);
}

static void arasan_sdhci_set_ios_post(struct sdhci_host *host)
{
	struct udevice *dev = host->mmc->dev;
	struct arasan_sdhci_plat *plat = dev_get_platdata(dev);
	unsigned int speed = host->mmc->clock;
	u16 clk;

	/* Reset SD Clock Enable */
	clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	clk &= ~SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);

	/* power off phy */
	if (plat->phy_is_on) {
		generic_phy_power_off(&plat->phy);
		plat->phy_is_on = false;
	}
	/* restart clock */
	sdhci_set_clock(host->mmc, speed);
	/* switch phy back on */
	if (speed > PHY_CLK_TOO_SLOW_HZ) {
		generic_phy_power_on(&plat->phy);
		plat->phy_is_on = true;
	}
}

const struct sdhci_ops arasan_ops = {
	.platform_execute_tuning	= &arasan_sdhci_execute_tuning,
	.set_control_reg		= &arasan_sdhci_set_control_reg,
	.set_ios_post			= &arasan_sdhci_set_ios_post,
};

static int arasan_sdhci_probe(struct udevice *dev)
{
	struct arasan_sdhci_plat *plat = dev_get_platdata(dev);
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct sdhci_host *host = dev_get_priv(dev);
	struct power_domain sdhci_pwrdmn;
	struct clk clk;
	unsigned long clock;
	int ret;

	ret = power_domain_get_by_index(dev, &sdhci_pwrdmn, 0);
	if (!ret) {
		power_domain_on(&sdhci_pwrdmn);
	} else if (ret != -ENOENT && ret != -ENODEV && ret != -ENOSYS) {
		dev_err(dev, "power_domain_get() failed: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "clk_xin", &clk);
	if (ret < 0) {
		dev_err(dev, "failed to get clock\n");
		return ret;
	}

	clock = clk_get_rate(&clk);
	if (IS_ERR_VALUE(clock)) {
		dev_err(dev, "failed to get rate\n");
		return clock;
	}
	debug("%s: CLK %ld\n", __func__, clock);

	ret = clk_enable(&clk);
	if (ret && ret != -ENOSYS) {
		dev_err(dev, "failed to enable clock\n");
		return ret;
	}

	if (device_is_compatible(dev, "ti,am654-sdhci-5.1")) {
		/* Get and init phy */
		ret = generic_phy_get_by_name(dev, "phy_arasan", &plat->phy);
		if (ret) {
			pr_err("can't get phy from DT\n");
			return ret;
		}

		ret = generic_phy_init(&plat->phy);
		if (ret) {
			pr_err("couldn't initialize MMC PHY\n");
			return ret;
		}

		plat->phy_is_on = false;
	}

	host->quirks = SDHCI_QUIRK_WAIT_SEND_CMD |
		       SDHCI_QUIRK_BROKEN_R1B;

#ifdef CONFIG_ZYNQ_HISPD_BROKEN
	host->quirks |= SDHCI_QUIRK_NO_HISPD_BIT;
#endif
	if (fdtdec_get_bool(gd->fdt_blob, dev_of_offset(dev),
			    "xlnx,fails-without-test-cd"))
		host->quirks |= SDHCI_QUIRK_FORCE_CD_TEST;

	switch (fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
		"bus-width", 4)) {
	case 8:
		host->host_caps |= MMC_MODE_8BIT;
		break;
	case 4:
		host->host_caps |= MMC_MODE_4BIT;
		break;
	case 1:
		break;
	default:
		printf("Invalid \"bus-width\" value\n");
		return -EINVAL;
	}

	host->max_clk = clock;
	host->mmc->dev = dev;

	host->ops = &arasan_ops;
	ret = sdhci_setup_cfg(&plat->cfg, host, plat->f_max,
			      CONFIG_ARASAN_SDHCI_MIN_FREQ);
	host->mmc = &plat->mmc;
	if (ret)
		return ret;
	host->mmc->priv = host;
	upriv->mmc = host->mmc;

	plat->phy.priv = (void *)&host->mmc->clock;

	return sdhci_probe(dev);
}

static int arasan_sdhci_ofdata_to_platdata(struct udevice *dev)
{
	struct arasan_sdhci_plat *plat = dev_get_platdata(dev);
	struct sdhci_host *host = dev_get_priv(dev);

	host->name = dev->name;
	host->ioaddr = (void *)devfdt_get_addr(dev);

	plat->f_max = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
				"max-frequency", CONFIG_ARASAN_SDHCI_MAX_FREQ);

	return 0;
}

static int arasan_sdhci_bind(struct udevice *dev)
{
	struct arasan_sdhci_plat *plat = dev_get_platdata(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct udevice_id arasan_sdhci_ids[] = {
	{ .compatible = "ti,am654-sdhci-5.1"},
	{ .compatible = "arasan,sdhci-8.9a" },
	{ .compatible = "arasan,sdhci-5.1"  },
	{ }
};

U_BOOT_DRIVER(arasan_sdhci_drv) = {
	.name		= "arasan_sdhci",
	.id		= UCLASS_MMC,
	.of_match	= arasan_sdhci_ids,
	.ofdata_to_platdata = arasan_sdhci_ofdata_to_platdata,
	.ops		= &sdhci_ops,
	.bind		= arasan_sdhci_bind,
	.probe		= arasan_sdhci_probe,
	.priv_auto_alloc_size = sizeof(struct sdhci_host),
	.platdata_auto_alloc_size = sizeof(struct arasan_sdhci_plat),
};
