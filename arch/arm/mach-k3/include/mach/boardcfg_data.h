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
#define BOARDCFG_RM_HOST_CFG_MAGIC_NUM		0x4C41
#define BOARDCFG_RM_RESASG_MAGIC_NUM		0x7B25
#define BOARDCFG_CONTROL_MAGIC_NUM		0xC1D3
#define BOARDCFG_SECPROXY_MAGIC_NUM		0x1207
#define BOARDCFG_MSMC_MAGIC_NUM			0xA5C3
#define BOARDCFG_PROC_ACL_MAGIC_NUM		0xF1EA
#define BOARDCFG_HOST_HIERARCHY_MAGIC_NUM       0x8D27
#define BOARDCFG_RESASG_MAGIC_NUM		0x4C41
#define BOARDCFG_DBG_CFG_MAGIC_NUM		0x020C
#define BOARDCFG_PMIC_CFG_MAGIC_NUM		0x3172

struct boardcfg_substructure_header {
	u16	magic;
	u16	size;
} __attribute__((__packed__));

struct boardcfg_abi_rev {
	u8	boardcfg_abi_maj;
	u8	boardcfg_abi_min;
} __attribute__((__packed__));

/**
 * Definitions, types, etc. as used for general board configuration
 */
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


#define BOARDCFG_TRACE_DST_UART0                BIT(0)
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

struct k3_boardcfg {
	struct boardcfg_abi_rev			rev;
	struct boardcfg_control			control;
	struct boardcfg_secproxy		secproxy;
	struct boardcfg_msmc			msmc;
	struct boardcfg_dbg_cfg			debug_cfg;
} __attribute__((__packed__));

/**
 * Definitions, types, etc. as used for resource assignment
 */
#define HOST_ID_DMSC				0
#define HOST_ID_R5_0				3
#define HOST_ID_R5_1				4
#define HOST_ID_R5_2				5
#define HOST_ID_R5_3				6
#define HOST_ID_A53_0				10
#define HOST_ID_A53_1				11
#define HOST_ID_A53_2				12
#define HOST_ID_A53_3				13
#define HOST_ID_A53_4				14
#define HOST_ID_A53_5				15
#define HOST_ID_A53_6				16
#define HOST_ID_A53_7				17
#define HOST_ID_GPU_0				30
#define HOST_ID_GPU_1				31
#define HOST_ID_ICSSG_0				50
#define HOST_ID_ICSSG_1				51
#define HOST_ID_ICSSG_2				52

struct boardcfg_rm_host_cfg_entry {
	u8	host_id;
	u8	allowed_atype;
	u16	allowed_qos;
	u32	allowed_orderid;
	u16	allowed_priority;
	u8	allowed_sched_priority;
} __attribute__((__packed__));

#define BOARDCFG_RM_HOST_CFG_ENTRIES (32U)

struct boardcfg_rm_host_cfg {
	struct boardcfg_substructure_header	subhdr;
	struct boardcfg_rm_host_cfg_entry
				 host_cfg_entries[BOARDCFG_RM_HOST_CFG_ENTRIES];
};

#define RESASG_TYPE_SHIFT			0x0006
#define RESASG_TYPE_MASK			0xFFC0
#define RESASG_SUBTYPE_SHIFT			0x0000
#define RESASG_SUBTYPE_MASK			0x003F

#define RESASG_UTYPE(type, subtype) \
	(((type << RESASG_TYPE_SHIFT) & RESASG_TYPE_MASK) | \
	 ((subtype << RESASG_SUBTYPE_SHIFT) & RESASG_SUBTYPE_MASK))

