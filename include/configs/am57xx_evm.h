/*
 * (C) Copyright 2014
 * Texas Instruments Incorporated.
 * Felipe Balbi <balbi@ti.com>
 *
 * Configuration settings for the TI Beagle x15 board.
 * See ti_omap5_common.h for omap5 common settings.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_AM57XX_EVM_H
#define __CONFIG_AM57XX_EVM_H

#define CONFIG_DRA7XX

#define CONFIG_IODELAY_RECALIBRATION

#define CONFIG_BOARD_EARLY_INIT_F

#define CONFIG_NR_DRAM_BANKS		2

#define CONFIG_ENV_SIZE			(64 << 10)
#define CONFIG_ENV_IS_IN_FAT
#define FAT_ENV_INTERFACE		"mmc"
#define FAT_ENV_DEVICE_AND_PART		"0:1"
#define FAT_ENV_FILE			"uboot.env"

#define CONSOLEDEV			"ttyO2"
#define CONFIG_SYS_NS16550_COM1		UART1_BASE	/* Base EVM has UART0 */
#define CONFIG_SYS_NS16550_COM2		UART2_BASE	/* UART2 */
#define CONFIG_SYS_NS16550_COM3		UART3_BASE	/* UART3 */
#define CONFIG_BAUDRATE			115200

#define CONFIG_SYS_OMAP_ABE_SYSCK

/* Define the default GPT table for eMMC */
#define PARTS_DEFAULT \
	"uuid_disk=${uuid_gpt_disk};" \
	"name=rootfs,start=2MiB,size=-,uuid=${uuid_gpt_rootfs}"

#define DFU_ALT_INFO_MMC \
	"dfu_alt_info_mmc=" \
	"boot part 0 1;" \
	"rootfs part 0 2;" \
	"MLO fat 0 1;" \
	"MLO.raw raw 0x100 0x100;" \
	"u-boot.img.raw raw 0x300 0x400;" \
	"spl-os-args.raw raw 0x80 0x80;" \
	"spl-os-image.raw raw 0x900 0x2000;" \
	"spl-os-args fat 0 1;" \
	"spl-os-image fat 0 1;" \
	"u-boot.img fat 0 1;" \
	"uEnv.txt fat 0 1\0"

#define DFU_ALT_INFO_EMMC \
	"dfu_alt_info_emmc=" \
	"rawemmc raw 0 3751936;" \
	"boot part 1 1;" \
	"rootfs part 1 2;" \
	"MLO fat 1 1;" \
	"MLO.raw raw 0x100 0x100;" \
	"u-boot.img.raw raw 0x300 0x400;" \
	"spl-os-args.raw raw 0x80 0x80;" \
	"spl-os-image.raw raw 0x900 0x2000;" \
	"spl-os-args fat 1 1;" \
	"spl-os-image fat 1 1;" \
	"u-boot.img fat 1 1;" \
	"uEnv.txt fat 1 1\0"

#define DFU_ALT_INFO_QSPI \
	"dfu_alt_info_qspi=" \
	"MLO raw 0x0 0x040000;" \
	"u-boot.img raw 0x040000 0x0100000;" \
	"u-boot-spl-os raw 0x140000 0x080000;" \
	"u-boot-env raw 0x1C0000 0x010000;" \
	"u-boot-env.backup raw 0x1D0000 0x010000;" \
	"kernel raw 0x1E0000 0x800000\0"

#define DFU_ALT_INFO_RAM \
	"dfu_alt_info_ram=" \
	"kernel ram 0x80200000 0x4000000;" \
	"fdt ram 0x80f80000 0x80000;" \
	"ramdisk ram 0x81000000 0x4000000\0"

#define DFUARGS \
	"dfu_bufsiz=0x10000\0" \
	DFU_ALT_INFO_MMC \
	DFU_ALT_INFO_EMMC \
	DFU_ALT_INFO_QSPI \
	DFU_ALT_INFO_RAM

#include <configs/ti_omap5_common.h>

/* Enhance our eMMC support / experience. */
#define CONFIG_CMD_GPT
#define CONFIG_EFI_PARTITION

