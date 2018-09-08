/*
 * AM654 DWC3 specific Glue layer
 *
 * Copyright (c) 2018, Texas Instruments Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/uclass-internal.h>
#include <generic-phy.h>
#include <errno.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <power-domain.h>
#include <usb.h>
#include "core.h"
#include "gadget.h"

int usb_gadget_handle_interrupts(int index)
{
	struct dwc3 *priv;
	struct udevice *dev;
	int ret;

	ret = uclass_find_device(UCLASS_USB_DEV_GENERIC, index, &dev);
	if (!dev || ret) {
		printf("No USB device found\n");
		return -ENODEV;
	}

	priv = dev_get_priv(dev);

	dwc3_gadget_uboot_handle_interrupt(priv);

	return 0;
}

static int am654_dwc3_setup_phy(struct udevice *dev, int index, struct phy *phy)
{
	int ret = 0;

	ret = generic_phy_get_by_index(dev, index, phy);
	if (ret) {
		if (ret != -ENOENT) {
			printf("Failed to get USB PHY for %s\n", dev->name);
			return ret;
		}
	} else {
		ret = generic_phy_init(phy);
		if (ret) {
			printf("Can't init USB PHY for %s\n", dev->name);
			return ret;
		}
		ret = generic_phy_power_on(phy);
		if (ret) {
			printf("Can't power on USB PHY for %s\n", dev->name);
			generic_phy_exit(phy);
			return ret;
		}
	}

	return 0;
}

static int am654_dwc3_peripheral_probe(struct udevice *dev)
{
	struct phy usb_phy;
	struct phy usb3_phy;
	struct dwc3 *priv = dev_get_priv(dev);
	int ret = 0;

	ret = am654_dwc3_setup_phy(dev, 0, &usb_phy);
	if (ret) {
		printf("Failed to setup USB PHY for %s\n", dev->name);
		return ret;
	}

	ret = am654_dwc3_setup_phy(dev, 1, &usb3_phy);
	if (ret) {
		printf("Failed to setup USB3 PHY for %s\n", dev->name);
		return ret;
	}

	return dwc3_init(priv);
}

static int am654_dwc3_peripheral_ofdata_to_platdata(struct udevice *dev)
{
	struct dwc3 *priv = dev_get_priv(dev);
	int node = dev_of_offset(dev);

	priv->regs = (void *)devfdt_get_addr(dev);
	priv->regs += DWC3_GLOBALS_REGS_START;

	priv->maximum_speed = usb_get_maximum_speed(node);
	if (priv->maximum_speed == USB_SPEED_UNKNOWN) {
		printf("%s Invalid usb maximum speed\n", __func__);
		return -ENODEV;
	}

	priv->dr_mode = usb_get_dr_mode(node);
	if (priv->dr_mode == USB_DR_MODE_UNKNOWN) {
		printf("%s Invalid usb mode setup\n", __func__);
		return -ENODEV;
	}
	return 0;
}

U_BOOT_DRIVER(dwc3_generic_peripheral) = {
	.name	= "am654-dwc3-peripheral",
	.id	= UCLASS_USB_DEV_GENERIC,
	.ofdata_to_platdata = am654_dwc3_peripheral_ofdata_to_platdata,
	.probe = am654_dwc3_peripheral_probe,
	.platdata_auto_alloc_size = sizeof(struct usb_platdata),
	.priv_auto_alloc_size = sizeof(struct dwc3),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};

static int am654_dwc3_glue_probe(struct udevice *dev)
{
	struct power_domain pwrdmn;
	int ret = 0;

	ret = power_domain_get_by_index(dev, &pwrdmn, 0);
	if (ret) {
		printf("%s: failed to get pwrdmn\n", __func__);
		return ret;
	}
	ret = power_domain_on(&pwrdmn);
	if (ret) {
		printf("%s: failed to set pwrdmn on\n", __func__);
		return ret;
	}

	return ret;
}

static int am654_dwc3_glue_bind(struct udevice *parent)
{
	ofnode ofnode;
	int ret;

	dev_for_each_subnode(ofnode, parent) {
		const char *name = ofnode_get_name(ofnode);
		enum usb_dr_mode dr_mode;
		struct udevice *dev;
		const char *driver;
		int node = ofnode_to_offset(ofnode);

		if (strncmp(name, "usb@", 4))
			continue;

		dr_mode = usb_get_dr_mode(node);

		switch (dr_mode) {
		case USB_DR_MODE_OTG:
			printf("%s does not support OTG mode\n", __func__);
			return -ENODEV;
		case USB_DR_MODE_PERIPHERAL:
			driver = "am654-dwc3-peripheral";
			break;
		case USB_DR_MODE_HOST:
			driver = "xhci-dwc3";
			break;
		default:
			return -ENODEV;
		};

		ret = device_bind_driver_to_node(parent, driver, name,
						 ofnode, &dev);
		if (ret) {
			printf("%s: not able to bind usb device mode\n",
			       __func__);
			return ret;
		}
	}

	return 0;
}

static const struct udevice_id am654_dwc3_glue_ids[] = {
	{ .compatible = "ti,am654-dwc3" },
	{ }
};

U_BOOT_DRIVER(dwc3_am654_glue) = {
	.name = "dwc3_am654_glue",
	.id = UCLASS_MISC,
	.of_match = am654_dwc3_glue_ids,
	.bind = am654_dwc3_glue_bind,
	.probe = am654_dwc3_glue_probe,
};
