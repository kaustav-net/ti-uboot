/*
 * PBIAS regulator driver
 *
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <config.h>
#include <asm/io.h>
#include <common.h>
#include <dm.h>
#include <dm/lists.h>
#include <power/regulator.h>

DECLARE_GLOBAL_DATA_PTR;

struct pbias_reg_info {
	u32 enable;
	u32 enable_mask;
	u32 disable_val;
	u32 vmode;
	unsigned int enable_time;
	char *name;
};

struct pbias_regulator_data {
	void *base;
	struct pbias_reg_info *reg_info;
};

struct of_regulator_match {
	const char *name;
	void *driver_data;
};

static const struct pbias_reg_info pbias_mmc_omap2430 = {
	.enable = BIT(1),
	.enable_mask = BIT(1),
	.vmode = BIT(0),
	.disable_val = 0,
	.enable_time = 100,
	.name = "pbias_mmc_omap2430"
};

static const struct pbias_reg_info pbias_sim_omap3 = {
	.enable = BIT(9),
	.enable_mask = BIT(9),
	.vmode = BIT(8),
	.enable_time = 100,
	.name = "pbias_sim_omap3"
};

static const struct pbias_reg_info pbias_mmc_omap4 = {
	.enable = BIT(26) | BIT(22),
	.enable_mask = BIT(26) | BIT(25) | BIT(22),
	.disable_val = BIT(25),
	.vmode = BIT(21),
	.enable_time = 100,
	.name = "pbias_mmc_omap4"
};

static const struct pbias_reg_info pbias_mmc_omap5 = {
	.enable = BIT(27) | BIT(26),
	.enable_mask = BIT(27) | BIT(25) | BIT(26),
	.disable_val = BIT(25),
	.vmode = BIT(21),
	.enable_time = 100,
	.name = "pbias_mmc_omap5"
};

static struct of_regulator_match pbias_matches[] = {
	{
		.name = "pbias_mmc_omap2430",
		.driver_data = (void *)&pbias_mmc_omap2430
	},
	{
		.name = "pbias_sim_omap3",
		.driver_data = (void *)&pbias_sim_omap3
	},
	{
		.name = "pbias_mmc_omap4",
		.driver_data = (void *)&pbias_mmc_omap4
	},
	{
		.name = "pbias_mmc_omap5",
		.driver_data = (void *)&pbias_mmc_omap5
	},
};

static int pbias_regulator_ofdata_to_platdata(struct udevice *dev)
{
	struct pbias_regulator_data *priv = dev_get_priv(dev);

	priv->base = map_physmem(dev_get_addr(dev->parent), 0x4, MAP_NOCACHE);
	if (!priv->base)
		return -ENOMEM;

	return 0;
}

static int pbias_regulator_probe(struct udevice *dev)
{
	int i;
	struct dm_regulator_uclass_platdata *uc_pdata;
	int num_matches = ARRAY_SIZE(pbias_matches);
	struct pbias_regulator_data *priv = dev_get_priv(dev);

	uc_pdata = dev_get_uclass_platdata(dev);

	for (i = 0; i < num_matches; i++) {
		struct of_regulator_match *match = &pbias_matches[i];
		if (strcmp(match->name, uc_pdata->name))
			continue;

		priv->reg_info = match->driver_data;
	}

	return 0;
}

static int pbias_regulator_set_value(struct udevice *dev, int uV)
{
	u32 value = 0;
	struct pbias_regulator_data *priv = dev_get_priv(dev);
	struct pbias_reg_info *reg_info = priv->reg_info;

	value = readl(priv->base);
	if (uV >= 3000000)
		value |= reg_info->vmode;
	else
		value &= ~reg_info->vmode;
	writel(value, priv->base);

	return 0;
}

static int pbias_regulator_set_enable(struct udevice *dev, bool enable)
{
	u32 value = 0;
	struct pbias_regulator_data *priv = dev_get_priv(dev);
	struct pbias_reg_info *reg_info = priv->reg_info;

	value = readl(priv->base);
	value &= ~reg_info->enable_mask;
	if (enable)
		value |= reg_info->enable;
	else
		value |= reg_info->disable_val;
	writel(value, priv->base);
	udelay(150);

	return 0;
}

static const struct dm_regulator_ops pbias_regulator_ops = {
	.set_value  = pbias_regulator_set_value,
	.set_enable = pbias_regulator_set_enable,
};

static const struct udevice_id pbias_regulator_ids[] = {
	{ .compatible = "ti,pbias-omap" },
	{ }
};

U_BOOT_DRIVER(pbias_regulator) = {
	.name	= "pbias-regulator",
	.id	= UCLASS_REGULATOR,
	.ops	= &pbias_regulator_ops,
	.of_match = pbias_regulator_ids,
	.probe	= pbias_regulator_probe,
	.ofdata_to_platdata = pbias_regulator_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct pbias_regulator_data),
};

static int pbias_bind(struct udevice *parent)
{
	int ret;
	int node;
	const void *fdt = gd->fdt_blob;
	const char *name;
	struct udevice *dev;

	node = fdt_first_subnode(fdt, parent->of_offset);
	if (node < 0) {
		error("pbias regulator not found\n");
		return -ENODEV;
	}

	name = fdt_get_name(fdt, node, NULL);
	ret = device_bind_driver_to_node(parent, "pbias-regulator", name, node,
					 &dev);
	if (ret) {
		error("pbias regulator not bound to driver\n");
		return ret;
	}

	return 0;
}

static const struct udevice_id pbias_ids[] = {
	{ .compatible = "ti,pbias-omap" },
	{ }
};

U_BOOT_DRIVER(pbias) = {
	.name	= "pbias",
	.id	= UCLASS_MISC,
	.of_match = pbias_ids,
	.bind	= pbias_bind,
};
