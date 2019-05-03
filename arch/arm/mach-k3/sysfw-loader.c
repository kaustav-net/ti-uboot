// SPDX-License-Identifier: GPL-2.0+
/*
 * K3: System Firmware Loader
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Andreas Dannenberg <dannenberg@ti.com>
 */

#include <common.h>
#include <spl.h>
#include <dm.h>
#include <dm/uclass-internal.h>
#include <dm/pinctrl.h>
#include <remoteproc.h>
#include <linux/libfdt.h>
#include <linux/soc/ti/ti_sci_protocol.h>
#include <image.h>
#include <malloc.h>
#include <spi_flash.h>

#include <asm/io.h>
#include <asm/spl.h>
#include <asm/sections.h>
#include <asm/armv7_mpu.h>
#include <asm/arch/hardware.h>
#include <asm/arch/sys_proto.h>

/* Name of the FIT image nodes for SYSFW and its config data */
#define SYSFW_FIRMWARE			"sysfw.bin"
#define SYSFW_CFG_BOARD			"board-cfg.bin"
#define SYSFW_CFG_PM			"pm-cfg.bin"
#define SYSFW_CFG_RM			"rm-cfg.bin"
#define SYSFW_CFG_SEC			"sec-cfg.bin"

#ifdef CONFIG_SPL_BUILD
static int fit_get_data_by_name(const void *fit, int images, const char *name,
				const void **addr, size_t *size)
{
	int node_offset;

	node_offset = fdt_subnode_offset(fit, images, name);
	if (node_offset < 0)
		return -ENOENT;

	return fit_image_get_data(fit, node_offset, addr, size);
}

static void k3_sysfw_load_using_fit(void *fit, struct ti_sci_handle **ti_sci)
{
	int images;
	const void *sysfw_addr;
	size_t sysfw_size;
	int ret;

	/* Find the node holding the images information */
	images = fdt_path_offset(fit, FIT_IMAGES_PATH);
	if (images < 0) {
		printf("Cannot find /images node (%d)\n", images);
		hang();
	}

	/* Extract System Firmware (SYSFW) image from FIT */
	ret = fit_get_data_by_name(fit, images, SYSFW_FIRMWARE,
				   &sysfw_addr, &sysfw_size);
	if (ret < 0) {
		printf("Error accessing %s node in FIT (%d)\n", SYSFW_FIRMWARE,
		       ret);
		hang();
	}

	/*
	 * Start up system controller firmware
	 *
	 * It is assumed that remoteproc device 0 is the corresponding
	 * system-controller that runs SYSFW. Make sure DT reflects the same.
	 */
	ret = rproc_dev_init(0);
	if (ret) {
		pr_err("rproc failed to be initialized (%d)\n", ret);
		hang();
	}

	ret = rproc_load(0, (ulong)sysfw_addr, (ulong)sysfw_size);
	if (ret) {
		pr_err("Firmware failed to start on rproc (%d)\n", ret);
		hang();
	}

	ret = rproc_start(0);
	if (ret) {
		pr_err("Firmware init failed on rproc (%d)\n", ret);
		hang();
	}

	/* Establish handle for easier access */
	*ti_sci = get_ti_sci_handle();
}

static void k3_sysfw_configure_using_fit(void *fit,
					 struct ti_sci_handle *ti_sci,
					 void (*config_pm_done_callback)(void))
{
	struct ti_sci_board_ops *board_ops = &ti_sci->ops.board_ops;
	int images;
	const void *cfg_fragment_addr;
	size_t cfg_fragment_size;
	int ret;

	/* Find the node holding the images information */
	images = fdt_path_offset(fit, FIT_IMAGES_PATH);
	if (images < 0) {
		printf("Cannot find /images node (%d)\n", images);
		hang();
	}

	/* Extract board configuration from FIT */
	ret = fit_get_data_by_name(fit, images, SYSFW_CFG_BOARD,
				   &cfg_fragment_addr, &cfg_fragment_size);
	if (ret < 0) {
		printf("Error accessing %s node in FIT (%d)\n", SYSFW_CFG_BOARD,
		       ret);
		hang();
	}

	/* Apply board configuration to SYSFW */
	ret = board_ops->board_config(ti_sci,
				      (u64)(u32)cfg_fragment_addr,
				      (u32)cfg_fragment_size);
	if (ret) {
		pr_err("Failed to set board configuration (%d)\n", ret);
		hang();
	}

	/* Extract power/clock (PM) specific configuration from FIT */
	ret = fit_get_data_by_name(fit, images, SYSFW_CFG_PM,
				   &cfg_fragment_addr, &cfg_fragment_size);
	if (ret < 0) {
		printf("Error accessing %s node in FIT (%d)\n", SYSFW_CFG_PM,
		       ret);
		hang();
	}

	/* Apply power/clock (PM) specific configuration to SYSFW */
	ret = board_ops->board_config_pm(ti_sci,
					 (u64)(u32)cfg_fragment_addr,
					 (u32)cfg_fragment_size);
	if (ret) {
		pr_err("Failed to set board PM configuration (%d)\n", ret);
		hang();
	}

	/* Extract resource management (RM) specific configuration from FIT */
	ret = fit_get_data_by_name(fit, images, SYSFW_CFG_RM,
				   &cfg_fragment_addr, &cfg_fragment_size);
	if (ret < 0) {
		printf("Error accessing %s node in FIT (%d)\n", SYSFW_CFG_RM,
		       ret);
		hang();
	}

	/* Apply resource management (RM) configuration to SYSFW */
	ret = board_ops->board_config_rm(ti_sci,
					 (u64)(u32)cfg_fragment_addr,
					 (u32)cfg_fragment_size);
	if (ret) {
		pr_err("Failed to set board RM configuration (%d)\n", ret);
		hang();
	}

	/* Extract security specific configuration from FIT */
	ret = fit_get_data_by_name(fit, images, SYSFW_CFG_SEC,
				   &cfg_fragment_addr, &cfg_fragment_size);
	if (ret < 0) {
		printf("Error accessing %s node in FIT (%d)\n", SYSFW_CFG_SEC,
		       ret);
		hang();
	}

	/* Apply security configuration to SYSFW */
	ret = board_ops->board_config_security(ti_sci,
					       (u64)(u32)cfg_fragment_addr,
					       (u32)cfg_fragment_size);
	if (ret) {
		pr_err("Failed to set board security configuration (%d)\n",
		       ret);
		hang();
	}

	/*
	 * Now that all clocks and PM aspects are setup, invoke a user-
	 * provided callback function. Usually this callback would be used
	 * to setup or re-configure the U-Boot console UART.
	 */
	if (config_pm_done_callback)
		config_pm_done_callback();
}

