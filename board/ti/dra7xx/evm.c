/*
 * (C) Copyright 2013
 * Texas Instruments Incorporated, <www.ti.com>
 *
 * Lokesh Vutla <lokeshvutla@ti.com>
 *
 * Based on previous work by:
 * Aneesh V       <aneesh@ti.com>
 * Steve Sakoman  <steve@sakoman.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <palmas.h>
#include <sata.h>
#include <asm/gpio.h>
#include <asm/arch/gpio.h>
#include <usb.h>
#include <linux/usb/gadget.h>
#include <asm/arch/dra7xx_iodelay.h>
#include <asm/arch-omap5/mux_dra7xx.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/mmc_host_def.h>
#include <asm/arch/sata.h>
#include <environment.h>
#include <dwc3-uboot.h>
#include <dwc3-omap-uboot.h>
#include <ti-usb-phy-uboot.h>
#include <pcf8575.h>

#include "mux_data.h"

#ifdef CONFIG_DRIVER_TI_CPSW
#include <cpsw.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

/* GPIO 7_11 */
#define GPIO_DDR_VTT_EN 203

/* pcf chip address enet_mux_s0 */
#define PCF_ENET_MUX_ADDR	0x21
#define PCF_SEL_ENET_MUX_S0	4

const struct omap_sysinfo sysinfo = {
	"Board: DRA7xx\n"
};

/**
 * @brief board_init
 *
 * @return 0
 */
int board_init(void)
{
	gpmc_init();
	gd->bd->bi_boot_params = (0x80000000 + 0x100); /* boot param addr */

	return 0;
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	if (omap_revision() == DRA722_ES1_0)
		setenv("board_name", "dra72x");
	else
		setenv("board_name", "dra7xx");
#endif
	init_sata(0);
	return 0;
}

void set_muxconf_regs_essential(void)
{
	do_set_mux32((*ctrl)->control_padconf_core_base,
		     early_padconf, ARRAY_SIZE(early_padconf));
}

#ifdef CONFIG_IODELAY_RECALIBRATION
void recalibrate_iodelay(void)
{
	if (is_dra72x()) {
		__recalibrate_iodelay(core_padconf_array_essential,
				      ARRAY_SIZE(core_padconf_array_essential),
				      iodelay_cfg_array,
				      ARRAY_SIZE(iodelay_cfg_array));
	} else {
		__recalibrate_iodelay(dra74x_core_padconf_array,
				      ARRAY_SIZE(dra74x_core_padconf_array),
				      dra742_iodelay_cfg_array,
				      ARRAY_SIZE(dra742_iodelay_cfg_array));
	}
}
#endif

#if !defined(CONFIG_SPL_BUILD) && defined(CONFIG_GENERIC_MMC)
int board_mmc_init(bd_t *bis)
{
	omap_mmc_init(0, 0, 0, -1, -1);
	omap_mmc_init(1, 0, 0, -1, -1);
	return 0;
}
#endif

#ifdef CONFIG_OMAP_HSMMC
#ifdef CONFIG_IODELAY_RECALIBRATION

#define MUX_MODE0 M0
#define MUX_MODE1 M1
#define A_DELAY(x) (x)
#define G_DELAY(x) (x)

struct pad_conf_entry mmc2_pins_default_hs[] = {
	{0x9c, (PIN_INPUT_PULLUP | MUX_MODE1)	/* gpmc_a23.mmc2_clk */},
	{0xb0, (PIN_INPUT_PULLUP | MUX_MODE1)	/* gpmc_cs1.mmc2_cmd */},
	{0xa0, (PIN_INPUT_PULLUP | MUX_MODE1)	/* gpmc_a24.mmc2_dat0 */},
	{0xa4, (PIN_INPUT_PULLUP | MUX_MODE1)	/* gpmc_a25.mmc2_dat1 */},
	{0xa8, (PIN_INPUT_PULLUP | MUX_MODE1)	/* gpmc_a26.mmc2_dat2 */},
	{0xac, (PIN_INPUT_PULLUP | MUX_MODE1)	/* gpmc_a27.mmc2_dat3 */},
	{0x8c, (PIN_INPUT_PULLUP | MUX_MODE1)	/* gpmc_a19.mmc2_dat4 */},
	{0x90, (PIN_INPUT_PULLUP | MUX_MODE1)	/* gpmc_a20.mmc2_dat5 */},
	{0x94, (PIN_INPUT_PULLUP | MUX_MODE1)	/* gpmc_a21.mmc2_dat6 */},
	{0x98, (PIN_INPUT_PULLUP | MUX_MODE1)	/* gpmc_a22.mmc2_dat7 */},
};

