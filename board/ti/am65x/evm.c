/*
 * Board specific initialization for AM654 EVM
 *
 * Copyright (C) 2017-2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <dm.h>
#include <spl.h>
#include <usb.h>
#include <debug_uart.h>
#include <dt-bindings/pinctrl/k3-am6.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_DEBUG_UART_OMAP
void board_debug_uart_init(void)
{
	/*
	 * NOTE: For the PADCONFIG registers on K3 to be accessible the
	 * corresponding CTRL_MMR region must have gotten unlocked first.
	 */

	/* (P4) MCU_OSPI1_D1.MCU_UART0_RXD */
	writel(PIN_INPUT | MUX_MODE4,
	       WKUP_CTRL_MMR0_BASE + CTRLMMR_PADCONFIG(17));

	/* (P5) MCU_OSPI1_D2.MCU_UART0_TXD */
	writel(PIN_OUTPUT | MUX_MODE4,
	       WKUP_CTRL_MMR0_BASE + CTRLMMR_PADCONFIG(18));
}
#endif

int board_init(void)
{
	return 0;
}

int dram_init(void)
{
#ifdef CONFIG_PHYS_64BIT
	gd->ram_size = 0x100000000;
#else
	gd->ram_size = 0x80000000;
#endif

	return 0;
}

ulong board_get_usable_ram_top(ulong total_size)
{
#ifdef CONFIG_PHYS_64BIT
	/* Limit RAM used by U-Boot to the DDR low region */
	if (gd->ram_top > 0x100000000)
		return 0x100000000;
#endif

	return gd->ram_top;
}

int dram_init_banksize(void)
{
	/* Bank 0 declares the memory available in the DDR low region */
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = 0x80000000;

#ifdef CONFIG_PHYS_64BIT
	/* Bank 1 declares the memory available in the DDR high region */
	gd->bd->bi_dram[1].start = CONFIG_SYS_SDRAM_BASE1;
	gd->bd->bi_dram[1].size = 0x80000000;
#endif

	return 0;
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
#ifdef CONFIG_TARGET_AM654_A53_EVM
	if (!strcmp(name, "k3-am654-base-board"))
		return 0;
#endif
#ifdef CONFIG_TARGET_AM654_R5_EVM
	if (!strcmp(name, "k3-am654-r5-base-board"))
		return 0;
#endif

	return -1;
}
#endif

#ifdef CONFIG_USB_DWC3
int board_usb_init(int index, enum usb_init_type init)
{
	struct udevice *dev;
	int ret;

	if (init != USB_INIT_DEVICE)
		return 0;

	ret = uclass_get_device(UCLASS_USB_DEV_GENERIC, index, &dev);
	if (!dev || ret) {
		printf("No USB device found\n");
		return -ENODEV;
	}

	return 0;
}
#endif
