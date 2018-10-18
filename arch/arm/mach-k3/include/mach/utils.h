/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2018 Linaro Ltd.
 * Sam Protsenko <semen.protsenko@linaro.org>
 */

#ifndef __ASM_ARCH_K3_UTILS_H_
#define __ASM_ARCH_K3_UTILS_H_

#ifdef CONFIG_FASTBOOT_FLASH
void k3_set_fastboot_vars(void);
#else
static inline void k3_set_fastboot_vars(void) { }
#endif

#endif /* __ASM_ARCH_K3_UTILS_H_ */