struct pad_conf_entry mmc2_pins_ddr_hs200_1_8v[] = {
	{0x9c, (PIN_INPUT_PULLUP | MANUAL_MODE | MUX_MODE1) /* gpmc_a23.mmc2_clk */},
	{0xb0, (PIN_INPUT_PULLUP | MANUAL_MODE | MUX_MODE1) /* gpmc_cs1.mmc2_cmd */},
	{0xa0, (PIN_INPUT_PULLUP | MANUAL_MODE | MUX_MODE1) /* gpmc_a24.mmc2_dat0 */},
	{0xa4, (PIN_INPUT_PULLUP | MANUAL_MODE | MUX_MODE1) /* gpmc_a25.mmc2_dat1 */},
	{0xa8, (PIN_INPUT_PULLUP | MANUAL_MODE | MUX_MODE1) /* gpmc_a26.mmc2_dat2 */},
	{0xac, (PIN_INPUT_PULLUP | MANUAL_MODE | MUX_MODE1) /* gpmc_a27.mmc2_dat3 */},
	{0x8c, (PIN_INPUT_PULLUP | MANUAL_MODE | MUX_MODE1) /* gpmc_a19.mmc2_dat4 */},
	{0x90, (PIN_INPUT_PULLUP | MANUAL_MODE | MUX_MODE1) /* gpmc_a20.mmc2_dat5 */},
	{0x94, (PIN_INPUT_PULLUP | MANUAL_MODE | MUX_MODE1) /* gpmc_a21.mmc2_dat6 */},
	{0x98, (PIN_INPUT_PULLUP | MANUAL_MODE | MUX_MODE1) /* gpmc_a22.mmc2_dat7 */},
	};

struct iodelay_cfg_entry mmc2_iodelay_hs200_1_8v_rev11_conf[] = {
	{0x190, A_DELAY(621), G_DELAY(600)	/* CFG_GPMC_A19_OEN */},
	{0x194, A_DELAY(300), G_DELAY(0)	/* CFG_GPMC_A19_OUT */},
	{0x1a8, A_DELAY(739), G_DELAY(600)	/* CFG_GPMC_A20_OEN */},
	{0x1ac, A_DELAY(240), G_DELAY(0)	/* CFG_GPMC_A20_OUT */},
	{0x1b4, A_DELAY(812), G_DELAY(600)	/* CFG_GPMC_A21_OEN */},
	{0x1b8, A_DELAY(240), G_DELAY(0)	/* CFG_GPMC_A21_OUT */},
	{0x1c0, A_DELAY(954), G_DELAY(600)	/* CFG_GPMC_A22_OEN */},
	{0x1c4, A_DELAY(60), G_DELAY(0)		/* CFG_GPMC_A22_OUT */},
	{0x1d0, A_DELAY(1340), G_DELAY(420)	/* CFG_GPMC_A23_OUT */},
	{0x1d8, A_DELAY(935), G_DELAY(600)	/* CFG_GPMC_A24_OEN */},
	{0x1dc, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A24_OUT */},
	{0x1e4, A_DELAY(525), G_DELAY(600)	/* CFG_GPMC_A25_OEN */},
	{0x1e8, A_DELAY(120), G_DELAY(0)	/* CFG_GPMC_A25_OUT */},
	{0x1f0, A_DELAY(767), G_DELAY(600)	/* CFG_GPMC_A26_OEN */},
	{0x1f4, A_DELAY(225), G_DELAY(0)	/* CFG_GPMC_A26_OUT */},
	{0x1fc, A_DELAY(565), G_DELAY(600)	/* CFG_GPMC_A27_OEN */},
	{0x200, A_DELAY(60), G_DELAY(0)		/* CFG_GPMC_A27_OUT */},
	{0x364, A_DELAY(969), G_DELAY(600)	/* CFG_GPMC_CS1_OEN */},
	{0x368, A_DELAY(180), G_DELAY(0)	/* CFG_GPMC_CS1_OUT */},
};

