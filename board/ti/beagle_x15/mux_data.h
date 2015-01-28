/*
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com
 *
 * Author: Felipe Balbi <balbi@ti.com>
 *
 * Based on board/ti/dra7xx/evm.c
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _MUX_DATA_BEAGLE_X15_H_
#define _MUX_DATA_BEAGLE_X15_H_

#include <asm/arch/mux_dra7xx.h>

const struct pad_conf_entry core_padconf_array_essential[] = {
	{MMC1_CLK, (IEN | PTU | PDIS | M0)},	/* MMC1_CLK */
	{MMC1_CMD, (IEN | PTU | PDIS | M0)},	/* MMC1_CMD */
	{MMC1_DAT0, (IEN | PTU | PDIS | M0)},	/* MMC1_DAT0 */
	{MMC1_DAT1, (IEN | PTU | PDIS | M0)},	/* MMC1_DAT1 */
	{MMC1_DAT2, (IEN | PTU | PDIS | M0)},	/* MMC1_DAT2 */
	{MMC1_DAT3, (IEN | PTU | PDIS | M0)},	/* MMC1_DAT3 */
	{MMC1_SDCD, (FSC | IEN | PTU | PDIS | M0)}, /* MMC1_SDCD */
	{MMC1_SDWP, (FSC | IEN | PTD | PEN | M14)}, /* MMC1_SDWP */
	{GPMC_A19, (IEN | PTU | PDIS | M1)},	/* mmc2_dat4 */
	{GPMC_A20, (IEN | PTU | PDIS | M1)},	/* mmc2_dat5 */
	{GPMC_A21, (IEN | PTU | PDIS | M1)},	/* mmc2_dat6 */
	{GPMC_A22, (IEN | PTU | PDIS | M1)},	/* mmc2_dat7 */
	{GPMC_A23, (IEN | PTU | PDIS | M1)},	/* mmc2_clk */
	{GPMC_A24, (IEN | PTU | PDIS | M1)},	/* mmc2_dat0 */
	{GPMC_A25, (IEN | PTU | PDIS | M1)},	/* mmc2_dat1 */
	{GPMC_A26, (IEN | PTU | PDIS | M1)},	/* mmc2_dat2 */
	{GPMC_A27, (IEN | PTU | PDIS | M1)},	/* mmc2_dat3 */
	{GPMC_CS1, (IEN | PTU | PDIS | M1)},	/* mmm2_cmd */
	{UART3_RXD, (FSC | IEN | PTU | PDIS | M0)}, /* UART3_RXD */
	{UART3_TXD, (FSC | IEN | PTU | PDIS | M0)}, /* UART3_TXD */
	{I2C1_SDA, (IEN | PTU | PDIS | M0)},	/* I2C1_SDA */
	{I2C1_SCL, (IEN | PTU | PDIS | M0)},	/* I2C1_SCL */
	{MDIO_MCLK, (PTU | PEN | M0)},		/* MDIO_MCLK  */
	{MDIO_D, (IEN | PTU | PEN | M0)},	/* MDIO_D  */
	{RGMII0_TXC, (PIN_OUTPUT | MANUAL_MODE | M0)},
	{RGMII0_TXCTL, (PIN_OUTPUT | MANUAL_MODE | M0)},
	{RGMII0_TXD3, (PIN_OUTPUT | MANUAL_MODE | M0)},
	{RGMII0_TXD2, (PIN_OUTPUT | MANUAL_MODE | M0)},
	{RGMII0_TXD1, (PIN_OUTPUT | MANUAL_MODE | M0)},
	{RGMII0_TXD0, (PIN_OUTPUT | MANUAL_MODE | M0)},
	{RGMII0_RXC, (PIN_INPUT | MANUAL_MODE | M0)},
	{RGMII0_RXCTL, (PIN_INPUT | MANUAL_MODE | M0)},
	{RGMII0_RXD3, (PIN_INPUT | MANUAL_MODE | M0)},
	{RGMII0_RXD2, (PIN_INPUT | MANUAL_MODE | M0)},
	{RGMII0_RXD1, (PIN_INPUT | MANUAL_MODE | M0)},
	{RGMII0_RXD0, (PIN_INPUT | MANUAL_MODE | M0)},
	{USB1_DRVVBUS, (M0 | FSC) },
	{SPI1_CS1, (PEN | IDIS | M14) }, /* GPIO7_11 */
};

const struct pad_conf_entry early_padconf[] = {
	{UART3_RXD, (PIN_INPUT_SLEW | M0)}, /* UART3_RXD */
	{UART3_TXD, (PIN_INPUT_SLEW | M0)}, /* UART3_TXD */
	{I2C1_SDA, (PIN_INPUT_PULLUP | M0)},	/* I2C1_SDA */
	{I2C1_SCL, (PIN_INPUT_PULLUP | M0)},	/* I2C1_SCL */
};

#ifdef CONFIG_IODELAY_RECALIBRATION
const struct iodelay_cfg_entry iodelay_cfg_array[] = {
	{0x6F0, 480, 0}, /* RGMMI0_RXC_IN */
	{0x6FC, 111, 1641}, /* RGMMI0_RXCTL_IN */
	{0x708, 272, 1116}, /* RGMMI0_RXD0_IN */
	{0x714, 243, 1260}, /* RGMMI0_RXD1_IN */
	{0x720, 0, 1614}, /* RGMMI0_RXD2_IN */
	{0x72C, 105, 1673}, /* RGMMI0_RXD3_IN */
	{0x740, 531, 120}, /* RGMMI0_TXC_OUT */
	{0x74C, 11, 60}, /* RGMMI0_TXCTL_OUT */
	{0x758, 7, 120}, /* RGMMI0_TXD0_OUT */
	{0x764, 0, 0}, /* RGMMI0_TXD1_OUT */
	{0x770, 276, 120}, /* RGMMI0_TXD2_OUT */
	{0x77C, 440, 120}, /* RGMMI0_TXD3_OUT */
	/*  Slave 1 - GMAC RGMII2: Need to add mux for RGMII2 */
	{0xAB0, 702, 0}, /* CFG_VIN2A_D18_IN */
	{0xABC, 136, 976}, /* CFG_VIN2A_D19_IN */
	{0xAD4, 210, 1357}, /* CFG_VIN2A_D20_IN */
	{0xAE0, 189, 1462}, /* CFG_VIN2A_D21_IN */
	{0xAEC, 232, 1278}, /* CFG_VIN2A_D22_IN */
	{0xAF8, 0, 1397}, /* CFG_VIN2A_D23_IN */
	{0xA70, 1551, 115}, /* CFG_VIN2A_D12_OUT */
	{0xA7C, 816, 0}, /* CFG_VIN2A_D13_OUT */
	{0xA88, 876, 0}, /* CFG_VIN2A_D14_OUT */
	{0xA94, 312, 0}, /* CFG_VIN2A_D15_OUT */
	{0xAA0, 58, 0}, /* CFG_VIN2A_D16_OUT */
	{0xAAC, 0, 0}, /* CFG_VIN2A_D17_OUT */
};
#endif
#endif /* _MUX_DATA_BEAGLE_X15_H_ */
