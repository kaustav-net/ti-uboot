/*
 * Board specific misc setup
 *
 * (C) Copyright 2015
 * Texas Instruments, <www.ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <asm/arch/omap.h>
#include <asm/omap_common.h>
#include <asm/arch/spl.h>
#include <linux/linkage.h>

#ifdef CONFIG_ARM_ARCH_CP15_ERRATA
	.globl	arch_cp15_errata_workaround
ENTRY(arch_cp15_errata_workaround)
	push	{r4-r11, lr}	@ save registers - ROM code may pollute

#ifdef CONFIG_ARM_ERRATA_798870
	mrc     p15, 0, r0, c0, c0, 0   @ Read Main ID Register (MIDR)
	and     r0, r0, #0x000000f0     @ check rev
	cmp     r0, #0x000000f0         @ compare rev
	bne     skip_errata_798870      @ skip if not affected rev
	mrc     p15, 1, r1, c15, c0, 0  @ read l2 aux ctrl reg
	orr     r1, r1, #1 << 7         @ set bit #7
	ldr     r0, =0x104             @ Set L2 Cache Auxiliary control register - value in R0
	b omap_smc1
skip_errata_798870:
#endif
	pop	{r4-r11, pc}
ENDPROC(arch_cp15_errata_workaround)

#endif
