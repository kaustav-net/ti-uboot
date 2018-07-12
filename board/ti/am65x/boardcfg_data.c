/*
 * K3 System Firmware Board Configuration Data
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Andreas Dannenberg <dannenberg@ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <asm/arch/boardcfg_data.h>

const struct k3_boardcfg k3_boardcfg_data = {
	/* boardcfg_abi_rev */
	.rev = {
		.boardcfg_abi_maj = 0x0,
		.boardcfg_abi_min = 0x1,
	},

	/* boardcfg_control */
	.control = {
		.subhdr = {
			.magic = BOARDCFG_CONTROL_MAGIC_NUM,
			.size = sizeof(struct k3_boardcfg),
		},
		.main_isolation_enable = 0x5A,
		.main_isolation_hostid = 0x2,
	},

	/* boardcfg sec_proxy */
	.secproxy = {
		.subhdr = {
			.magic = BOARDCFG_SECPROXY_MAGIC_NUM,
			.size = sizeof(struct k3_boardcfg),
		},
		.scaling_factor = 0x1,
		.scaling_profile = 0x1,
		.disable_main_nav_secure_proxy = 0,
	},

	/* boardcfg_msmc */
	.msmc = {
		.subhdr = {
			.magic = BOARDCFG_MSMC_MAGIC_NUM,
			.size = sizeof(struct k3_boardcfg),
		},
		.msmc_cache_size = 0x10,
	},

	/* boardcfg_proc_acl */
	.processor_acl_list = {
		.subhdr = {
			.magic = BOARDCFG_PROC_ACL_MAGIC_NUM,
			.size = sizeof(struct k3_boardcfg),
		},
		.proc_acl_entries = {{0}},
	},

	/* boardcfg_dbg_cfg */
	.debug_cfg = {
		.subhdr = {
			.magic = BOARDCFG_DBG_CFG_MAGIC_NUM,
			.size = sizeof(struct k3_boardcfg),
		},
	},

	/* boardcfg_pmic_cfg */
	.pmic_cfg = {
		.subhdr = {
			.magic = BOARDCFG_PMIC_CFG_MAGIC_NUM,
			.size = sizeof(struct k3_boardcfg),
		},
	},
};
