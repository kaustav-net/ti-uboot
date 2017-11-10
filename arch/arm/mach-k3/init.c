/*
 * K3: Architecture initialization
 *
 * Copyright (C) 2017-2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <spl.h>
#include <asm/arch/hardware.h>
#include <asm/armv7_mpu.h>

#ifdef CONFIG_SPL_BUILD
#ifdef CONFIG_CPU_V7R
struct mpu_region_config k3_mpu_regions[16] = {
	/*
	 * Make all 4GB as Device Memory and not executable. We are overriding
	 * it with next region for any requirement.
	 */
	{0x00000000, REGION_0, XN_EN, PRIV_RW_USR_RW, SHARED_WRITE_BUFFERED,
	 REGION_4GB},

	/* SPL code area marking it as WB and Write allocate. */
	{CONFIG_SPL_TEXT_BASE, REGION_1, XN_DIS, PRIV_RW_USR_RW,
	 O_I_WB_RD_WR_ALLOC, REGION_8MB},

	/* U-Boot's code area marking it as WB and Write allocate */
	{CONFIG_SYS_SDRAM_BASE, REGION_2, XN_DIS, PRIV_RW_USR_RW,
	 O_I_WB_RD_WR_ALLOC, REGION_2GB},
	{0x0, 3, 0x0, 0x0, 0x0, 0x0},
	{0x0, 4, 0x0, 0x0, 0x0, 0x0},
	{0x0, 5, 0x0, 0x0, 0x0, 0x0},
	{0x0, 6, 0x0, 0x0, 0x0, 0x0},
	{0x0, 7, 0x0, 0x0, 0x0, 0x0},
	{0x0, 8, 0x0, 0x0, 0x0, 0x0},
	{0x0, 9, 0x0, 0x0, 0x0, 0x0},
	{0x0, 10, 0x0, 0x0, 0x0, 0x0},
	{0x0, 11, 0x0, 0x0, 0x0, 0x0},
	{0x0, 12, 0x0, 0x0, 0x0, 0x0},
	{0x0, 13, 0x0, 0x0, 0x0, 0x0},
	{0x0, 14, 0x0, 0x0, 0x0, 0x0},
	{0x0, 15, 0x0, 0x0, 0x0, 0x0},
};
#endif

static void mmr_unlock(u32 base, u32 partition)
{
	/* Translate the base address */
	phys_addr_t part_base = base + partition * CTRL_MMR0_PARTITION_SIZE;

	/* Unlock the requested partition if locked using two-step sequence */
	writel(CTRLMMR_LOCK_KICK0_UNLOCK_VAL, part_base + CTRLMMR_LOCK_KICK0);
	writel(CTRLMMR_LOCK_KICK1_UNLOCK_VAL, part_base + CTRLMMR_LOCK_KICK1);
}

static void ctrl_mmr_unlock(void)
{
	/* Unlock all WKUP_CTRL_MMR0 module registers */
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 0);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 1);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 2);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 3);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 6);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 7);

	/* Unlock all MCU_CTRL_MMR0 module registers */
	mmr_unlock(MCU_CTRL_MMR0_BASE, 0);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 1);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 2);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 6);

	/* Unlock all CTRL_MMR0 module registers */
	mmr_unlock(CTRL_MMR0_BASE, 0);
	mmr_unlock(CTRL_MMR0_BASE, 1);
	mmr_unlock(CTRL_MMR0_BASE, 2);
	mmr_unlock(CTRL_MMR0_BASE, 3);
	mmr_unlock(CTRL_MMR0_BASE, 6);
	mmr_unlock(CTRL_MMR0_BASE, 7);

}

static void store_boot_index_from_rom(void)
{
	u32 *boot_index = (u32 *)K3_BOOT_PARAM_TABLE_INDEX_VAL;

	*boot_index = *(u32 *)(CONFIG_BOOT_PARAM_TABLE_INDEX);
}

void board_init_f(ulong dummy)
{
	/*
	 * Cannot delay this further as there is a chance that
	 * BOOT_PARAM_TABLE_INDEX can be over written by SPL MALLOC section.
	 */
	store_boot_index_from_rom();

	/* Make all control module registers accessible */
	ctrl_mmr_unlock();

#ifdef CONFIG_CPU_V7R
	setup_mpu_regions(k3_mpu_regions, ARRAY_SIZE(k3_mpu_regions));
#endif

	/* Init DM early in-order to invoke system controller */
	spl_early_init();

	/* Prepare console output */
	preloader_console_init();
}

static u32 __get_backup_bootmedia(u32 devstat)
{
	u32 bkup_boot = (devstat & CTRLMMR_MAIN_DEVSTAT_BKUP_BOOTMODE_MASK) >>
			CTRLMMR_MAIN_DEVSTAT_BKUP_BOOTMODE_SHIFT;

	switch (bkup_boot) {
#define __BKUP_BOOT_DEVICE(n)			\
	case BACKUP_BOOT_DEVICE_##n:		\
		return BOOT_DEVICE_##n;
	__BKUP_BOOT_DEVICE(USB);
	__BKUP_BOOT_DEVICE(UART);
	__BKUP_BOOT_DEVICE(ETHERNET);
	__BKUP_BOOT_DEVICE(MMC2);
	__BKUP_BOOT_DEVICE(SPI);
	__BKUP_BOOT_DEVICE(HYPERFLASH);
	__BKUP_BOOT_DEVICE(I2C);
	};

	return BOOT_DEVICE_RAM;
}

static u32 __get_primary_bootmedia(u32 devstat)
{
	u32 bootmode = devstat & CTRLMMR_MAIN_DEVSTAT_BOOTMODE_MASK;

	if (bootmode == BOOT_DEVICE_OSPI || bootmode ==	BOOT_DEVICE_QSPI)
		bootmode = BOOT_DEVICE_SPI;

	return bootmode;
}

u32 spl_boot_device(void)
{
	u32 devstat = readl(CTRLMMR_MAIN_DEVSTAT);
	u32 bootindex = readl(K3_BOOT_PARAM_TABLE_INDEX_VAL);

	if (bootindex == K3_PRIMARY_BOOTMODE)
		return __get_primary_bootmedia(devstat);
	else
		return __get_backup_bootmedia(devstat);
}
#endif

#ifndef CONFIG_SYSRESET
void reset_cpu(ulong ignored)
{
}
#endif