enum resasg_types {
	RESASG_TYPE_MAIN_NAV_UDMASS_IA0 = 0x000,
	RESASG_TYPE_MAIN_NAV_MODSS_IA0 = 0x001,
	RESASG_TYPE_MAIN_NAV_MODSS_IA1 = 0x002,
	RESASG_TYPE_MCU_NAV_UDMASS_IA0 = 0x003,
	RESASG_TYPE_MAIN_NAV_MCRC = 0x004,
	RESASG_TYPE_MCU_NAV_MCRC = 0x005,
	RESASG_TYPE_MAIN_NAV_UDMAP = 0x006,
	RESASG_TYPE_MCU_NAV_UDMAP = 0x007,
	RESASG_TYPE_MSMC = 0x008,
	RESASG_TYPE_MAIN_NAV_RA = 0x009,
	RESASG_TYPE_MCU_NAV_RA = 0x00A,
	RESASG_TYPE_GIC_IRQ = 0x00B,
	RESASG_TYPE_PULSAR_C0_IRQ = 0x00C,
	RESASG_TYPE_PULSAR_C1_IRQ = 0x00D,
	RESASG_TYPE_ICSSG0_IRQ = 0x00E,
	RESASG_TYPE_ICSSG1_IRQ = 0x00F,
	RESASG_TYPE_ICSSG2_IRQ = 0x010,
	RESASG_TYPE_MAX = 0x3FF
};

enum resasg_subtype_main_nav_udmass_ia0 {
	RESASG_SUBTYPE_MAIN_NAV_UDMASS_IA0_VINT = 0x00,
	RESASG_SUBTYPE_MAIN_NAV_UDMASS_IA0_SEVI = 0x01,
	RESASG_SUBTYPE_MAIN_NAV_UDMASS_IA0_MEVI = 0x02,
	RESASG_SUBTYPE_MAIN_NAV_UDMASS_IA0_GEVI = 0x03,
	RESASG_SUBYTPE_MAIN_NAV_UDMASS_IA0_CNT = 0x04,
};

enum resasg_subtype_main_nav_modss_ia0 {
	RESASG_SUBTYPE_MAIN_NAV_MODSS_IA0_VINT = 0x00,
	RESASG_SUBTYPE_MAIN_NAV_MODSS_IA0_SEVI = 0x01,
	RESASG_SUBYTPE_MAIN_NAV_MODSS_IA0_CNT = 0x02,
};

enum resasg_subtype_main_nav_modss_ia1 {
	RESASG_SUBTYPE_MAIN_NAV_MODSS_IA1_VINT = 0x00,
	RESASG_SUBTYPE_MAIN_NAV_MODSS_IA1_SEVI = 0x01,
	RESASG_SUBYTPE_MAIN_NAV_MODSS_IA1_CNT = 0x02,
};

enum resasg_subtype_mcu_nav_udmass_ia0 {
	RESASG_SUBTYPE_MCU_NAV_UDMASS_IA0_VINT = 0x00,
	RESASG_SUBTYPE_MCU_NAV_UDMASS_IA0_SEVI = 0x01,
	RESASG_SUBTYPE_MCU_NAV_UDMASS_IA0_MEVI = 0x02,
	RESASG_SUBTYPE_MCU_NAV_UDMASS_IA0_GEVI = 0x03,
	RESASG_SUBYTPE_MCU_NAV_UDMASS_IA0_CNT = 0x04,
};

enum resasg_subtype_main_nav_mcrc {
	RESASG_SUBTYPE_MAIN_NAV_MCRC_LEVI = 0x00,
	RESASG_SUBYTPE_MAIN_NAV_MCRC_CNT = 0x01,
};

enum resasg_subtype_mcu_nav_mcrc {
	RESASG_SUBTYPE_MCU_NAV_MCRC_LEVI = 0x00,
	RESASG_SUBYTPE_MCU_NAV_MCRC_CNT = 0x01,
};

enum resasg_subtype_main_nav_udmap {
	RESASG_SUBTYPE_MAIN_NAV_UDMAP_TRIGGER = 0x00,
	RESASG_SUBTYPE_MAIN_NAV_UDMAP_TX_HCHAN = 0x01,
	RESASG_SUBTYPE_MAIN_NAV_UDMAP_TX_CHAN = 0x02,
	RESASG_SUBTYPE_MAIN_NAV_UDMAP_TX_ECHAN = 0x03,
	RESASG_SUBTYPE_MAIN_NAV_UDMAP_RX_HCHAN = 0x04,
	RESASG_SUBTYPE_MAIN_NAV_UDMAP_RX_CHAN = 0x05,
	RESASG_SUBTYPE_MAIN_NAV_UDMAP_RX_FLOW_COMMON = 0x06,
	RESASG_SUBTYPE_MAIN_NAV_UDMAP_INVALID_FLOW_OES = 0x07,
	RESASG_SUBYTPE_MAIN_NAV_UDMAP_CNT = 0x08,
};

