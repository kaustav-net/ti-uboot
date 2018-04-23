/*
 * Texas Instruments' K3 Remoteproc driver
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <remoteproc.h>
#include <errno.h>
#include <clk.h>
#include <reset.h>
#include <asm/io.h>
#include <power-domain.h>

/**
 * struct k3_rproc_privdata - Structure representing Remote processor data.
 * @rproc_pwrdmn:	rproc power domain data
 * @rproc_rst:		rproc reset control data
 * @rproc_ctrl:		rproc processor control data
 */
struct k3_rproc_privdata {
	struct power_domain rproc_pwrdmn;
	struct power_domain gtc_pwrdmn;
	struct reset_ctl rproc_rst;
	struct proc_ctrl rproc_ctrl;
};

/**
 * k3_rproc_load() - Load up the Remote processor image
 * @dev:	rproc device pointer
 * @addr:	Address at which image is available
 * @size:	size of the image
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
static int k3_rproc_load(struct udevice *dev, ulong addr, ulong size)
{
	struct k3_rproc_privdata *rproc = dev_get_priv(dev);
	int ret;

	dev_dbg(dev, "%s addr = 0x%lx, size = 0x%lx\n", __func__, addr, size);

	ret = proc_ctrl_rproc_request(&rproc->rproc_ctrl);
	if (ret) {
		dev_err(dev, "Processor request failed %d\n", ret);
		return ret;
	}

	ret = proc_ctrl_rproc_load(&rproc->rproc_ctrl, addr, size);
	if (ret) {
		dev_err(dev, "Processor load failed %d\n", ret);
		return ret;
	}

	dev_dbg(dev, "%s: rproc successfully loaded\n", __func__);

	return 0;
}

/**
 * k3_rproc_start() - Start the remote processor
 * @dev:	rproc device pointer
 *
 * Return: 0 if all went ok, else return appropriate error
 */
static int k3_rproc_start(struct udevice *dev)
{
	struct k3_rproc_privdata *rproc = dev_get_priv(dev);
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	ret = power_domain_on(&rproc->gtc_pwrdmn);
	if (ret) {
		dev_err(dev, "power_domain_on() failed: %d\n", ret);
		return ret;
	}

	/* Enable GTC, ToDo: Get base address from DT. */
	writel(0x3, 0x00A90000);

	/*
	 * Setting the right clock frequency would have taken care by
	 * assigned-clock-rates during the device probe. So no need to
	 * set the frequency again here.
	 */
	ret = power_domain_on(&rproc->rproc_pwrdmn);
	if (ret) {
		dev_err(dev, "power_domain_on() failed: %d\n", ret);
		return ret;
	}

	ret = proc_ctrl_rproc_release(&rproc->rproc_ctrl);
	if (ret) {
		dev_err(dev, "Processor release failed %d\n", ret);
		return ret;
	}

	dev_dbg(dev, "%s: rproc successfully started\n", __func__);

	return 0;
}

/**
 * k3_rproc_init() - Initialize the remote processor
 * @dev:	rproc device pointer
 *
 * Return: 0 if all went ok, else return appropriate error
 */
static int k3_rproc_init(struct udevice *dev)
{
	dev_dbg(dev, "%s\n", __func__);

	/* Enable the module */
	dev_dbg(dev, "%s: rproc successfully initialized\n", __func__);

	return 0;
}

static const struct dm_rproc_ops k3_rproc_ops = {
	.init = k3_rproc_init,
	.load = k3_rproc_load,
	.start = k3_rproc_start,
};

/**
 * k3_of_to_priv() - generate private data from device tree
 * @dev:	corresponding k3 remote processor device
 * @priv:	pointer to driver specific private data
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
static int k3_rproc_of_to_priv(struct udevice *dev,
			       struct k3_rproc_privdata *rproc)
{
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	ret = power_domain_get_by_index(dev, &rproc->rproc_pwrdmn, 0);
	if (ret) {
		dev_err(dev, "power_domain_get() failed: %d\n", ret);
		return ret;
	}

	ret = power_domain_get_by_index(dev, &rproc->gtc_pwrdmn, 1);
	if (ret) {
		dev_err(dev, "power_domain_get() failed: %d\n", ret);
		return ret;
	}

	ret = reset_get_by_index(dev, 0, &rproc->rproc_rst);
	if (ret) {
		dev_err(dev, "reset_get() failed: %d\n", ret);
		return ret;
	}

	ret = proc_ctrl_get_by_index(dev, 0, &rproc->rproc_ctrl);
	if (ret) {
		dev_err(dev, "proc_ctrl_get() failed: %d\n", ret);
		return ret;
	}

	return 0;
}

/**
 * k3_rproc_probe() - Basic probe
 * @dev:	corresponding k3 remote processor device
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
static int k3_rproc_probe(struct udevice *dev)
{
	struct k3_rproc_privdata *priv;
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	priv = dev_get_priv(dev);

	ret = k3_rproc_of_to_priv(dev, priv);
	if (ret) {
		dev_dbg(dev, "%s: Probe failed with error %d\n", __func__, ret);
		return ret;
	}

	dev_dbg(dev, "Remoteproc successfully probed\n");

	return 0;
}

static const struct udevice_id k3_rproc_ids[] = {
	{ .compatible = "ti,am654-rproc"},
	{}
};

U_BOOT_DRIVER(k3_rproc) = {
	.name = "k3_rproc",
	.of_match = k3_rproc_ids,
	.id = UCLASS_REMOTEPROC,
	.ops = &k3_rproc_ops,
	.probe = k3_rproc_probe,
	.priv_auto_alloc_size = sizeof(struct k3_rproc_privdata),
};
