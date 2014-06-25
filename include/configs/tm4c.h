/*
 * (C) Copyright 2012 Emcraft Systems
 *
 * Configuration settings for Emcraft Systems'
 * SmartFusion system-on-module (SOM).
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * Disable debug messages
 */
#define DEBUG

/*
 * This is an ARM Cortex-M3 CPU core
 */
#define CONFIG_SYS_ARMCORTEXM3

/*
 * This is the Actel SmartFusion (aka A2F) device
 */
#define CONFIG_SYS_TM4C

/*
 * Display CPU and Board information
 */
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_SYS_PROMPT		"U-Boot# "

/*
 * We want to call the CPU specific initialization
 */
#define CONFIG_ARCH_CPU_INIT

/*
 * Number of clock ticks in 1 sec
 */
#define CONFIG_SYS_HZ			1000

/*
 * Enable/disable h/w watchdog
 */
#undef CONFIG_HW_WATCHDOG

/*
 * No interrupts
 */
#undef CONFIG_USE_IRQ

/*
 * Memory layout configuration
 * NVM == "non-volatile memory", aka normal flash
 */ 
#define CONFIG_MEM_NVM_BASE		0x00000000
#define CONFIG_MEM_NVM_LEN		(1024 << 10)	/* 1 MB */

#define CONFIG_MEM_RAM_BASE		0x20000000
/* XXX: Understand breakdown, confirm base location */
/* XXX: Total size is 256KB */
#define CONFIG_MEM_RAM_LEN		(16 * 1024)
#define CONFIG_MEM_RAM_BUF_LEN		(32 * 1024)
#define CONFIG_MEM_MALLOC_LEN		(12 * 1024)
#define CONFIG_MEM_STACK_LEN		(4 * 1024)
/* XXX: New */
#define CONFIG_MEM_RAM_SIZE		(256 << 10)

/*
 * malloc() pool size
 */
#define CONFIG_SYS_MALLOC_LEN		CONFIG_MEM_MALLOC_LEN

/*
 * Configuration of the external memory
 */
/* XXX: We have none, so.. */
#if 0
#define CONFIG_NR_DRAM_BANKS		1
#define CONFIG_SYS_RAM_BASE		0x70000000
#define CONFIG_SYS_RAM_SIZE		(16 * 1024 * 1024)
#else
#define CONFIG_NR_DRAM_BANKS		1
#define CONFIG_SYS_RAM_BASE		CONFIG_MEM_RAM_BASE
#define CONFIG_SYS_RAM_SIZE		CONFIG_MEM_RAM_SIZE
#endif

/*
 * External Memory Controller settings
 * Slow, safe timings for external SRAM
 * XXX: A2F SOM still
#define CONFIG_SYS_EMC0CS0CR		0x00002aad
 */

/*
 * Timings for the external Flash
 */
#define CONFIG_SYS_EMC0CS1CR		0x00011147

#if 0
/*
 * Settings for the CFI Flash driver
 */
#define CONFIG_SYS_FLASH_CFI		1
#define CONFIG_FLASH_CFI_DRIVER		1
#define CONFIG_SYS_FLASH_CFI_WIDTH	FLASH_CFI_16BIT
#define CONFIG_SYS_FLASH_BANKS_LIST	{ CONFIG_SYS_FLASH_BANK1_BASE }
#define CONFIG_SYS_MAX_FLASH_BANKS	1
#define CONFIG_SYS_MAX_FLASH_SECT	128
#define CONFIG_SYS_FLASH_CFI_AMD_RESET	1
/* Use writebuffer, otherwise the flash is too slow */
#define CONFIG_SYS_FLASH_USE_BUFFER_WRITE
#else
#define CONFIG_SYS_NO_FLASH
#endif

#if 0
/*
 * U-boot environment configuration
 */
#define CONFIG_ENV_IS_IN_FLASH		1
#define CONFIG_ENV_ADDR			CONFIG_SYS_FLASH_BANK1_BASE
#define CONFIG_ENV_SIZE			0x1000
#define CONFIG_INFERNO			1
#define CONFIG_ENV_OVERWRITE		1
#else
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_SIZE			0x1000
#endif

/*
 * Serial console configuration
 */
#define CONFIG_SYS_NS16550		1
#undef CONFIG_NS16550_MIN_FUNCTIONS
#define CONFIG_SYS_NS16550_SERIAL	1
#define CONFIG_SYS_NS16550_REG_SIZE     (-4)
#define CONFIG_SYS_NS16550_CLK		clock_get(CLOCK_PCLK0)
#define CONFIG_CONS_INDEX               1
#define CONFIG_SYS_NS16550_COM1         0x40000000
#define CONFIG_BAUDRATE                 115200
#define CONFIG_SYS_BAUDRATE_TABLE       { 9600, 19200, 38400, 57600, 115200 }

/*
 * Console I/O buffer size
 */
#define CONFIG_SYS_CBSIZE		256

/*
 * Print buffer size
 */
#define CONFIG_SYS_PBSIZE               (CONFIG_SYS_CBSIZE + \
                                        sizeof(CONFIG_SYS_PROMPT) + 16)

#if 0
/*
 * Ethernet driver configuration
 */
#define CONFIG_NET_MULTI
#define CONFIG_CORE10100		1

/*
 * Keep Rx & Tx buffers in internal RAM
 */
#define CONFIG_CORE10100_INTRAM_ADDRESS	0x20008000
#define CONFIG_BITBANGMII		1
#define CONFIG_BITBANGMII_MULTI		1
#endif

#define CONFIG_SYS_MEMTEST_START	CONFIG_MEM_RAM_BASE
#define CONFIG_SYS_MEMTEST_END		(CONFIG_MEM_RAM_BASE + \
					CONFIG_MEM_RAM_SIZE)

/*
 * Needed by "loadb"
 */
#define CONFIG_SYS_LOAD_ADDR		CONFIG_MEM_RAM_BASE

/*
 * Monitor is actually in eNVM. In terms of U-Boot, it is neither "flash",
 * not RAM, but CONFIG_SYS_MONITOR_BASE must be defined.
 */
#define CONFIG_SYS_MONITOR_BASE		0x0

/*
 * Monitor is not in flash. Needs to define this to prevent
 * U-Boot from running flash_protect() on the monitor code.
 */
#define CONFIG_MONITOR_IS_IN_RAM	1

/*
 * Enable all those monitor commands that are needed
 */
#include <config_cmd_default.h>
#undef CONFIG_CMD_BOOTD
#undef CONFIG_CMD_CONSOLE
#undef CONFIG_CMD_ECHO
#undef CONFIG_CMD_EDITENV
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_IMI
#undef CONFIG_CMD_ITEST
#undef CONFIG_CMD_IMLS
#undef CONFIG_CMD_LOADS
#undef CONFIG_CMD_MISC
#undef CONFIG_CMD_NET
#undef CONFIG_CMD_NFS
#undef CONFIG_CMD_SOURCE
#undef CONFIG_CMD_XIMG
#undef CONFIG_CMD_SOMTEST

/*
 * To save memory disable long help
 */
#undef CONFIG_SYS_LONGHELP

/*
 * Max number of command args
 */
#define CONFIG_SYS_MAXARGS		16

/*
 * Auto-boot sequence configuration
 */
#define CONFIG_BOOTDELAY		-1
#define CONFIG_ZERO_BOOTDELAY_CHECK
#define CONFIG_HOSTNAME			tm4c

#endif /* __CONFIG_H */
