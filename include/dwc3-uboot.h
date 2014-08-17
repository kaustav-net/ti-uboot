/* include/dwc3-uboot.h
 *
 * Copyright (c) 2014 Texas Instruments Incorporated - http://www.ti.com
 *
 * Designware SuperSpeed USB uboot init
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __DWC3_UBOOT_H_
#define __DWC3_UBOOT_H_

#include <linux/usb/otg.h>

struct dwc3_device {
	u32 maximum_speed;
	int base;
	unsigned needs_fifo_resize:1;
	enum usb_dr_mode dr_mode;
	int index;
};

int dwc3_uboot_init(struct dwc3_device *dev);
void dwc3_uboot_exit(int index);
void dwc3_uboot_handle_interrupt(int index);
#endif /* __DWC3_UBOOT_H_ */