enum resasg_subtype_mcu_nav_udmap {
	RESASG_SUBTYPE_MCU_NAV_UDMAP_TRIGGER = 0x00,
	RESASG_SUBTYPE_MCU_NAV_UDMAP_TX_HCHAN = 0x01,
	RESASG_SUBTYPE_MCU_NAV_UDMAP_TX_CHAN = 0x02,
	RESASG_SUBTYPE_MCU_NAV_UDMAP_RX_HCHAN = 0x03,
	RESASG_SUBTYPE_MCU_NAV_UDMAP_RX_CHAN = 0x04,
	RESASG_SUBTYPE_MCU_NAV_UDMAP_RX_FLOW_COMMON = 0x05,
	RESASG_SUBTYPE_MCU_NAV_UDMAP_INVALID_FLOW_OES = 0x06,
	RESASG_SUBYTPE_MCU_NAV_UDMAP_CNT = 0x07,
};

enum resasg_subtype_msmc {
	RESASG_SUBTYPE_MSMC_DRU = 0x00,
	RESASG_SUBYTPE_MSMC_CNT = 0x01,
};

enum resasg_subtype_main_nav_ra {
	RESASG_SUBTYPE_MAIN_NAV_RA_RING_UDMAP_TX = 0x00,
	RESASG_SUBTYPE_MAIN_NAV_RA_RING_UDMAP_RX = 0x01,
	RESASG_SUBTYPE_MAIN_NAV_RA_RING_GP = 0x02,
	RESASG_SUBTYPE_MAIN_NAV_RA_ERROR_OES = 0x03,
	RESASG_SUBYTPE_MAIN_NAV_RA_CNT = 0x04,
};

enum resasg_subtype_mcu_nav_ra {
	RESASG_SUBTYPE_MCU_NAV_RA_RING_UDMAP_TX = 0x00,
	RESASG_SUBTYPE_MCU_NAV_RA_RING_UDMAP_RX = 0x01,
	RESASG_SUBTYPE_MCU_NAV_RA_RING_GP = 0x02,
	RESASG_SUBTYPE_MCU_NAV_RA_ERROR_OES = 0x03,
	RESASG_SUBYTPE_MCU_NAV_RA_CNT = 0x04,
};

enum resasg_subtype_gic_irq {
	RESASG_SUBTYPE_GIC_IRQ_MAIN_NAV_SET0 = 0x00,
	RESASG_SUBTYPE_GIC_IRQ_MAIN_GPIO = 0x01,
	RESASG_SUBTYPE_GIC_IRQ_MAIN_NAV_SET1 = 0x02,
	RESASG_SUBTYPE_GIC_IRQ_COMP_EVT = 0x03,
	RESASG_SUBTYPE_GIC_IRQ_WKUP_GPIO = 0x04,
	RESASG_SUBYTPE_GIC_IRQ_CNT = 0x05,
};

enum resasg_subtype_pulsar_c0_irq {
	RESASG_SUBTYPE_PULSAR_C0_IRQ_MCU_NAV = 0x00,
	RESASG_SUBTYPE_PULSAR_C0_IRQ_WKUP_GPIO = 0x01,
	RESASG_SUBTYPE_PULSAR_C0_IRQ_MAIN2MCU_LVL = 0x02,
	RESASG_SUBTYPE_PULSAR_C0_IRQ_MAIN2MCU_PLS = 0x03,
	RESASG_SUBYTPE_PULSAR_C0_IRQ_CNT = 0x04,
};

enum resasg_subtype_pulsar_c1_irq {
	RESASG_SUBTYPE_PULSAR_C1_IRQ_MCU_NAV = 0x00,
	RESASG_SUBTYPE_PULSAR_C1_IRQ_WKUP_GPIO = 0x01,
	RESASG_SUBTYPE_PULSAR_C1_IRQ_MAIN2MCU_LVL = 0x02,
	RESASG_SUBTYPE_PULSAR_C1_IRQ_MAIN2MCU_PLS = 0x03,
	RESASG_SUBYTPE_PULSAR_C1_IRQ_CNT = 0x04,
};