struct iodelay_cfg_entry mmc2_iodelay_hs200_1_8v_rev20_conf[] = {
	{0x190, A_DELAY(274), G_DELAY(0)  /* CFG_GPMC_A19_OEN */},
	{0x194, A_DELAY(162), G_DELAY(0)  /* CFG_GPMC_A19_OUT */},
	{0x1a8, A_DELAY(401), G_DELAY(0)  /* CFG_GPMC_A20_OEN */},
	{0x1ac, A_DELAY(73), G_DELAY(0)   /* CFG_GPMC_A20_OUT */},
	{0x1b4, A_DELAY(465), G_DELAY(0)  /* CFG_GPMC_A21_OEN */},
	{0x1b8, A_DELAY(115), G_DELAY(0)  /* CFG_GPMC_A21_OUT */},
	{0x1c0, A_DELAY(633), G_DELAY(0)  /* CFG_GPMC_A22_OEN */},
	{0x1c4, A_DELAY(47), G_DELAY(0)   /* CFG_GPMC_A22_OUT */},
	{0x1d0, A_DELAY(935), G_DELAY(280)/* CFG_GPMC_A23_OUT */},
	{0x1d8, A_DELAY(621), G_DELAY(0)  /* CFG_GPMC_A24_OEN */},
	{0x1dc, A_DELAY(0), G_DELAY(0)    /* CFG_GPMC_A24_OUT */},
	{0x1e4, A_DELAY(183), G_DELAY(0)  /* CFG_GPMC_A25_OEN */},
	{0x1e8, A_DELAY(0), G_DELAY(0)    /* CFG_GPMC_A25_OUT */},
	{0x1f0, A_DELAY(467), G_DELAY(0)  /* CFG_GPMC_A26_OEN */},
	{0x1f4, A_DELAY(0), G_DELAY(0)    /* CFG_GPMC_A26_OUT */},
	{0x1fc, A_DELAY(262), G_DELAY(0)  /* CFG_GPMC_A27_OEN */},
	{0x200, A_DELAY(46), G_DELAY(0)   /* CFG_GPMC_A27_OUT */},
	{0x364, A_DELAY(684), G_DELAY(0)  /* CFG_GPMC_CS1_OEN */},
	{0x368, A_DELAY(76), G_DELAY(0)   /* CFG_GPMC_CS1_OUT */},
};

struct iodelay_cfg_entry mmc2_iodelay_ddr_1_8v_rev11_conf[] = {
	{0x18c, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A19_IN */},
	{0x1a4, A_DELAY(274), G_DELAY(240)	/* CFG_GPMC_A20_IN */},
	{0x1b0, A_DELAY(0), G_DELAY(60)		/* CFG_GPMC_A21_IN */},
	{0x1bc, A_DELAY(0), G_DELAY(60)		/* CFG_GPMC_A22_IN */},
	{0x1c8, A_DELAY(514), G_DELAY(360)	/* CFG_GPMC_A23_IN */},
	{0x1d4, A_DELAY(187), G_DELAY(120)	/* CFG_GPMC_A24_IN */},
	{0x1e0, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A25_IN */},
	{0x1ec, A_DELAY(0), G_DELAY(60)		/* CFG_GPMC_A26_IN */},
	{0x1f8, A_DELAY(121), G_DELAY(60)	/* CFG_GPMC_A27_IN */},
	{0x360, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_CS1_IN */},
	{0x190, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A19_OEN */},
	{0x194, A_DELAY(174), G_DELAY(0)	/* CFG_GPMC_A19_OUT */},
	{0x1a8, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A20_OEN */},
	{0x1ac, A_DELAY(168), G_DELAY(0)	/* CFG_GPMC_A20_OUT */},
	{0x1b4, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A21_OEN */},
	{0x1b8, A_DELAY(136), G_DELAY(0)	/* CFG_GPMC_A21_OUT */},
	{0x1c0, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A22_OEN */},
	{0x1c4, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A22_OUT */},
	{0x1d0, A_DELAY(879), G_DELAY(0)	/* CFG_GPMC_A23_OUT */},
	{0x1d8, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A24_OEN */},
	{0x1dc, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A24_OUT */},
	{0x1e4, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A25_OEN */},
	{0x1e8, A_DELAY(34), G_DELAY(0)		/* CFG_GPMC_A25_OUT */},
	{0x1f0, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A26_OEN */},
	{0x1f4, A_DELAY(120), G_DELAY(0)	/* CFG_GPMC_A26_OUT */},
	{0x1fc, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A27_OEN */},
	{0x200, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A27_OUT */},
	{0x364, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_CS1_OEN */},
	{0x368, A_DELAY(11), G_DELAY(0)		/* CFG_GPMC_CS1_OUT */},
};

