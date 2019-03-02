// SPDX-License-Identifier: GPL-2.0+
/*
 * K3: Common utility routines
 *
 * (C) Copyright 2018 Linaro Ltd.
 * Sam Protsenko <semen.protsenko@linaro.org>
 */

#include <common.h>
#include <asm/arch/utils.h>

#ifdef CONFIG_FASTBOOT_FLASH
#ifdef CONFIG_FASTBOOT_FLASH_MMC

/*
 * FIXME: Code duplication w.r.t. arch/arm/mach-omap2/utils.c
 *        Think about extracting it to some common code later.
 */
static u32 k3_mmc_get_part_size(const char *part)
{
	int res;
	struct blk_desc *dev_desc;
	disk_partition_t info;
	u64 sz = 0;

	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid mmc device\n");
		return 0;
	}

	/* Check only for EFI (GPT) partition table */
	res = part_get_info_by_name_type(dev_desc, part, &info, PART_TYPE_EFI);
	if (res < 0)
		return 0;

	/* Calculate size in bytes */
	sz = (info.size * (u64)info.blksz);
	/* to KiB */
	sz >>= 10;

	return (u32)sz;
}

static void k3_set_fastboot_userdata_size(void)
{
	char buf[16];
	u32 sz_kb;

	sz_kb = k3_mmc_get_part_size("userdata");
	if (sz_kb == 0)
		return; /* probably it's not Android partition table */

	sprintf(buf, "%u", sz_kb);
	env_set("fastboot.userdata_size", buf);
}

#else

static inline void k3_set_fastboot_userdata_size(void)
{
}

#endif /* CONFIG_FASTBOOT_FLASH_MMC */

/*
 * TODO: Get variables from EEPROM or SoC registers instead of hard-code
 *       (like it's done in arch/arm/mach-omap2/utils.c).
 */
void k3_set_fastboot_vars(void)
{
	const char *cpu = "AM654";
	const char *secure = "GP";
	const char *board_rev;

	board_rev = env_get("board_rev");
	if (!board_rev)
		printf("Warning: fastboot.board_rev: unknown board revision\n");

	env_set("fastboot.cpu", cpu);
	env_set("fastboot.secure", secure);
	env_set("fastboot.board_rev", board_rev);

	k3_set_fastboot_userdata_size();
}

#endif /* CONFIG_FASTBOOT_FLASH */
