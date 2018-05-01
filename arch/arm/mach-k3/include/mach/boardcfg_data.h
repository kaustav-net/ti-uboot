/*
 * K3 System Firmware Board Configuration Data Structures
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Andreas Dannenberg <dannenberg@ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <linux/types.h>

/**
 * Fault tolerant boolean type
 */
typedef u8 ftbool;

/**
 * Various definitions as expected by the 'struct' declarations below
 */
#define BOARDCFG_MAX_MAIN_HOST_COUNT		8
#define BOARDCFG_MAX_MCU_HOST_COUNT		4
#define BOARDCFG_CONTROL_MAGIC_NUM		0xC1D3
#define BOARDCFG_SECPROXY_MAGIC_NUM		0x1207
#define BOARDCFG_MSMC_MAGIC_NUM			0xA5C3
#define BOARDCFG_PROC_ACL_MAGIC_NUM		0xF1EA
#define BOARDCFG_RESASG_MAGIC_NUM		0x4C41
#define BOARDCFG_DBG_CFG_MAGIC_NUM		0x020C
#define BOARDCFG_PMIC_CFG_MAGIC_NUM		0x3172

#define BOARDCFG_ABI_MAJ_VALUE			0x00
#define BOARDCFG_ABI_MIN_VALUE			0x01

struct boardcfg_substructure_header {
	u16	magic;
	u16	size;
} __attribute__((__packed__));

struct boardcfg_abi_rev {
	u8	boardcfg_abi_maj;
	u8	boardcfg_abi_min;
} __attribute__((__packed__));

struct boardcfg_control {
	struct boardcfg_substructure_header	subhdr;
	ftbool					main_isolation_enable;
	u16					main_isolation_hostid;
} __attribute__((__packed__));

struct boardcfg_secproxy {
	struct boardcfg_substructure_header	subhdr;
	u8					scaling_factor;
	u8					scaling_profile;
	u8					disable_main_nav_secure_proxy;
} __attribute__((__packed__));

struct boardcfg_msmc {
	struct boardcfg_substructure_header	subhdr;
	u8					msmc_cache_size;
} __attribute__((__packed__));

#define PROCESSOR_ACL_SECONDARY_MASTERS_MAX (3U)

struct boardcfg_proc_acl_entry {
	u8	processor_id;
	u8	proc_access_master;
	u8	proc_access_secondary[PROCESSOR_ACL_SECONDARY_MASTERS_MAX];
} __attribute__((__packed__));

#define PROCESSOR_ACL_ENTRIES (32U)

struct boardcfg_proc_acl {
	struct boardcfg_substructure_header	subhdr;
	struct boardcfg_proc_acl_entry		proc_acl_entries[
		PROCESSOR_ACL_ENTRIES];
} __attribute__((__packed__));

struct boardcfg_orderid {
	u32 allowed;
} __attribute__((__packed__));

struct boardcfg_one_resasg {
	u16	start_resource;
	u16	num_resource;
} __attribute__((__packed__));

struct boardcfg_udma_resasg {
	struct boardcfg_one_resasg	udma_ext_ch;
	struct boardcfg_one_resasg	udma_tx_ch;
	struct boardcfg_one_resasg	udma_rx_ch;
	struct boardcfg_one_resasg	udma_h_ch;
	struct boardcfg_one_resasg	udma_rx_flow;
} __attribute__((__packed__));

struct boardcfg_ringacc_resasg {
	struct boardcfg_one_resasg	ringacc_tx_ring;
	struct boardcfg_one_resasg	ringacc_rx_ring;
	struct boardcfg_one_resasg	ringacc_gp_ring;
} __attribute__((__packed__));

struct boardcfg_proxy_resasg {
	struct boardcfg_one_resasg proxy;
} __attribute__((__packed__));

struct boardcfg_com_nav_resasg {
	struct boardcfg_udma_resasg	udma;
	struct boardcfg_ringacc_resasg	ringacc;
	struct boardcfg_proxy_resasg	proxy;
} __attribute__((__packed__));

struct boardcfg_main_nav_resasg {
	struct boardcfg_com_nav_resasg	nav;
	struct boardcfg_one_resasg	modss0_ia;
	struct boardcfg_one_resasg	modss1_ia;
	struct boardcfg_one_resasg	udmass0_ia;
} __attribute__((__packed__));

struct boardcfg_mcu_nav_resasg {
	struct boardcfg_com_nav_resasg	nav;
	struct boardcfg_one_resasg	udmass0_ia;
} __attribute__((__packed__));

struct boardcfg_one_host_main_resasg {
	u8				host_id;
	struct boardcfg_orderid		orderid;
	struct boardcfg_main_nav_resasg main_nav;
	struct boardcfg_one_resasg	wkup_gpio_ir;
	struct boardcfg_one_resasg	main_gpio_ir;
} __attribute__((__packed__));

struct boardcfg_one_host_mcu_resasg {
	u8				host_id;
	struct boardcfg_orderid		orderid;
	struct boardcfg_main_nav_resasg main_nav;
	struct boardcfg_one_resasg	main2mcu_ir;
} __attribute__((__packed__));

struct boardcfg_resasg {
	struct boardcfg_substructure_header	subhdr;
	struct boardcfg_one_host_main_resasg	main_cfg[
		BOARDCFG_MAX_MAIN_HOST_COUNT];
	struct boardcfg_one_host_mcu_resasg	mcu_cfg[
		BOARDCFG_MAX_MCU_HOST_COUNT];
} __attribute__((__packed__));

#define BOARDCFG_TRACE_DST_UART0                BIT(0)
#define BOARDCFG_TRACE_DST_I2C0_UART            BIT(1)
#define BOARDCFG_TRACE_DST_ITM                  BIT(2)
#define BOARDCFG_TRACE_DST_MEM                  BIT(3)

#define BOARDCFG_TRACE_SRC_PM                   BIT(0)
#define BOARDCFG_TRACE_SRC_RM                   BIT(1)
#define BOARDCFG_TRACE_SRC_SEC                  BIT(2)
#define BOARDCFG_TRACE_SRC_BASE                 BIT(3)
#define BOARDCFG_TRACE_SRC_USER                 BIT(4)
#define BOARDCFG_TRACE_SRC_SUPR                 BIT(5)

struct boardcfg_dbg_cfg {
	struct boardcfg_substructure_header	subhdr;
	u16					trace_dst_enables;
	u16					trace_src_enables;
} __attribute__((__packed__));

struct boardcfg_pmic_cfg {
	struct boardcfg_substructure_header subhdr;
} __attribute__((__packed__));

struct k3_boardcfg {
	struct boardcfg_abi_rev		rev;
	struct boardcfg_control		control;
	struct boardcfg_secproxy	secproxy;
	struct boardcfg_msmc		msmc;
	struct boardcfg_proc_acl	processor_acl_list;
	struct boardcfg_resasg		resasg;
	struct boardcfg_dbg_cfg		debug_cfg;
	struct boardcfg_pmic_cfg	pmic_cfg;
} __attribute__((__packed__));

/**
 * Export different board configuration structure
 */
extern const struct k3_boardcfg k3_boardcfg_data;