struct iodelay_cfg_entry mmc2_iodelay_ddr_1_8v_rev20_conf[] = {
	{0x18c, A_DELAY(270), G_DELAY(0)	/* CFG_GPMC_A19_IN */},
	{0x1a4, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A20_IN */},
	{0x1b0, A_DELAY(170), G_DELAY(0)	/* CFG_GPMC_A21_IN */},
	{0x1bc, A_DELAY(758), G_DELAY(0)	/* CFG_GPMC_A22_IN */},
	{0x1c8, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A23_IN */},
	{0x1d4, A_DELAY(81), G_DELAY(0)		/* CFG_GPMC_A24_IN */},
	{0x1e0, A_DELAY(286), G_DELAY(0)	/* CFG_GPMC_A25_IN */},
	{0x1ec, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A26_IN */},
	{0x1f8, A_DELAY(123), G_DELAY(0)	/* CFG_GPMC_A27_IN */},
	{0x360, A_DELAY(346), G_DELAY(0)	/* CFG_GPMC_CS1_IN */},
	{0x190, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A19_OEN */},
	{0x194, A_DELAY(55), G_DELAY(0)		/* CFG_GPMC_A19_OUT */},
	{0x1a8, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A20_OEN */},
	{0x1ac, A_DELAY(422), G_DELAY(0)	/* CFG_GPMC_A20_OUT */},
	{0x1b4, A_DELAY(642), G_DELAY(0)	/* CFG_GPMC_A21_OEN */},
	{0x1b8, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A21_OUT */},
	{0x1c0, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A22_OEN */},
	{0x1c4, A_DELAY(128), G_DELAY(0)	/* CFG_GPMC_A22_OUT */},
	{0x1d0, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A23_OUT */},
	{0x1d8, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A24_OEN */},
	{0x1dc, A_DELAY(395), G_DELAY(0)	/* CFG_GPMC_A24_OUT */},
	{0x1e4, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A25_OEN */},
	{0x1e8, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A25_OUT */},
	{0x1f0, A_DELAY(623), G_DELAY(0)	/* CFG_GPMC_A26_OEN */},
	{0x1f4, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A26_OUT */},
	{0x1fc, A_DELAY(54), G_DELAY(0)		/* CFG_GPMC_A27_OEN */},
	{0x200, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_A27_OUT */},
	{0x364, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_CS1_OEN */},
	{0x368, A_DELAY(0), G_DELAY(0)		/* CFG_GPMC_CS1_OUT */},
};

