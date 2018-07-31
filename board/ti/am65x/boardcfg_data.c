/*
 * K3 System Firmware Board Configuration Data
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Andreas Dannenberg <dannenberg@ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <asm/arch/boardcfg_data.h>

const struct k3_boardcfg am65_boardcfg_data = {
	/* boardcfg_abi_rev */
	.rev = {
		.boardcfg_abi_maj = 0x0,
		.boardcfg_abi_min = 0x1,
	},

	/* boardcfg_control */
	.control = {
		.subhdr = {
			.magic = BOARDCFG_CONTROL_MAGIC_NUM,
			.size = sizeof(struct boardcfg_control),
		},
		.main_isolation_enable = 0x5A,
		.main_isolation_hostid = 0x2,
	},

	/* boardcfg sec_proxy */
	.secproxy = {
		.subhdr = {
			.magic = BOARDCFG_SECPROXY_MAGIC_NUM,
			.size = sizeof(struct boardcfg_secproxy),
		},
		.scaling_factor = 0x1,
		.scaling_profile = 0x1,
		.disable_main_nav_secure_proxy = 0,
	},

	/* boardcfg_msmc */
	.msmc = {
		.subhdr = {
			.magic = BOARDCFG_MSMC_MAGIC_NUM,
			.size = sizeof(struct boardcfg_msmc),
		},
		.msmc_cache_size = 0x10,
	},

	/* boardcfg_dbg_cfg */
	.debug_cfg = {
		.subhdr = {
			.magic = BOARDCFG_DBG_CFG_MAGIC_NUM,
			.size = sizeof(struct boardcfg_dbg_cfg),
		},
	},
};

