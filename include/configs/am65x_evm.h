/*
 * Configuration header file for K3 AM654 EVM
 *
 * Copyright (C) 2017-2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __CONFIG_AM654_EVM_H
#define __CONFIG_AM654_EVM_H

#include <linux/sizes.h>
#include <config_distro_bootcmd.h>

#define CONFIG_ENV_SIZE			(128 << 10)
#define CONFIG_NR_DRAM_BANKS		2

/* SPL Loader Configuration */
#ifdef CONFIG_TARGET_AM654_A53_EVM
#define CONFIG_SPL_TEXT_BASE		0x80080000
#else
#define CONFIG_SPL_TEXT_BASE		0x41c00000
#endif

/* Clock Defines */
#define V_OSCK				24000000
#define V_SCLK				V_OSCK

#ifdef CONFIG_K3_SPL_ATF
#define CONFIG_SPL_FS_LOAD_PAYLOAD_NAME	"tispl.bin"
#endif

#define CONFIG_SKIP_LOWLEVEL_INIT

#define CONFIG_SPL_MAX_SIZE		CONFIG_MAX_DOWNLODABLE_IMAGE_SIZE
#define CONFIG_SYS_INIT_SP_ADDR         (CONFIG_SPL_TEXT_BASE +	\
					CONFIG_NON_SECURE_MSRAM_SIZE - 4)

/* Now for the remaining common defines */
#include <configs/ti_armv7_common.h>

#endif /* __CONFIG_AM654_EVM_H */