struct iodelay_cfg_entry mmc2_iodelay_hs200_1_8v_dra72_conf[] = {
	{0x194, A_DELAY(150) , G_DELAY(95)	/* CFG_GPMC_A19_OUT */},
	{0x1AC, A_DELAY(250) , G_DELAY(0)	/* CFG_GPMC_A20_OUT */},
	{0x1B8, A_DELAY(125) , G_DELAY(0)	/* CFG_GPMC_A21_OUT */},
	{0x1C4, A_DELAY(100) , G_DELAY(0)	/* CFG_GPMC_A22_OUT */},
	{0x1D0, A_DELAY(870) , G_DELAY(415)	/* CFG_GPMC_A23_OUT */},
	{0x1DC, A_DELAY(30)  , G_DELAY(0)	/* CFG_GPMC_A24_OUT */},
	{0x1E8, A_DELAY(200) , G_DELAY(0)	/* CFG_GPMC_A25_OUT */},
	{0x1F4, A_DELAY(200) , G_DELAY(0)	/* CFG_GPMC_A26_OUT */},
	{0x200, A_DELAY(0)   , G_DELAY(0)	/* CFG_GPMC_A27_OUT */},
	{0x368, A_DELAY(240) , G_DELAY(0)	/* CFG_GPMC_CS1_OUT */},
	{0x190, A_DELAY(695) , G_DELAY(0)	/* CFG_GPMC_A19_OEN */},
	{0x1A8, A_DELAY(924) , G_DELAY(0)	/* CFG_GPMC_A20_OEN */},
	{0x1B4, A_DELAY(719) , G_DELAY(0)	/* CFG_GPMC_A21_OEN */},
	{0x1C0, A_DELAY(824) , G_DELAY(0)	/* CFG_GPMC_A22_OEN */},
	{0x1D8, A_DELAY(877) , G_DELAY(0)	/* CFG_GPMC_A24_OEN */},
	{0x1E4, A_DELAY(446) , G_DELAY(0)	/* CFG_GPMC_A25_OEN */},
	{0x1F0, A_DELAY(847) , G_DELAY(0)	/* CFG_GPMC_A26_OEN */},
	{0x1FC, A_DELAY(586) , G_DELAY(0)	/* CFG_GPMC_A27_OEN */},
	{0x364, A_DELAY(1039) , G_DELAY(0)	/* CFG_GPMC_CS1_OEN */},
};

struct pad_conf_entry hsmmc1_default_padconf[] = {
	{0x354, (PIN_INPUT_PULLUP | MUX_MODE0)	/* mmc1_clk.clk */},
	{0x358, (PIN_INPUT_PULLUP | MUX_MODE0)	/* mmc1_cmd.cmd */},
	{0x35c, (PIN_INPUT_PULLUP | MUX_MODE0)	/* mmc1_dat0.dat0 */},
	{0x360, (PIN_INPUT_PULLUP | MUX_MODE0)	/* mmc1_dat1.dat1 */},
	{0x364, (PIN_INPUT_PULLUP | MUX_MODE0)	/* mmc1_dat2.dat2 */},
	{0x368, (PIN_INPUT_PULLUP | MUX_MODE0)	/* mmc1_dat3.dat3 */},
};

#define dimof(t) (sizeof(t) / sizeof(t[0]))
static struct omap_hsmmc_pinctrl_state hsmmc1_default = {
	.padconf = hsmmc1_default_padconf,
	.npads = dimof(hsmmc1_default_padconf),
	.iodelay = NULL,
	.niodelays = 0,
};

static struct omap_hsmmc_pinctrl_state hsmmc2_default_hs = {
	.padconf = mmc2_pins_default_hs,
	.npads = dimof(mmc2_pins_default_hs),
	.iodelay = NULL,
	.niodelays = 0,
};

static struct omap_hsmmc_pinctrl_state hsmmc2_ddr_1v8_rev11 = {
	.padconf = mmc2_pins_ddr_hs200_1_8v,
	.npads = dimof(mmc2_pins_ddr_hs200_1_8v),
	.iodelay = mmc2_iodelay_ddr_1_8v_rev20_conf,
	.niodelays = dimof(mmc2_iodelay_ddr_1_8v_rev20_conf),
};

static struct omap_hsmmc_pinctrl_state hsmmc2_ddr_1v8_rev20 = {
	.padconf = mmc2_pins_ddr_hs200_1_8v,
	.npads = dimof(mmc2_pins_ddr_hs200_1_8v),
	.iodelay = mmc2_iodelay_ddr_1_8v_rev20_conf,
	.niodelays = dimof(mmc2_iodelay_ddr_1_8v_rev20_conf),
};

static struct omap_hsmmc_pinctrl_state hsmmc2_hs200_1v8_rev11 = {
	.padconf = mmc2_pins_ddr_hs200_1_8v,
	.npads = dimof(mmc2_pins_ddr_hs200_1_8v),
	.iodelay = mmc2_iodelay_hs200_1_8v_rev11_conf,
	.niodelays = dimof(mmc2_iodelay_hs200_1_8v_rev11_conf),
};

