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
#include <remoteproc.h>
#include <linux/libfdt.h>
#include <image.h>
#include <asm/sections.h>
#include <asm/armv7_mpu.h>
#include <asm/arch/hardware.h>

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

#ifdef CONFIG_K3_LOAD_SYSFW
static int locate_system_controller_firmware(int *addr, int *len)
{
	struct image_header *header;
	int images, node;
	const void *fit;

	if (IS_ENABLED(CONFIG_SPL_SEPARATE_BSS))
		fit = (ulong *)&_image_binary_end;
	else
		fit = (ulong *)&__bss_end;

	header = (struct image_header *)fit;
	if (image_get_magic(header) != FDT_MAGIC) {
		debug("No FIT image appended to SPL\n");
		return -EINVAL;
	}

	/* find the node holding the images information */
	images = fdt_path_offset(fit, FIT_IMAGES_PATH);
	if (images < 0) {
		debug("%s: Cannot find /images node: %d\n", __func__, images);
		return -ENOENT;
	}

	/* Find the subnode holding system controller firmware */
	node = fdt_subnode_offset(fit, images, "sysfw");
	if (node < 0) {
		debug("%s: Cannot find fdt node sysfw in FIT: %d\n",
		      __func__, node);
		return -EINVAL;
	}

	fit_image_get_data_offset(fit, node, addr);
	*addr += (int)fit + ((fdt_totalsize(fit) + 3) & ~3);
	fit_image_get_data_size(fit, node, len);

	return 0;
}
#endif

void board_init_f(ulong dummy)
{
#ifdef CONFIG_K3_LOAD_SYSFW
	int ret, fw_addr, len;
#endif

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

#ifdef CONFIG_K3_LOAD_SYSFW
	/* Try to locate firmware image and load it to system controller */
	if (!locate_system_controller_firmware(&fw_addr, &len)) {
		debug("Firmware located. Now try to load\n");
		/*
		 * It is assumed that remoteproc device 0 is the corresponding
		 * system-controller that runs SYSFW.
		 * Make sure DT reflects the same.
		 */
		ret = rproc_dev_init(0);
		if (ret) {
			debug("rproc failed to be initialized: ret= %d\n",
			      ret);
			return;
		}

		ret = rproc_load(0, fw_addr, len);
		if (ret) {
			debug("Firmware failed to start on rproc: ret= %d\n",
			      ret);
			return;
		}

		ret = rproc_start(0);
		if (ret) {
			debug("Firmware init failed on rproc: ret= %d\n",
			      ret);
			return;
		}
	}
#endif

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

#ifdef CONFIG_K3_SPL_ATF
void __noreturn jump_to_image_no_args(struct spl_image_info *spl_image)
{
	int ret;

	/*
	 * It is assumed that remoteproc device 1 is the corresponding
	 * cortex A core which runs ATF. Make sure DT reflects the same.
	 */
	ret = rproc_dev_init(1);
	if (ret) {
		printf("%s: ATF failed to Initialize on rproc: ret= %d\n",
		       __func__, ret);
		hang();
	}

	ret = rproc_load(1, spl_image->entry_point, 0x200);
	if (ret) {
		printf("%s: ATF failed to load on rproc: ret= %d\n",
		       __func__, ret);
		hang();
	}

	/* Add an extra newline to differentiate the ATF logs from SPL*/
	printf("Starting ATF on ARM64 core...\n\n");

	ret = rproc_start(1);
	if (ret) {
		printf("%s: ATF failed to start on rproc: ret= %d\n",
		       __func__, ret);
		hang();
	}

	debug("ATF started. Wait indefiniely\n");
	while (1)
		asm volatile("wfe");
}
#endif
