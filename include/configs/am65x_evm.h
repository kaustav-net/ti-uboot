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
#include <environment/ti/mmc.h>

#define CONFIG_ENV_SIZE			(128 << 10)
#define CONFIG_NR_DRAM_BANKS		2

/* SPL Loader Configuration */
#ifdef CONFIG_TARGET_AM654_A53_EVM
#define CONFIG_SPL_TEXT_BASE		0x80080000
#define CONFIG_SYS_SPI_U_BOOT_OFFS	0x300000
#else
#define CONFIG_SPL_TEXT_BASE		0x41c00000
#define CONFIG_SYS_SPI_U_BOOT_OFFS	0x100000
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

/* U-Boot general configuration */
#define EXTRA_ENV_AM65X_BOARD_SETTINGS					\
	"findfdt="							\
		"if test $board_name = am65x; then "			\
			"setenv name_fdt k3-am654-base-board.dtb; fi;"	\
		"if test $board_name = am65x_idk; then "         \
			"setenv name_fdt k3-am654-base-board.dtb; "	\
			"setenv name_overlays \"k3-am654-pcie-usb2.dtbo k3-am654-idk.dtbo\"; fi;"\
		"if test $board_name = am65x_evm; then "         \
			"setenv name_fdt k3-am654-base-board.dtb; "	\
			"setenv name_overlays \"k3-am654-pcie-usb3.dtbo k3-am654-evm-csi2-ov490.dtbo k3-am654-evm-oldi-lcd1evm.dtbo\"; "\
		"else if test $name_fdt = undefined; then "		\
			"echo WARNING: Could not determine device tree to use;"\
		"fi; fi; "						\
		"setenv fdtfile ${name_fdt};"				\
		"setenv overlay_files ${name_overlays}\0"		\
	"loadaddr=0x80080000\0"						\
	"fdtaddr=0x82000000\0"						\
	"overlayaddr=0x83000000\0"					\
	"name_kern=Image\0"						\
	"console=ttyS2,115200n8\0"					\
	"args_all=setenv optargs earlycon=ns16550a,mmio32,0x02800000\0" \
	"run_kern=booti ${loadaddr} ${rd_spec} ${fdtaddr}\0"

/* U-Boot MMC-specific configuration */
#define EXTRA_ENV_AM65X_BOARD_SETTINGS_MMC				\
	"boot=mmc\0"							\
	"mmcdev=1\0"							\
	"bootpart=1:2\0"						\
	"bootdir=/boot\0"						\
	"rd_spec=-\0"							\
	"init_mmc=run args_all args_mmc\0"				\
	"get_fdt_mmc=load mmc ${bootpart} ${fdtaddr} ${bootdir}/${name_fdt}\0" \
	"get_overlay_mmc="						\
		"fdt address ${fdtaddr};"				\
		"fdt resize 0x100000;"					\
		"for overlay in $overlay_files;"			\
		"do;"							\
		"load mmc ${bootpart} ${overlayaddr} ${bootdir}/${overlay};"	\
		"fdt apply ${overlayaddr};"				\
		"done;\0"						\
	"get_kern_mmc=load mmc ${bootpart} ${loadaddr} "		\
		"${bootdir}/${name_kern}\0"

/* Incorporate settings into the U-Boot environment */
#define CONFIG_EXTRA_ENV_SETTINGS					\
	DEFAULT_MMC_TI_ARGS						\
	EXTRA_ENV_AM65X_BOARD_SETTINGS					\
	EXTRA_ENV_AM65X_BOARD_SETTINGS_MMC

/* Non Kconfig SF configs */
#define CONFIG_SPL_SPI_LOAD
#define CONFIG_SF_DEFAULT_SPEED		0
#define CONFIG_SF_DEFAULT_MODE		0

/* Now for the remaining common defines */
#include <configs/ti_armv7_common.h>

#endif /* __CONFIG_AM654_EVM_H */
