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
 * struct ti_sci_version_info - version information structure
 * @abi_major:	Major ABI version. Change here implies risk of backward
 *		compatibility break.
 * @abi_minor:	Minor ABI version. Change here implies new feature addition,
 *		or compatible change in ABI.
 * @firmware_revision:	Firmware revision (not usually used).
 * @firmware_description: Firmware description (not usually used).
 */
struct ti_sci_version_info {
	u8 abi_major;
	u8 abi_minor;
	u16 firmware_revision;
	char firmware_description[32];
};

struct ti_sci_handle;

/**
 * struct ti_sci_misc_ops - Miscellaneous operations
 * @get_revision: Command to obtain and populate SYSFW revision
 *		  Returns 0 for successful exclusive request, else returns
 *		  corresponding error message.
 * @board_config: Command to set the board configuration
 *		  Returns 0 for successful exclusive request, else returns
 *		  corresponding error message.
 */
struct ti_sci_misc_ops {
	int (*get_revision)(struct ti_sci_handle *handle);
	int (*board_config)(const struct ti_sci_handle *handle,
			    u64 addr, u32 size);
};

/**
 * struct ti_sci_ops - Function support for TI SCI
 * @misc_ops:	Miscellaneous operations
 */
struct ti_sci_ops {
	struct ti_sci_misc_ops misc_ops;
};

/**
 * struct ti_sci_handle - Handle returned to TI SCI clients for usage.
 * @ops:	operations that are made available to TI SCI clients
 * @version:	structure containing version information
 */
struct ti_sci_handle {
	struct ti_sci_ops ops;
	struct ti_sci_version_info version;
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