static struct omap_hsmmc_pinctrl_state hsmmc2_hs200_1v8_rev20 = {
	.padconf = mmc2_pins_ddr_hs200_1_8v,
	.npads = dimof(mmc2_pins_ddr_hs200_1_8v),
	.iodelay = mmc2_iodelay_hs200_1_8v_rev20_conf,
	.niodelays = dimof(mmc2_iodelay_hs200_1_8v_rev20_conf),
};

static struct omap_hsmmc_pinctrl_state hsmmc2_hs200_1v8_dra72 = {
	.padconf = mmc2_pins_ddr_hs200_1_8v,
	.npads = dimof(mmc2_pins_ddr_hs200_1_8v),
	.iodelay = mmc2_iodelay_hs200_1_8v_dra72_conf,
	.niodelays = dimof(mmc2_iodelay_hs200_1_8v_dra72_conf),
};

struct pinctrl_desc {
	const char *name;
	struct omap_hsmmc_pinctrl_state *pinctrl;
};

static struct pinctrl_desc pinctrl_descs_hsmmc1[] = {
	{"default", &hsmmc1_default},
	{"hs", &hsmmc1_default},
	{"ddr_1_8v", &hsmmc1_default},
	{"hs200_1_8v", &hsmmc1_default},
	{NULL}
};

static struct pinctrl_desc pinctrl_descs_hsmmc2_rev20[] = {
	{"default", &hsmmc2_default_hs},
	{"hs", &hsmmc2_default_hs},
	{"ddr_1_8v", &hsmmc2_ddr_1v8_rev20},
	{"hs200_1_8v", &hsmmc2_hs200_1v8_rev20},
};

static struct pinctrl_desc pinctrl_descs_hsmmc2_rev11[] = {
	{"default", &hsmmc2_default_hs},
	{"hs", &hsmmc2_default_hs},
	{"ddr_1_8v", &hsmmc2_ddr_1v8_rev11},
	{"hs200_1_8v", &hsmmc2_hs200_1v8_rev11},
};

static struct pinctrl_desc pinctrl_descs_hsmmc2_dra72x[] = {
	{"default", &hsmmc2_default_hs},
	{"hs200_1_8v", &hsmmc2_hs200_1v8_dra72},
};

struct omap_hsmmc_pinctrl_state *platform_fixup_get_pinctrl_by_mode
				  (unsigned int dev_index, const char *mode)
{
	struct pinctrl_desc *p;

	switch (dev_index) {
	case 0:
		p = pinctrl_descs_hsmmc1;
		break;
	case 1:
		if ((omap_revision() == DRA752_ES1_0) ||
		    (omap_revision() == DRA752_ES1_1))
			p = pinctrl_descs_hsmmc2_rev11;
		else if (is_dra7xx())
			p = pinctrl_descs_hsmmc2_rev20;
		else if (is_dra72x())
			p = pinctrl_descs_hsmmc2_dra72x;
		else
			return NULL;
		break;
	default:
		return NULL;
	}

	while (p->name) {
		if (strcmp(mode, p->name) == 0)
			return p->pinctrl;
		p++;
	}
	return NULL;
}
#endif

int platform_fixup_disable_uhs_mode(void)
{
	return omap_revision() == DRA752_ES1_1;
}
#endif

#ifdef CONFIG_USB_DWC3
static struct dwc3_device usb_otg_ss1 = {
	.maximum_speed = USB_SPEED_SUPER,
	.base = DRA7_USB_OTG_SS1_BASE,
	.needs_fifo_resize = false,
	.index = 0,
};

static struct dwc3_omap_device usb_otg_ss1_glue = {
	.base = (void *)DRA7_USB_OTG_SS1_GLUE_BASE,
	.utmi_mode = DWC3_OMAP_UTMI_MODE_SW,
	.index = 0,
};

static struct ti_usb_phy_device usb_phy1_device = {
	.pll_ctrl_base = (void *)DRA7_USB3_PHY1_PLL_CTRL,
	.usb2_phy_power = (void *)DRA7_USB2_PHY1_POWER,
	.usb3_phy_power = (void *)DRA7_USB3_PHY1_POWER,
	.index = 0,
};

static struct dwc3_device usb_otg_ss2 = {
	.maximum_speed = USB_SPEED_SUPER,
	.base = DRA7_USB_OTG_SS2_BASE,
	.needs_fifo_resize = false,
	.index = 1,
};

