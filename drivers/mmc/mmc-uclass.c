/*
 * Copyright (C) 2015 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <mmc.h>
#include <dm.h>
#include <dm/lists.h>
#include <dm/root.h>

int mmc_of_parse(const void *fdt, int node, struct mmc_config *cfg)
{
	int val;

	val = fdtdec_get_int(fdt, node, "bus-width", -1);
	if (val < 0) {
		printf("bus-width property missing; assuming 1 bit\n");
		val = 1;
	}

	switch (val) {
	case 0x8:
		cfg->host_caps |= MMC_MODE_8BIT;
	case 0x4:
		cfg->host_caps |= MMC_MODE_4BIT;
		break;
	case 0x1:
		break;
	default:
		printf("error: invalid bus-width property\n");
		return -ENOENT;
	}

	cfg->f_max = fdtdec_get_int(fdt, node, "max-frequency", 52000000);

	if (fdtdec_get_bool(fdt, node, "cap-sd-highspeed"))
		cfg->host_caps |= MMC_MODE_HS;
	if (fdtdec_get_bool(fdt, node, "cap-mmc-highspeed"))
		cfg->host_caps |= MMC_MODE_HS;
	if (fdtdec_get_bool(fdt, node, "mmc-ddr-1_8v"))
		cfg->host_caps |= MMC_MODE_DDR_52MHz;
	if (fdtdec_get_bool(fdt, node, "mmc-hs200-1_8v"))
		cfg->host_caps |= MMC_MODE_HS200;
	if (fdtdec_get_bool(fdt, node, "sd-uhs-sdr104"))
		cfg->host_caps |= MMC_MODE_UHS_SDR104;
	if (fdtdec_get_bool(fdt, node, "sd-uhs-sdr50"))
		cfg->host_caps |= MMC_MODE_UHS_SDR50;
	if (fdtdec_get_bool(fdt, node, "sd-uhs-ddr50"))
		cfg->host_caps |= MMC_MODE_UHS_DDR50;
	if (fdtdec_get_bool(fdt, node, "sd-uhs-sdr25"))
		cfg->host_caps |= MMC_MODE_UHS_SDR25;
	if (fdtdec_get_bool(fdt, node, "sd-uhs-sdr12"))
		cfg->host_caps |= MMC_MODE_UHS_SDR12;

	return 0;
}

struct mmc *mmc_get_mmc_dev(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv;

	if (!device_active(dev))
		return NULL;
	upriv = dev_get_uclass_priv(dev);
	return upriv->mmc;
}

U_BOOT_DRIVER(mmc) = {
	.name	= "mmc",
	.id	= UCLASS_MMC,
};

UCLASS_DRIVER(mmc) = {
	.id		= UCLASS_MMC,
	.name		= "mmc",
	.flags		= DM_UC_FLAG_SEQ_ALIAS,
	.per_device_auto_alloc_size = sizeof(struct mmc_uclass_priv),
};
