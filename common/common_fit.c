/*
 * Copyright (C) 2016 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <image.h>
#include <libfdt.h>
#include <spl.h>

ulong fdt_getprop_u32(const void *fdt, int node, const char *prop)
{
	const u32 *cell;
	int len;

	cell = fdt_getprop(fdt, node, prop, &len);
	if (len != sizeof(*cell))
		return -1U;
	return fdt32_to_cpu(*cell);
}

int fit_select_fdt(const void *fdt, int images, int *fdt_offsetp)
{
	const char *name, *fdt_name;
	int conf, node, fdt_node;
	int len;

	*fdt_offsetp = 0;
	conf = fdt_path_offset(fdt, FIT_CONFS_PATH);
	if (conf < 0) {
		debug("%s: Cannot find /configurations node: %d\n", __func__,
		      conf);
		return -EINVAL;
	}
	for (node = fdt_first_subnode(fdt, conf);
	     node >= 0;
	     node = fdt_next_subnode(fdt, node)) {
		name = fdt_getprop(fdt, node, "description", &len);
		if (!name) {
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
			printf("%s: Missing FDT description in DTB\n",
			       __func__);
#endif
			return -EINVAL;
		}
		if (board_fit_config_name_match(name))
			continue;

		debug("Selecting config '%s'", name);
		fdt_name = fdt_getprop(fdt, node, FIT_FDT_PROP, &len);
		if (!fdt_name) {
			debug("%s: Cannot find fdt name property: %d\n",
			      __func__, len);
			return -EINVAL;
		}

		debug(", fdt '%s'\n", fdt_name);
		fdt_node = fdt_subnode_offset(fdt, images, fdt_name);
		if (fdt_node < 0) {
			debug("%s: Cannot find fdt node '%s': %d\n",
			      __func__, fdt_name, fdt_node);
			return -EINVAL;
		}

		*fdt_offsetp = fdt_getprop_u32(fdt, fdt_node, "data-offset");
		len = fdt_getprop_u32(fdt, fdt_node, "data-size");
		debug("FIT: Selected '%s'\n", name);

		return len;
	}

#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
	printf("No matching DT out of these options:\n");
	for (node = fdt_first_subnode(fdt, conf);
	     node >= 0;
	     node = fdt_next_subnode(fdt, node)) {
		name = fdt_getprop(fdt, node, "description", &len);
		printf("   %s\n", name);
	}
#endif

	return -ENOENT;
}
