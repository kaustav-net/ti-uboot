/*
 * DRA76x: DDR ECC test command.
 *
 * (C) Copyright 2017 Texas Instruments Incorporated, <www.ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/emif.h>
#include <asm/cache.h>
#include <asm/omap_common.h>

static void ddr_check_ecc_status(void)
{
	struct emif_reg_struct *emif = (struct emif_reg_struct *)EMIF1_BASE;
	u32 err_1b = readl(&emif->emif_1b_ecc_err_cnt);
	u32 int_status = readl(&emif->emif_irqstatus_raw_sys);
	int ecc_test = 0;
	char *env;

	env = getenv("ecc_test");
	if (env)
		ecc_test = simple_strtol(env, NULL, 0);

	puts("ECC test Status:\n");
	if (int_status & EMIF_INT_WR_ECC_ERR_SYS_MASK)
		puts("\tECC test: DDR ECC write error interrupted\n");

	if (int_status & EMIF_INT_TWOBIT_ECC_ERR_SYS_MASK)
		if (!ecc_test)
			panic("\tECC test: DDR ECC 2-bit error interrupted");

	if (int_status & EMIF_INT_ONEBIT_ECC_ERR_SYS_MASK)
		puts("\tECC test: DDR ECC 1-bit error interrupted\n");

	if (err_1b)
		printf("\tECC test: 1-bit ECC err count: 0x%x\n", err_1b);
}

static int ddr_verify_ecc(u32 addr, u32 ecc_err)
{
	struct emif_reg_struct *emif = (struct emif_reg_struct *)EMIF1_BASE;
	u32 ecc_ctrl = readl(&emif->emif_ecc_ctrl_reg);
	u32 val1, val2, val3;

	/* Disable Caches */
	debug("Disabling D-Cache before ECC test\n");
	dcache_disable();
	invalidate_dcache_all();

	puts("Testing DDR ECC:\n");
	puts("\tECC test: Disabling DDR ECC ...\n");
	writel(0, &emif->emif_ecc_ctrl_reg);

	val1 = readl(addr);
	val2 = val1 ^ ecc_err;
	writel(val2, addr);

	val3 = readl(addr);
	printf("\tECC test: addr 0x%x, read data 0x%x, written data 0x%x, err pattern: 0x%x, read after write data 0x%x\n",
	       addr, val1, val2, ecc_err, val3);

	puts("\tECC test: Enabling DDR ECC ...\n");
	writel(ecc_ctrl, &emif->emif_ecc_ctrl_reg);

	val1 = readl(addr);
	printf("\tECC test: addr 0x%x, read data 0x%x\n", addr, val1);

	ddr_check_ecc_status();

	debug("Enabling D-cache back after ECC test\n");
	enable_caches();

	return 0;
}

static int is_addr_valid(u32 addr)
{
	struct emif_reg_struct *emif = (struct emif_reg_struct *)EMIF1_BASE;
	u32 start_addr, end_addr, range, ecc_ctrl;

	ecc_ctrl = readl(&emif->emif_ecc_ctrl_reg);

	/* Check in ecc address range 1 */
	if (ecc_ctrl & EMIF_ECC_REG_ECC_ADDR_RGN_1_EN_MASK) {
		range = readl(&emif->emif_ecc_address_range_1);
		start_addr = ((range & EMIF_ECC_REG_ECC_START_ADDR_MASK) << 16)
				+ CONFIG_SYS_SDRAM_BASE;
		end_addr = start_addr + (range & EMIF_ECC_REG_ECC_END_ADDR_MASK)
				+ 0xFFFF;
		if ((addr >= start_addr) && (addr <= end_addr))
			/* addr within ecc address range 1 */
			return 1;
	}

	/* Check in ecc address range 2 */
	if (ecc_ctrl & EMIF_ECC_REG_ECC_ADDR_RGN_2_EN_MASK) {
		range = readl(&emif->emif_ecc_address_range_2);
		start_addr = ((range & EMIF_ECC_REG_ECC_START_ADDR_MASK) << 16)
				+ CONFIG_SYS_SDRAM_BASE;
		end_addr = start_addr + (range & EMIF_ECC_REG_ECC_END_ADDR_MASK)
				+ 0xFFFF;
		if ((addr >= start_addr) && (addr <= end_addr))
			/* addr within ecc address range 2 */
			return 1;
	}

	return 0;
}

static int is_ecc_enabled(void)
{
	struct emif_reg_struct *emif = (struct emif_reg_struct *)EMIF1_BASE;
	u32 ecc_ctrl = readl(&emif->emif_ecc_ctrl_reg);

	return (ecc_ctrl & EMIF_ECC_CTRL_REG_ECC_EN_MASK) &&
		(ecc_ctrl & EMIF_ECC_REG_RMW_EN_MASK);
}

static int do_ddr_test(cmd_tbl_t *cmdtp,
		       int flag, int argc, char * const argv[])
{
	u32 addr, ecc_err;

	/* ECC supported only on dra76x silicons */
	if (!is_dra76x()) {
		puts("ECC not supported on this SoC. ECC supported only on DRA76x\n");
		return CMD_RET_FAILURE;
	}

	if ((argc == 4) && (strncmp(argv[1], "ecc_err", 8) == 0)) {
		if (!is_ecc_enabled()) {
			puts("ECC not enabled. Please Enable ECC any try again\n");
			return CMD_RET_FAILURE;
		}

		addr = simple_strtoul(argv[2], NULL, 16);
		ecc_err = simple_strtoul(argv[3], NULL, 16);

		if (!is_addr_valid(addr)) {
			puts("Invalid address. Please enter ECC supported address!\n");
			return CMD_RET_FAILURE;
		}

		ddr_verify_ecc(addr, ecc_err);

		return CMD_RET_SUCCESS;
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(ddr,	4, 1, do_ddr_test,
	   "DDR3 ECC test",
	   "ecc_err <addr in hex> <bit_err in hex> - generate bit errors\n"
	   "	in DDR data at <addr>, the command will read a 32-bit data\n"
	   "	from <addr>, and write (data ^ bit_err) back to <addr>\n"
);