enum resasg_subtype_icssg0_irq {
	RESASG_SUBTYPE_ICSSG0_IRQ_MAIN_NAV = 0x00,
	RESASG_SUBTYPE_ICSSG0_IRQ_MAIN_GPIO = 0x01,
	RESASG_SUBYTPE_ICSSG0_IRQ_CNT = 0x02,
};

enum resasg_subtype_icssg1_irq {
	RESASG_SUBTYPE_ICSSG1_IRQ_MAIN_NAV = 0x00,
	RESASG_SUBTYPE_ICSSG1_IRQ_MAIN_GPIO = 0x01,
	RESASG_SUBYTPE_ICSSG1_IRQ_CNT = 0x02,
};

enum resasg_subtype_icssg2_irq {
	RESASG_SUBTYPE_ICSSG2_IRQ_MAIN_NAV = 0x00,
	RESASG_SUBTYPE_ICSSG2_IRQ_MAIN_GPIO = 0x01,
	RESASG_SUBYTPE_ICSSG2_IRQ_CNT = 0x02,
};

struct boardcfg_rm_resasg_entry {
	u16					start_resource;
	u16					num_resource;
	u16					type;
	u8					host_id;
	u8					reserved;
};

struct boardcfg_rm_resasg {
	struct boardcfg_substructure_header	subhdr;
	u16					resasg_entries_size;
	u16					reserved;
	struct boardcfg_rm_resasg_entry		resasg_entries[];
} __attribute__((__packed__));

struct k3_boardcfg_rm {
	struct boardcfg_abi_rev			rev;
	struct boardcfg_rm_host_cfg		host_cfg;
	struct boardcfg_rm_resasg		resasg;
} __attribute__((__packed__));

#define AM65_BOARDCFG_RM_RESASG_ENTRIES		59

/*
 * This is essentially 'struct k3_boardcfg_rm', but modified to pull
 * .resasg_entries which is a member of 'struct boardcfg_rm_resasg' into
 * the outer structure for easier explicit initialization.
 */
struct am65_boardcfg_rm_local {
	struct k3_boardcfg_rm			rm_boardcfg;
	struct boardcfg_rm_resasg_entry
				resasg_entries[AM65_BOARDCFG_RM_RESASG_ENTRIES];
};

/**
 * Definitions, types, etc. as used for the security configuration
 */
#define PROCESSOR_ACL_SECONDARY_MASTERS_MAX	3

struct boardcfg_proc_acl_entry {
	u8					processor_id;
	u8					proc_access_master;
	u8	     proc_access_secondary[PROCESSOR_ACL_SECONDARY_MASTERS_MAX];
} __attribute__((__packed__));

#define PROCESSOR_ACL_ENTRIES			32

struct boardcfg_proc_acl {
	struct boardcfg_substructure_header	subhdr;
	struct boardcfg_proc_acl_entry  proc_acl_entries[PROCESSOR_ACL_ENTRIES];
} __attribute__((__packed__));

struct boardcfg_host_hierarchy_entry {
	u8					host_id;
	u8					supervisor_host_id;
} __attribute__((__packed__));

#define HOST_HIERARCHY_ENTRIES			32

struct boardcfg_host_hierarchy {
	struct boardcfg_substructure_header	subhdr;
	struct boardcfg_host_hierarchy_entry
				 host_hierarchy_entries[HOST_HIERARCHY_ENTRIES];
} __attribute__((__packed__));

struct k3_boardcfg_security {
	struct boardcfg_abi_rev			rev;
	struct boardcfg_proc_acl		processor_acl_list;
	struct boardcfg_host_hierarchy		host_hierarchy;
} __attribute__((__packed__));

/**
 * Definitions, types, etc. as used for PM configuration
 */
struct k3_boardcfg_pm {
	struct boardcfg_abi_rev			rev;
} __attribute__((__packed__));

/**
 * Export different board configuration structures
 */
extern const struct k3_boardcfg am65_boardcfg_data;
extern const struct am65_boardcfg_rm_local am65_boardcfg_rm_data;
extern const struct k3_boardcfg_security am65_boardcfg_security_data;
extern const struct k3_boardcfg_pm am65_boardcfg_pm_data;