#if CONFIG_IS_ENABLED(SPI_LOAD)
static void *get_sysfw_spi_addr(void)
{
	struct udevice *dev;
	fdt_addr_t addr;
	int ret;

	ret = uclass_find_device_by_seq(UCLASS_SPI, CONFIG_SF_DEFAULT_BUS,
					true, &dev);
	if (ret)
		return NULL;

	addr = dev_read_addr_index(dev, 1);
	if (addr == FDT_ADDR_T_NONE)
		return NULL;

	return (void *)(addr + CONFIG_K3_SYSFW_IMAGE_SPI_OFFS);
}
#endif

void k3_sysfw_loader(void (*config_pm_done_callback)(void))
{
	void *addr;
	struct spl_image_info spl_image = { 0 };
	struct spl_boot_device bootdev = { 0 };
	struct ti_sci_handle *ti_sci;
	int ret = 0;

	addr = memalign(ARCH_DMA_MINALIGN, CONFIG_K3_SYSFW_IMAGE_SIZE_MAX);
	if (!addr) {
		printf("Error allocating %u bytes of memory for SYSFW image\n",
		       CONFIG_K3_SYSFW_IMAGE_SIZE_MAX);
		hang();
	}

	debug("%s: allocated %u bytes at 0x%p\n", __func__,
	      CONFIG_K3_SYSFW_IMAGE_SIZE_MAX, addr);

	bootdev.boot_device = spl_boot_device();

	/* Load combined System Controller firmware and config data image */
	switch (bootdev.boot_device) {
#if CONFIG_IS_ENABLED(MMC_SUPPORT)
	case BOOT_DEVICE_MMC1:
	case BOOT_DEVICE_MMC2:
	case BOOT_DEVICE_MMC2_2:
		ret = spl_mmc_load(&spl_image, &bootdev,
#ifdef CONFIG_K3_SYSFW_IMAGE_NAME
				   CONFIG_K3_SYSFW_IMAGE_NAME,
#else
				   NULL,
#endif
#ifdef CONFIG_K3_SYSFW_IMAGE_MMCSD_RAW_MODE_PART
				   CONFIG_K3_SYSFW_IMAGE_MMCSD_RAW_MODE_PART,
#else
				   0,
#endif
#ifdef CONFIG_K3_SYSFW_IMAGE_MMCSD_RAW_MODE_SECT
				   CONFIG_K3_SYSFW_IMAGE_MMCSD_RAW_MODE_SECT,
#else
				   0,
#endif
				   addr);
		break;
#endif
#if CONFIG_IS_ENABLED(SPI_LOAD)
	case BOOT_DEVICE_SPI:
		addr = get_sysfw_spi_addr();
		if (!addr)
			ret = -ENODEV;
		break;
#endif
#if CONFIG_IS_ENABLED(YMODEM_SUPPORT)
	case BOOT_DEVICE_UART:
		ret = spl_ymodem_load(&spl_image, &bootdev, addr);
		break;
#endif
	default:
		printf("Loading SYSFW image from device %u not supported!\n",
		       bootdev.boot_device);
		hang();
	}

	if (ret) {
		printf("Error %d occurred during loading SYSFW image!\n", ret);
		hang();
	}

	/* Ensure the SYSFW image is in FIT format */
	if (image_get_magic((const image_header_t *)addr) != FDT_MAGIC) {
		printf("SYSFW image not in FIT format!\n");
		hang();
	}

	/* Extract and start SYSFW */
	k3_sysfw_load_using_fit(addr, &ti_sci);

	/* Parse and apply the different SYSFW configuration fragments */
	k3_sysfw_configure_using_fit(addr, ti_sci, config_pm_done_callback);

	/* Output System Firmware version info */
	printf("SYSFW ABI: %d.%d (firmware rev 0x%04x '%.*s')\n",
	       ti_sci->version.abi_major, ti_sci->version.abi_minor,
	       ti_sci->version.firmware_revision,
	       sizeof(ti_sci->version.firmware_description),
	       ti_sci->version.firmware_description);
}
#endif
