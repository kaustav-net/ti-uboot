/*
 * (C) Copyright 2017
 * Texas Instruments, <www.ti.com>
 *
 * Franklin S Cooper Jr. <fcooper@ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <boot_fit.h>
#include <common.h>
#include <errno.h>
#include <image.h>
#include <libfdt.h>

int fdt_offset(void *fit)
{
	int fdt_offset, fdt_len;
	int images;

	images = fdt_path_offset(fit, FIT_IMAGES_PATH);
	if (images < 0) {
		debug("%s: Cannot find /images node: %d\n", __func__, images);
		return -1;
	}

	/* Figure out which device tree the board wants to use */
	fdt_len = fit_select_fdt(fit, images, &fdt_offset);

	if (fdt_len < 0)
		return fdt_len;

	return fdt_offset;
}

void *locate_dtb_in_fit(void *fit)
{
	struct image_header *header;
	int size;
	int ret;

	size = fdt_totalsize(fit);
	size = (size + 3) & ~3;

	header = (struct image_header *)fit;

	if (image_get_magic(header) != FDT_MAGIC) {
		debug("No FIT image appended to U-boot\n");
		return NULL;
	}

	ret = fdt_offset(fit);

	if (ret <= 0)
		return NULL;
	else
		return (void *)fit+size+ret;
}
