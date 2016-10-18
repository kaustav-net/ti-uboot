/*
 * K2HK EVM : Board common header
 *
 * (C) Copyright 2014
 *     Texas Instruments Incorporated, <www.ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _KS2_BOARD
#define _KS2_BOARD

#include <asm/ti-common/keystone_net.h>

extern struct eth_priv_t eth_priv_cfg[];

static inline int board_is_k2g_gp(void)
{
#if defined(CONFIG_K2G_GP_EVM) && defined(CONFIG_SOC_K2G)
	return true;
#else
	return false;
#endif
}

static inline int board_is_k2g_ice(void)
{
#if !defined(CONFIG_K2G_GP_EVM) && defined(CONFIG_SOC_K2G)
	return true;
#else
	return false;
#endif
}

int get_num_eth_ports(void);
void spl_init_keystone_plls(void);

#endif
