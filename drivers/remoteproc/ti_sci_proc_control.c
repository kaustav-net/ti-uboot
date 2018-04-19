/*
 * Texas Instruments' TISCI Processor Control driver
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <linux/soc/ti/ti_sci_protocol.h>
#include <power-domain.h>
#include <remoteproc.h>
#include <reset.h>

/**
 * struct ti_sci_proc_ctrl_privdata - Structure representing Processor
 *				      control data.
 * @sci:		TISCI handle
 */
struct ti_sci_proc_ctrl_privdata {
	const struct ti_sci_handle *sci;
};

static int ti_sci_proc_ctrl_xlate(struct proc_ctrl *ctrl,
				  struct ofnode_phandle_args *args)
{
	debug("%s(ctrl=%p, args_count=%d)\n", __func__, ctrl, args->args_count);

	if (args->args_count != 2) {
		debug("Invalid args_count: %d\n", args->args_count);
		return -EINVAL;
	}

	ctrl->id = args->args[0];
	ctrl->data = args->args[1];

	return 0;
}

/**
 * ti_sci_proc_ctrl_request() - Request control for the TISCI remote processor
 * @dev:	rproc device pointer
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
static int ti_sci_proc_ctrl_request(struct proc_ctrl *ctrl)
{
	struct ti_sci_proc_ctrl_privdata *rproc = dev_get_priv(ctrl->dev);
	const struct ti_sci_proc_ops *pops = &rproc->sci->ops.proc_ops;
	int ret;

	dev_dbg(dev, "%s(ctrl=%p)\n", __func__, ctrl);

	/* request for the processor */
	ret = pops->proc_request(rproc->sci, ctrl->id);
	if (ret) {
		dev_err(dev, "Requesting processor failed %d\n", ret);
		return ret;
	}

	return 0;
}

/**
 * ti_sci_proc_ctrl_release() - handover control for the TISCI remote processor
 * @dev:	rproc device pointer
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
static int ti_sci_proc_ctrl_release(struct proc_ctrl *ctrl)
{
	struct ti_sci_proc_ctrl_privdata *rproc = dev_get_priv(ctrl->dev);
	const struct ti_sci_proc_ops *pops = &rproc->sci->ops.proc_ops;
	int ret;

	dev_dbg(dev, "%s(ctrl=%p)\n", __func__, ctrl);

	/*
	 * hand over the control to the given host id.
	 * ToDo: If host id is not provided call proc_release()
	 *	 instead of calling proc_handover().
	 */
	ret = pops->proc_handover(rproc->sci, ctrl->id, ctrl->data);
	if (ret) {
		dev_err(dev, "Handover processor failed %d\n", ret);
		return ret;
	}

	return 0;
}

/**
 * ti_sci_proc_ctrl_load() - Loadup the TISCI remote processor
 * @dev:	rproc device pointer
 * @addr:	Address at which image is available
 * @size:	size of the image
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
static int ti_sci_proc_ctrl_load(struct proc_ctrl *ctrl, ulong addr, ulong size)
{
	struct ti_sci_proc_ctrl_privdata *rproc = dev_get_priv(ctrl->dev);
	const struct ti_sci_proc_ops *pops = &rproc->sci->ops.proc_ops;
	int ret;

	dev_dbg(dev, "%s addr = 0x%lx, size = 0x%lx\n", __func__, addr, size);

	ret = pops->set_proc_boot_cfg(rproc->sci, ctrl->id, addr, 0, 0);
	if (ret) {
		dev_err(dev, "set_proc_boot_cfg failed %d\n", ret);
		return ret;
	}

	dev_dbg(dev, "%s: rproc successfully loaded\n", __func__);

	return 0;
}

/**
 * ti_sci_proc_ctrl_start() - Start the TISCI remote processor
 * @dev:	rproc device pointer
 *
 * Return: 0 if all went ok, else return appropriate error
 */
static int ti_sci_proc_ctrl_start(struct proc_ctrl *ctrl)
{
	dev_dbg(ctrl->dev, "%s\n", __func__);

	return 0;
}

static const struct proc_ctrl_ops ti_sci_proc_ctrl_ops = {
	.request = ti_sci_proc_ctrl_request,
	.release = ti_sci_proc_ctrl_release,
	.load = ti_sci_proc_ctrl_load,
	.start = ti_sci_proc_ctrl_start,
	.of_xlate = ti_sci_proc_ctrl_xlate,
};

/**
 * ti_sci_proc_ctrl_probe() - Basic probe
 * @dev:	corresponding TISCI remote processor device
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
static int ti_sci_proc_ctrl_probe(struct udevice *dev)
{
	struct ti_sci_proc_ctrl_privdata *priv;

	dev_dbg(dev, "%s\n", __func__);

	priv = dev_get_priv(dev);

	priv->sci = ti_sci_get_handle(dev);
	if (IS_ERR(priv->sci))
		return PTR_ERR(priv->sci);

	dev_dbg(dev, "Processor Control successfully probed\n");

	return 0;
}

static const struct udevice_id ti_sci_proc_ctrl_ids[] = {
	{ .compatible = "ti,sci-proc-control"},
	{}
};

U_BOOT_DRIVER(ti_sci_proc_ctrl) = {
	.name = "ti_sci_proc_control",
	.of_match = ti_sci_proc_ctrl_ids,
	.id = UCLASS_PROC_CONTROL,
	.ops = &ti_sci_proc_ctrl_ops,
	.probe = ti_sci_proc_ctrl_probe,
	.priv_auto_alloc_size = sizeof(struct ti_sci_proc_ctrl_privdata),
};
