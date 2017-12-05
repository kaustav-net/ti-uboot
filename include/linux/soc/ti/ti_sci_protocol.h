/*
 * Texas Instruments System Control Interface Protocol
 * Based on include/linux/soc/ti/ti_sci_protocol.h from Linux.
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Nishanth Menon
 *	Lokesh Vutla <lokeshvutla@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __TISCI_PROTOCOL_H
#define __TISCI_PROTOCOL_H

/**
 * struct ti_sci_handle - Handle returned to TI SCI clients for usage.
 * @ops:	operations that are made available to TI SCI clients
 */
struct ti_sci_handle {
	struct ti_sci_ops ops;
};

#if IS_ENABLED(CONFIG_TI_SCI_PROTOCOL)

const struct ti_sci_handle *ti_sci_get_handle_from_sysfw(struct udevice *sci_dev);
const struct ti_sci_handle *ti_sci_get_handle(struct udevice *dev);

#else	/* CONFIG_TI_SCI_PROTOCOL */

static inline const struct ti_sci_handle *ti_sci_get_handle_from_sysfw(
						       struct udevice *sci_dev)
{
	return ERR_PTR(-EINVAL);
}

static inline const struct ti_sci_handle *ti_sci_get_handle(struct udevice *dev)
{
	return ERR_PTR(-EINVAL);
}

#endif	/* CONFIG_TI_SCI_PROTOCOL */

#endif	/* __TISCI_PROTOCOL_H */
