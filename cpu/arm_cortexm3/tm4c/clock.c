/*
 * Copyright (C) 2011
 * Dmitry Cherukhin, Emcraft Systems, dima_ch@emcraft.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <asm/io.h>
#include "clock.h"


/*
 * System register clock control mask and shift for PCLK dividers.
 */
#define PCLK_DIV_MASK		0x00000003
#define PCLK0_DIV_SHIFT		2
#define PCLK1_DIV_SHIFT		4
#define ACE_DIV_SHIFT		6

/*
 * System register MSS_CCC_DIV_CR mask and shift for GLB (FPGA fabric clock).
 */
#define OBDIV_SHIFT		8
#define OBDIV_MASK		0x0000001F
#define OBDIVHALF_SHIFT		13
#define OBDIVHALF_MASK		0x00000001

/*
 * Actel system boot version defines used to extract the system clock from eNVM
 * spare pages.
 * These defines allow detecting the presence of Actel system boot in eNVM spare
 * pages and the version of that system boot executable and associated
 * configuration data.
 */
#define SYSBOOT_KEY_ADDR	0x6008081C
#define SYSBOOT_KEY_VALUE	0x4C544341
#define SYSBOOT_VERSION_ADDR	0x60080840
#define SYSBOOT_1_3_FCLK_ADDR	0x6008162C
#define SYSBOOT_2_x_FCLK_ADDR	0x60081EAC

/*
 * The system boot version is stored in the least significant 24 bits of a word.
 * The FCLK is stored in eNVM from version 1.3.1 of the system boot. We expect
 * that the major version number of the system boot version will change if the
 * system boot configuration data layout needs to change.
 */
#define SYSBOOT_VERSION_MASK	0x00FFFFFF
#define MIN_SYSBOOT_VERSION	0x00010301
#define SYSBOOT_VERSION_2_X	0x00020000
#define MAX_SYSBOOT_VERSION	0x00030000

static unsigned long clock[CLOCK_END];

/*
 * Retrieve the system clock frequency from eNVM spare page if available.
 * Returns the frequency defined through SMARTFUSION_FCLK_FREQ
 * if FCLK cannot be retrieved from eNVM spare pages.
 * The FCLK frequency value selected in the MSS Configurator software tool
 * is stored in eNVM spare pages as part of the Actel system boot
 * configuration data.
 * @returns		FCLK clock frequency
 */
static unsigned long clock_get_fclk(void)
{
	unsigned long fclk = 0;

	/*
	 * XXX:
	 * Try some reading magic and if that fails, return a hard-coded
	 * default.
	 * Step one, just return hard coded value, step two see if we can
	 * calculate this.
	 */

	return fclk;
}

/*
 * Initialize the reference clocks.
 * Get the reference clock settings from the hardware.
 * System frequency (FCLK) and the other derivative clocks
 * coming out from firmware. These are defined by the Libero
 * project programmed onto SmartFusion and then, optionally, by firmware.
 */
void clock_init(void)
{
	unsigned long clk_cr;
	unsigned long pclk0_div;
	unsigned long pclk1_div;
	unsigned long ace_div;

	const unsigned long pclk_div_lut[4] = {1, 2, 4, 1};

	/*
	 * Read PCLK dividers from system registers.
	 */
	clk_cr = readl(&A2F_SYSREG->mss_clk_cr);
	pclk0_div = pclk_div_lut[(clk_cr >> PCLK0_DIV_SHIFT) & PCLK_DIV_MASK];
	pclk1_div = pclk_div_lut[(clk_cr >> PCLK1_DIV_SHIFT) & PCLK_DIV_MASK];
	ace_div	 = pclk_div_lut[(clk_cr >> ACE_DIV_SHIFT) & PCLK_DIV_MASK];

	/*
	 * Retrieve FCLK from eNVM spare pages
	 * if Actel system boot programmed as part of the system.
	 */
	clock[CLOCK_FCLK]	= clock_get_fclk();

	clock[CLOCK_PCLK0]	= clock[CLOCK_FCLK] / pclk0_div;
	clock[CLOCK_PCLK1]	= clock[CLOCK_FCLK] / pclk1_div;
	clock[CLOCK_ACE]	= clock[CLOCK_FCLK] / ace_div;

	/*
	 * Now, initialize the system timer clock source.
	 * Release systimer from reset
	 */
	//A2F_SYSREG->soft_rst_cr &= ~A2F_SOFT_RST_TIMER_SR;
	/*
	 * enable 32bit timer1
	 */
	//A2F_TIMER->timer64_mode &= ~A2F_TIM64_64MODE_EN;
	/*
	 * timer1 is used by envm driver
	 */
	//A2F_TIMER->timer1_ctrl = A2F_TIM_CTRL_MODE_ONESHOT | A2F_TIM_CTRL_EN;
	/*
	 * No reference clock
	 */
	//A2F_SYSREG->systick_cr &= ~A2F_SYSTICK_NOREF;
	/*
	 * div by 32
	 */
	//A2F_SYSREG->systick_cr |= (A2F_SYSTICK_STCLK_DIV_32 <<
	//			A2F_SYSTICK_STCLK_DIV_SHIFT);
	//A2F_SYSREG->systick_cr &= ~A2F_SYSTICK_TENMS_MSK;
	//A2F_SYSREG->systick_cr |= 0x7a12;

	clock[CLOCK_SYSTICK] = clock[CLOCK_FCLK] / 32;
}

/*
 * Return a clock value for the specified clock.
 */
/* XXX: Should be OK? */
ulong clock_get(enum clock clck)
{
	ulong res = 0;

	if (clck >= 0 && clck < CLOCK_END) {
		res = clock[clck];
	}

	return res;
}