/* CPSW Ethernet */
#define CONFIG_BOOTP_DNS		/* Configurable parts of CMD_DHCP */
#define CONFIG_BOOTP_DNS2
#define CONFIG_BOOTP_SEND_HOSTNAME
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_SUBNETMASK
#define CONFIG_NET_RETRY_COUNT		10
#define CONFIG_DRIVER_TI_CPSW		/* Driver for IP block */
#define CONFIG_MII			/* Required in net/eth.c */
#define CONFIG_PHY_GIGE			/* per-board part of CPSW */
#define CONFIG_PHYLIB
#define PHY_ANEG_TIMEOUT	8000	/* PHY needs longer aneg time at 1G */

#define CONFIG_SUPPORT_EMMC_BOOT

/* USB xHCI HOST */
#define CONFIG_USB_HOST
#define CONFIG_USB_XHCI_DWC3
#define CONFIG_USB_XHCI
#define CONFIG_USB_XHCI_OMAP
#define CONFIG_USB_STORAGE
#define CONFIG_SYS_USB_XHCI_MAX_ROOT_PORTS 2

#define CONFIG_OMAP_USB_PHY
#define CONFIG_OMAP_USB3PHY1_HOST

/* USB Device Firmware Update support */
#define CONFIG_USB_FUNCTION_DFU
#define CONFIG_DFU_RAM

#ifndef CONFIG_SPL_BUILD
#define CONFIG_DFU_MMC
#define CONFIG_DFU_SF
#endif

/* SATA */
#define CONFIG_BOARD_LATE_INIT
#define CONFIG_CMD_SCSI
#define CONFIG_LIBATA
#define CONFIG_SCSI_AHCI
#define CONFIG_SCSI_AHCI_PLAT
#define CONFIG_SYS_SCSI_MAX_SCSI_ID	1
#define CONFIG_SYS_SCSI_MAX_LUN		1
#define CONFIG_SYS_SCSI_MAX_DEVICE	(CONFIG_SYS_SCSI_MAX_SCSI_ID * \
						CONFIG_SYS_SCSI_MAX_LUN)

/* EEPROM */
#define CONFIG_EEPROM_CHIP_ADDRESS 0x50
#define CONFIG_EEPROM_BUS_ADDRESS 0

/*
 * Default to using SPI for environment, etc.
 * 0x000000 - 0x040000 : QSPI.SPL (256KiB)
 * 0x040000 - 0x140000 : QSPI.u-boot (1MiB)
 * 0x140000 - 0x1C0000 : QSPI.u-boot-spl-os (512KiB)
 * 0x1C0000 - 0x1D0000 : QSPI.u-boot-env (64KiB)
 * 0x1D0000 - 0x1E0000 : QSPI.u-boot-env.backup1 (64KiB)
 * 0x1E0000 - 0x9E0000 : QSPI.kernel (8MiB)
 * 0x9E0000 - 0x2000000 : USERLAND
 */
#define CONFIG_SYS_SPI_KERNEL_OFFS      0x1E0000
#define CONFIG_SYS_SPI_ARGS_OFFS        0x140000
#define CONFIG_SYS_SPI_ARGS_SIZE        0x80000

#ifdef CONFIG_SPL_BUILD
#undef CONFIG_DM_SPI
#undef CONFIG_DM_SPI_FLASH
#undef CONFIG_SPL_MAX_SIZE
#define CONFIG_SPL_MAX_SIZE             (256 << 10) /* 256 KiB */
#endif

/* SPI SPL */
#define CONFIG_SPL_SPI_SUPPORT
#define CONFIG_SPL_DMA_SUPPORT
#define CONFIG_TI_EDMA3
#define CONFIG_SPL_SPI_LOAD
#define CONFIG_SPL_SPI_FLASH_SUPPORT
#define CONFIG_SYS_SPI_U_BOOT_OFFS     0x40000

/* SPI */
#undef	CONFIG_OMAP3_SPI
#define CONFIG_TI_SPI_MMAP
#define CONFIG_SF_DEFAULT_SPEED                76800000
#define CONFIG_SF_DEFAULT_MODE                 SPI_MODE_0
#define CONFIG_QSPI_QUAD_SUPPORT

#define CONFIG_DRA7XX_DWC2

#endif /* __CONFIG_AM57XX_EVM_H */