const struct am65_boardcfg_rm_local am65_boardcfg_rm_data = {
	.rm_boardcfg = {
		/* boardcfg_abi_rev */
		.rev = {
			.boardcfg_abi_maj = 0x0,
			.boardcfg_abi_min = 0x1,
		},

		/* boardcfg_rm_host_cfg */
		.host_cfg = {
			.subhdr = {
				.magic = BOARDCFG_RM_HOST_CFG_MAGIC_NUM,
				.size = sizeof(struct boardcfg_rm_host_cfg),
			},
			.host_cfg_entries = {{ 0 } },
		},

		/* boardcfg_rm_resasg */
		.resasg = {
			.subhdr = {
				.magic = BOARDCFG_RM_RESASG_MAGIC_NUM,
				.size = sizeof(struct boardcfg_rm_resasg),
			},
			.resasg_entries_size = AM65_BOARDCFG_RM_RESASG_ENTRIES *
					sizeof(struct boardcfg_rm_resasg_entry),
			.reserved = 0,
			/* .resasg_entries is set via k3_boardcfg_rm_local */
		},
	},

	/* This is actually part of .resasg */
	.resasg_entries = {
		{
			.start_resource = 16,
			.num_resource = 240,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_UDMASS_IA0,
				       RESASG_SUBTYPE_MAIN_NAV_UDMASS_IA0_VINT),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 16,
			.num_resource = 4592,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_UDMASS_IA0,
				       RESASG_SUBTYPE_MAIN_NAV_UDMASS_IA0_SEVI),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 32768,
			.num_resource = 512,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_UDMASS_IA0,
				       RESASG_SUBTYPE_MAIN_NAV_UDMASS_IA0_MEVI),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 36864,
			.num_resource = 512,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_UDMASS_IA0,
				       RESASG_SUBTYPE_MAIN_NAV_UDMASS_IA0_GEVI),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 0,
			.num_resource = 64,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_MODSS_IA0,
					RESASG_SUBTYPE_MAIN_NAV_MODSS_IA0_VINT),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 20480,
			.num_resource = 1024,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_MODSS_IA0,
					RESASG_SUBTYPE_MAIN_NAV_MODSS_IA0_SEVI),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 0,
			.num_resource = 64,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_MODSS_IA1,
					RESASG_SUBTYPE_MAIN_NAV_MODSS_IA1_VINT),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 22528,
			.num_resource = 1024,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_MODSS_IA1,
					RESASG_SUBTYPE_MAIN_NAV_MODSS_IA1_SEVI),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 8,
			.num_resource = 248,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_UDMASS_IA0,
					RESASG_SUBTYPE_MCU_NAV_UDMASS_IA0_VINT),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 16392,
			.num_resource = 992,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_UDMASS_IA0,
					RESASG_SUBTYPE_MCU_NAV_UDMASS_IA0_SEVI),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 17384,
			.num_resource = 536,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_UDMASS_IA0,
					RESASG_SUBTYPE_MCU_NAV_UDMASS_IA0_SEVI),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 34816,
			.num_resource = 128,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_UDMASS_IA0,
					RESASG_SUBTYPE_MCU_NAV_UDMASS_IA0_MEVI),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 39936,
			.num_resource = 256,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_UDMASS_IA0,
					RESASG_SUBTYPE_MCU_NAV_UDMASS_IA0_GEVI),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 43008,
			.num_resource = 4,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_MCRC,
					     RESASG_SUBTYPE_MAIN_NAV_MCRC_LEVI),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 43136,
			.num_resource = 4,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_MCRC,
					     RESASG_SUBTYPE_MCU_NAV_MCRC_LEVI),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 49152,
			.num_resource = 1024,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_UDMAP,
					 RESASG_SUBTYPE_MAIN_NAV_UDMAP_TRIGGER),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 0,
			.num_resource = 8,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_UDMAP,
					RESASG_SUBTYPE_MAIN_NAV_UDMAP_TX_HCHAN),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 8,
			.num_resource = 112,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_UDMAP,
					 RESASG_SUBTYPE_MAIN_NAV_UDMAP_TX_CHAN),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 120,
			.num_resource = 32,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_UDMAP,
					RESASG_SUBTYPE_MAIN_NAV_UDMAP_TX_ECHAN),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 0,
			.num_resource = 8,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_UDMAP,
					RESASG_SUBTYPE_MAIN_NAV_UDMAP_RX_HCHAN),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 8,
			.num_resource = 142,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_UDMAP,
					 RESASG_SUBTYPE_MAIN_NAV_UDMAP_RX_CHAN),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 150,
			.num_resource = 150,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_UDMAP,
				  RESASG_SUBTYPE_MAIN_NAV_UDMAP_RX_FLOW_COMMON),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 0,
			.num_resource = 1,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_UDMAP,
				RESASG_SUBTYPE_MAIN_NAV_UDMAP_INVALID_FLOW_OES),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 56320,
			.num_resource = 256,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_UDMAP,
					  RESASG_SUBTYPE_MCU_NAV_UDMAP_TRIGGER),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 0,
			.num_resource = 2,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_UDMAP,
					 RESASG_SUBTYPE_MCU_NAV_UDMAP_TX_HCHAN),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 2,
			.num_resource = 46,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_UDMAP,
					  RESASG_SUBTYPE_MCU_NAV_UDMAP_TX_CHAN),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 0,
			.num_resource = 2,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_UDMAP,
					 RESASG_SUBTYPE_MCU_NAV_UDMAP_RX_HCHAN),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 2,
			.num_resource = 46,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_UDMAP,
					  RESASG_SUBTYPE_MCU_NAV_UDMAP_RX_CHAN),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 48,
			.num_resource = 48,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_UDMAP,
				   RESASG_SUBTYPE_MCU_NAV_UDMAP_RX_FLOW_COMMON),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 0,
			.num_resource = 1,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_UDMAP,
				 RESASG_SUBTYPE_MCU_NAV_UDMAP_INVALID_FLOW_OES),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 61440,
			.num_resource = 64,
			.type = RESASG_UTYPE(RESASG_TYPE_MSMC,
					     RESASG_SUBTYPE_MSMC_DRU),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 0,
			.num_resource = 152,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_RA,
				      RESASG_SUBTYPE_MAIN_NAV_RA_RING_UDMAP_TX),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 152,
			.num_resource = 150,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_RA,
				      RESASG_SUBTYPE_MAIN_NAV_RA_RING_UDMAP_RX),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 302,
			.num_resource = 466,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_RA,
					    RESASG_SUBTYPE_MAIN_NAV_RA_RING_GP),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 0,
			.num_resource = 1,
			.type = RESASG_UTYPE(RESASG_TYPE_MAIN_NAV_RA,
					  RESASG_SUBTYPE_MAIN_NAV_RA_ERROR_OES),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 0,
			.num_resource = 48,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_RA,
				       RESASG_SUBTYPE_MCU_NAV_RA_RING_UDMAP_TX),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 48,
			.num_resource = 48,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_RA,
				       RESASG_SUBTYPE_MCU_NAV_RA_RING_UDMAP_RX),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 96,
			.num_resource = 160,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_RA,
					     RESASG_SUBTYPE_MCU_NAV_RA_RING_GP),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 0,
			.num_resource = 1,
			.type = RESASG_UTYPE(RESASG_TYPE_MCU_NAV_RA,
					   RESASG_SUBTYPE_MCU_NAV_RA_ERROR_OES),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 80,
			.num_resource = 48,
			.type = RESASG_UTYPE(RESASG_TYPE_GIC_IRQ,
					  RESASG_SUBTYPE_GIC_IRQ_MAIN_NAV_SET0),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 392,
			.num_resource = 32,
			.type = RESASG_UTYPE(RESASG_TYPE_GIC_IRQ,
					     RESASG_SUBTYPE_GIC_IRQ_MAIN_GPIO),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 448,
			.num_resource = 50,
			.type = RESASG_UTYPE(RESASG_TYPE_GIC_IRQ,
					  RESASG_SUBTYPE_GIC_IRQ_MAIN_NAV_SET1),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 498,
			.num_resource = 6,
			.type = RESASG_UTYPE(RESASG_TYPE_GIC_IRQ,
					  RESASG_SUBTYPE_GIC_IRQ_MAIN_NAV_SET1),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 544,
			.num_resource = 16,
			.type = RESASG_UTYPE(RESASG_TYPE_GIC_IRQ,
					     RESASG_SUBTYPE_GIC_IRQ_COMP_EVT),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 712,
			.num_resource = 16,
			.type = RESASG_UTYPE(RESASG_TYPE_GIC_IRQ,
					     RESASG_SUBTYPE_GIC_IRQ_WKUP_GPIO),
			.host_id = HOST_ID_A53_2,
		},
		{
			.start_resource = 64,
			.num_resource = 32,
			.type = RESASG_UTYPE(RESASG_TYPE_PULSAR_C0_IRQ,
					  RESASG_SUBTYPE_PULSAR_C0_IRQ_MCU_NAV),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 124,
			.num_resource = 16,
			.type = RESASG_UTYPE(RESASG_TYPE_PULSAR_C0_IRQ,
					RESASG_SUBTYPE_PULSAR_C0_IRQ_WKUP_GPIO),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 160,
			.num_resource = 64,
			.type = RESASG_UTYPE(RESASG_TYPE_PULSAR_C0_IRQ,
				     RESASG_SUBTYPE_PULSAR_C0_IRQ_MAIN2MCU_LVL),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 224,
			.num_resource = 48,
			.type = RESASG_UTYPE(RESASG_TYPE_PULSAR_C0_IRQ,
				     RESASG_SUBTYPE_PULSAR_C0_IRQ_MAIN2MCU_PLS),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 64,
			.num_resource = 32,
			.type = RESASG_UTYPE(RESASG_TYPE_PULSAR_C1_IRQ,
					  RESASG_SUBTYPE_PULSAR_C1_IRQ_MCU_NAV),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 124,
			.num_resource = 16,
			.type = RESASG_UTYPE(RESASG_TYPE_PULSAR_C1_IRQ,
					RESASG_SUBTYPE_PULSAR_C1_IRQ_WKUP_GPIO),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 160,
			.num_resource = 64,
			.type = RESASG_UTYPE(RESASG_TYPE_PULSAR_C1_IRQ,
				     RESASG_SUBTYPE_PULSAR_C1_IRQ_MAIN2MCU_LVL),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 224,
			.num_resource = 48,
			.type = RESASG_UTYPE(RESASG_TYPE_PULSAR_C1_IRQ,
				     RESASG_SUBTYPE_PULSAR_C1_IRQ_MAIN2MCU_PLS),
			.host_id = HOST_ID_R5_0,
		},
		{
			.start_resource = 46,
			.num_resource = 8,
			.type = RESASG_UTYPE(RESASG_TYPE_ICSSG0_IRQ,
					    RESASG_SUBTYPE_ICSSG0_IRQ_MAIN_NAV),
			.host_id = HOST_ID_ICSSG_0,
		},
		{
			.start_resource = 88,
			.num_resource = 8,
			.type = RESASG_UTYPE(RESASG_TYPE_ICSSG0_IRQ,
					   RESASG_SUBTYPE_ICSSG0_IRQ_MAIN_GPIO),
			.host_id = HOST_ID_ICSSG_0,
		},
		{
			.start_resource = 46,
			.num_resource = 8,
			.type = RESASG_UTYPE(RESASG_TYPE_ICSSG1_IRQ,
					    RESASG_SUBTYPE_ICSSG1_IRQ_MAIN_NAV),
			.host_id = HOST_ID_ICSSG_1,
		},
		{
			.start_resource = 88,
			.num_resource = 8,
			.type = RESASG_UTYPE(RESASG_TYPE_ICSSG1_IRQ,
					   RESASG_SUBTYPE_ICSSG1_IRQ_MAIN_GPIO),
			.host_id = HOST_ID_ICSSG_1,
		},
		{
			.start_resource = 46,
			.num_resource = 8,
			.type = RESASG_UTYPE(RESASG_TYPE_ICSSG2_IRQ,
					    RESASG_SUBTYPE_ICSSG2_IRQ_MAIN_NAV),
			.host_id = HOST_ID_ICSSG_2,
		},
		{
			.start_resource = 88,
			.num_resource = 8,
			.type = RESASG_UTYPE(RESASG_TYPE_ICSSG2_IRQ,
					   RESASG_SUBTYPE_ICSSG2_IRQ_MAIN_GPIO),
			.host_id = HOST_ID_ICSSG_2,
		},
	},
};

const struct k3_boardcfg_security am65_boardcfg_security_data = {
	/* boardcfg_abi_rev */
	.rev = {
		.boardcfg_abi_maj = 0x0,
		.boardcfg_abi_min = 0x1,
	},

	/* boardcfg_proc_acl */
	.processor_acl_list = {
		.subhdr = {
			.magic = BOARDCFG_PROC_ACL_MAGIC_NUM,
			.size = sizeof(struct boardcfg_proc_acl),
		},
		.proc_acl_entries = {{ 0 } },
	},

	/* boardcfg_host_hierarchy */
	.host_hierarchy = {
			.subhdr = {
				.magic = BOARDCFG_HOST_HIERARCHY_MAGIC_NUM,
				.size = sizeof(struct boardcfg_host_hierarchy),
			},
			.host_hierarchy_entries = {{ 0 } },
	},
};

const struct k3_boardcfg_pm am65_boardcfg_pm_data = {
	/* empty for now */
};
