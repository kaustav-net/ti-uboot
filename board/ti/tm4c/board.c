/*
 * board/ti/tm4c/board.c
 *
 * Board specific code the TI Tiva C "TM4C" family.
 *
 * Copyright (C) 2014, Texas Instruments, Inc. http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	return 0;
}

int checkboard(void)
{
	puts("Board: TM4C\n");
	return 0;
}

/*
 * The TM4C platforms have 256KiB of SRAM and no external DDR.
 */
int dram_init (void)
{

	/*
	 * Fill in global info with description of SRAM configuration
	 */
	gd->bd->bi_dram[0].start = CONFIG_MEM_RAM_BASE;
	gd->bd->bi_dram[0].size  = CONFIG_MEM_RAM_SIZE;

	cortex_m3_mpu_full_access();

	return 0;
}

int misc_init_r(void)
{
	return 0;
}

int board_eth_init(bd_t *bis)
{
	/* Driver eth init func */
	return 0;
}