static struct dwc3_omap_device usb_otg_ss2_glue = {
	.base = (void *)DRA7_USB_OTG_SS2_GLUE_BASE,
	.utmi_mode = DWC3_OMAP_UTMI_MODE_SW,
	.index = 1,
};

static struct ti_usb_phy_device usb_phy2_device = {
	.usb2_phy_power = (void *)DRA7_USB2_PHY2_POWER,
	.index = 1,
};

int board_usb_init(int index, enum usb_init_type init)
{
	enable_usb_clocks(index);
	switch (index) {
	case 0:
		if (init == USB_INIT_DEVICE) {
			usb_otg_ss1.dr_mode = USB_DR_MODE_PERIPHERAL;
			usb_otg_ss1_glue.vbus_id_status = OMAP_DWC3_VBUS_VALID;
		} else {
			usb_otg_ss1.dr_mode = USB_DR_MODE_HOST;
			usb_otg_ss1_glue.vbus_id_status = OMAP_DWC3_ID_GROUND;
		}

		ti_usb_phy_uboot_init(&usb_phy1_device);
		dwc3_omap_uboot_init(&usb_otg_ss1_glue);
		dwc3_uboot_init(&usb_otg_ss1);
		break;
	case 1:
		if (init == USB_INIT_DEVICE) {
			usb_otg_ss2.dr_mode = USB_DR_MODE_PERIPHERAL;
			usb_otg_ss2_glue.vbus_id_status = OMAP_DWC3_VBUS_VALID;
		} else {
			usb_otg_ss2.dr_mode = USB_DR_MODE_HOST;
			usb_otg_ss2_glue.vbus_id_status = OMAP_DWC3_ID_GROUND;
		}

		ti_usb_phy_uboot_init(&usb_phy2_device);
		dwc3_omap_uboot_init(&usb_otg_ss2_glue);
		dwc3_uboot_init(&usb_otg_ss2);
		break;
	default:
		printf("Invalid Controller Index\n");
	}

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	switch (index) {
	case 0:
	case 1:
		ti_usb_phy_uboot_exit(index);
		dwc3_uboot_exit(index);
		dwc3_omap_uboot_exit(index);
		break;
	default:
		printf("Invalid Controller Index\n");
	}
	disable_usb_clocks(index);
	return 0;
}

int usb_gadget_handle_interrupts(int index)
{
	u32 status;

	status = dwc3_omap_uboot_interrupt_status(index);
	if (status)
		dwc3_uboot_handle_interrupt(index);

	return 0;
}
#endif

#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_OS_BOOT)
int spl_start_uboot(void)
{
	/* break into full u-boot on 'c' */
	if (serial_tstc() && serial_getc() == 'c')
		return 1;

#ifdef CONFIG_SPL_ENV_SUPPORT
	env_init();
	env_relocate_spec();
	if (getenv_yesno("boot_os") != 1)
		return 1;
#endif

	return 0;
}
#endif

#ifdef CONFIG_DRIVER_TI_CPSW

/* Delay value to add to calibrated value */
#define RGMII0_TXCTL_DLY_VAL		((0x3 << 5) + 0x8)
#define RGMII0_TXD0_DLY_VAL		((0x3 << 5) + 0x8)
#define RGMII0_TXD1_DLY_VAL		((0x3 << 5) + 0x2)
#define RGMII0_TXD2_DLY_VAL		((0x4 << 5) + 0x0)
#define RGMII0_TXD3_DLY_VAL		((0x4 << 5) + 0x0)
#define VIN2A_D13_DLY_VAL		((0x3 << 5) + 0x8)
#define VIN2A_D17_DLY_VAL		((0x3 << 5) + 0x8)
#define VIN2A_D16_DLY_VAL		((0x3 << 5) + 0x2)
#define VIN2A_D15_DLY_VAL		((0x4 << 5) + 0x0)
#define VIN2A_D14_DLY_VAL		((0x4 << 5) + 0x0)

extern u32 *const omap_si_rev;

static void cpsw_control(int enabled)
{
	/* VTP can be added here */

	return;
}

static struct cpsw_slave_data cpsw_slaves[] = {
	{
		.slave_reg_ofs	= 0x208,
		.sliver_reg_ofs	= 0xd80,
		.phy_addr	= 2,
	},
	{
		.slave_reg_ofs	= 0x308,
		.sliver_reg_ofs	= 0xdc0,
		.phy_addr	= 3,
	},
};

