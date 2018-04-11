/*
 * OMAP5 family DWC3 specific Glue layer
 *
 * Copyright (c) 2017
 * Jean-Jacques Hiblot <jjhiblot@ti.com>
 * based on dwc3-sti-glue
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <dm/of_access.h>

DECLARE_GLOBAL_DATA_PTR;

static inline bool is_ofnode_compatible(ofnode node, const char *compat)
{
	const void *fdt = gd->fdt_blob;

	if (ofnode_is_np(node))
		return of_device_is_compatible(ofnode_to_np(node), compat,
					       NULL, NULL);
	else
		return !fdt_node_check_compatible(fdt, ofnode_to_offset(node),
						  compat);
}

static int omap5_dwc3_glue_bind(struct udevice *dev)
{
	bool found = false;
	ofnode node;

	dev_for_each_subnode(node, dev) {
		if (is_ofnode_compatible(node, "snps,dwc3")) {
			found = true;
			break;
		}
	}

	if (!found) {
		dev_err(dev, "Can't find subnode compatible with dwc3");
		return -ENOENT;
	}

	return dm_scan_fdt_dev(dev);
}

static const struct udevice_id omap5_dwc3_glue_ids[] = {
	{ .compatible = "ti,dwc3" },
	{ }
};

U_BOOT_DRIVER(dwc3_omap5_glue) = {
	.name = "dwc3_omap5_glue",
	.id = UCLASS_MISC,
	.of_match = omap5_dwc3_glue_ids,
	.bind = omap5_dwc3_glue_bind,
};
