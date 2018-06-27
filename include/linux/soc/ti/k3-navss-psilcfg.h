/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *  Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com
 *  Author: Peter Ujfalusi <peter.ujfalusi@ti.com>
 */

#ifndef __SOC_TI_K3_NAVSS_PSILCFG_H__
#define __SOC_TI_K3_NAVSS_PSILCFG_H__

struct k3_nav_psil_entry;

struct k3_nav_psil_entry *k3_nav_psil_request_link(
		struct udevice *psilcfg_dev, u32 src_thread, u32 dst_thread);
int k3_nav_psil_release_link(struct k3_nav_psil_entry *link);

#endif /* __SOC_TI_K3_NAVSS_PSILCFG_H__ */