static struct cpsw_platform_data cpsw_data = {
	.mdio_base		= CPSW_MDIO_BASE,
	.cpsw_base		= CPSW_BASE,
	.mdio_div		= 0xff,
	.channels		= 8,
	.cpdma_reg_ofs		= 0x800,
	.slaves			= 2,
	.slave_data		= cpsw_slaves,
	.ale_reg_ofs		= 0xd00,
	.ale_entries		= 1024,
	.host_port_reg_ofs	= 0x108,
	.hw_stats_reg_ofs	= 0x900,
	.bd_ram_ofs		= 0x2000,
	.mac_control		= (1 << 5),
	.control		= cpsw_control,
	.host_port_num		= 0,
	.version		= CPSW_CTRL_VERSION_2,
};

int board_eth_init(bd_t *bis)
{
	int ret;
	uint8_t mac_addr[6];
	uint32_t mac_hi, mac_lo;
	uint32_t ctrl_val;

	/* try reading mac address from efuse */
	mac_lo = readl((*ctrl)->control_core_mac_id_0_lo);
	mac_hi = readl((*ctrl)->control_core_mac_id_0_hi);
	mac_addr[0] = (mac_hi & 0xFF0000) >> 16;
	mac_addr[1] = (mac_hi & 0xFF00) >> 8;
	mac_addr[2] = mac_hi & 0xFF;
	mac_addr[3] = (mac_lo & 0xFF0000) >> 16;
	mac_addr[4] = (mac_lo & 0xFF00) >> 8;
	mac_addr[5] = mac_lo & 0xFF;

	if (!getenv("ethaddr")) {
		printf("<ethaddr> not set. Validating first E-fuse MAC\n");

		if (is_valid_ether_addr(mac_addr))
			eth_setenv_enetaddr("ethaddr", mac_addr);
	}

	mac_lo = readl((*ctrl)->control_core_mac_id_1_lo);
	mac_hi = readl((*ctrl)->control_core_mac_id_1_hi);
	mac_addr[0] = (mac_hi & 0xFF0000) >> 16;
	mac_addr[1] = (mac_hi & 0xFF00) >> 8;
	mac_addr[2] = mac_hi & 0xFF;
	mac_addr[3] = (mac_lo & 0xFF0000) >> 16;
	mac_addr[4] = (mac_lo & 0xFF00) >> 8;
	mac_addr[5] = mac_lo & 0xFF;

	if (!getenv("eth1addr")) {
		if (is_valid_ether_addr(mac_addr))
			eth_setenv_enetaddr("eth1addr", mac_addr);
	}

	ctrl_val = readl((*ctrl)->control_core_control_io1) & (~0x33);
	ctrl_val |= 0x22;
	writel(ctrl_val, (*ctrl)->control_core_control_io1);

	if (*omap_si_rev == DRA722_ES1_0) {
		cpsw_data.active_slave = 0;
		cpsw_data.slave_data[0].phy_addr = 3;
		pcf8575_output(PCF_ENET_MUX_ADDR, PCF_SEL_ENET_MUX_S0,
			       PCF8575_OUT_LOW);
	}

	ret = cpsw_register(&cpsw_data);
	if (ret < 0)
		printf("Error %d registering CPSW switch\n", ret);

	return ret;
}
#endif

#ifdef CONFIG_BOARD_EARLY_INIT_F
/* VTT regulator enable */
static inline void vtt_regulator_enable(void)
{
	if (omap_hw_init_context() == OMAP_INIT_CONTEXT_UBOOT_AFTER_SPL)
		return;

	/* Do not enable VTT for DRA722 */
	if (omap_revision() == DRA722_ES1_0)
		return;

	/*
	 * EVM Rev G and later use gpio7_11 for DDR3 termination.
	 * This is safe enough to do on older revs.
	 */
	gpio_request(GPIO_DDR_VTT_EN, "ddr_vtt_en");
	gpio_direction_output(GPIO_DDR_VTT_EN, 1);
}

int board_early_init_f(void)
{
	vtt_regulator_enable();
	return 0;
}
#endif
