// SPDX-License-Identifier: GPL-2.0+
/*
 *  Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com
 *  Author: Peter Ujfalusi <peter.ujfalusi@ti.com>
 */

#include <common.h>
#include <asm/io.h>
#include <asm/processor.h>
#include <dm.h>
#include <misc.h>
#include <linux/compat.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/soc/ti/k3-navss-psilcfg.h>
#include <linux/soc/ti/ti_sci_protocol.h>

#define PSIL_DST_THREAD_MARK		0x8000

struct k3_nav_psil_priv {
	struct udevice *dev;
	const struct ti_sci_handle *tisci;
	const struct ti_sci_rm_psil_ops *tisci_psil_ops;
	u32  tisci_dev_id;

	struct list_head psil_links;
};

struct k3_nav_psil_entry {
	struct list_head node;
	struct k3_nav_psil_priv *priv;

	u32 src_thread;
	u32 dst_thread;
};

struct k3_nav_psil_entry *k3_nav_psil_request_link(
				struct udevice *psilcfg_dev,
				u32 src_thread, u32 dst_thread)
{
	struct k3_nav_psil_entry *link;
	struct k3_nav_psil_priv *priv;
	int ret;

	priv = dev_get_priv(psilcfg_dev);
	if (!priv) {
		pr_err("driver data not available\n");
		return ERR_PTR(-EINVAL);
	}

	link = kzalloc(sizeof(*link), GFP_KERNEL);
	if (!link)
		return ERR_PTR(-ENOMEM);

	link->src_thread = src_thread;
	link->dst_thread = (dst_thread | PSIL_DST_THREAD_MARK);
	link->priv = priv;

	ret = priv->tisci_psil_ops->pair(priv->tisci, priv->tisci_dev_id,
					 link->src_thread, link->dst_thread);
	if (ret) {
		kfree(link);
		link = ERR_PTR(ret);
	} else {
		list_add(&link->node, &priv->psil_links);
	}

	return link;
}

int k3_nav_psil_release_link(struct k3_nav_psil_entry *link)
{
	struct k3_nav_psil_priv *priv = link->priv;
	int ret;

	list_del(&link->node);
	ret = priv->tisci_psil_ops->unpair(priv->tisci, priv->tisci_dev_id,
					   link->src_thread, link->dst_thread);

	kfree(link);
	return ret;
}

static int psilcfg_probe(struct udevice *dev)
{
	struct k3_nav_psil_priv *priv = dev_get_priv(dev);
	struct udevice *tisci_dev = NULL;
	int ret;

	priv->dev = dev;

	INIT_LIST_HEAD(&priv->psil_links);

	ret = uclass_get_device_by_name(UCLASS_FIRMWARE, "dmsc", &tisci_dev);
	if (ret) {
		debug("TISCI RA PSI-L get failed (%d)\n", ret);
		priv->tisci = NULL;
		return 0;
	}
	priv->tisci = (struct ti_sci_handle *)
			ti_sci_get_handle_from_sysfw(tisci_dev);

	ret = dev_read_u32_default(dev, "ti,sci", 0);
	if (!ret) {
		dev_err(dev, "TISCI PSI-L RM disabled\n");
		priv->tisci = NULL;
		return ret;
	}

	ret = dev_read_u32(dev, "ti,sci-dev-id", &priv->tisci_dev_id);
	if (ret) {
		dev_err(dev, "ti,sci-dev-id read failure %d\n", ret);
		return ret;
	}
	priv->tisci_psil_ops = &priv->tisci->ops.rm_psil_ops;

	dev_info(dev, "TISCI PSI-L RM dev-id %u\n", priv->tisci_dev_id);

	return 0;
}

static const struct udevice_id psilcfg_ids[] = {
	{ .compatible = "ti,k3-navss-psilcfg" },
	{ }
};

U_BOOT_DRIVER(k3_navss_psilcfg) = {
	.name	= "k3-navss-psilcfg",
	.id	= UCLASS_MISC,
	.of_match = psilcfg_ids,
	.probe = psilcfg_probe,
	.priv_auto_alloc_size = sizeof(struct k3_nav_psil_priv),
};
