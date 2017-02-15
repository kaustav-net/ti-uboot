/*
 * K2HK: secure kernel command file
 *
 * (C) Copyright 2012-2014
 *     Texas Instruments Incorporated, <www.ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <mach/mon.h>
asm(".arch_extension sec\n\t");

int mon_install(u32 addr, u32 dpsc, u32 freq)
{
	int result;

	__asm__ __volatile__ (
		"stmfd r13!, {lr}\n"
		"mov r0, %1\n"
		"mov r1, %2\n"
		"mov r2, %3\n"
		"blx r0\n"
		"ldmfd r13!, {lr}\n"
		: "=&r" (result)
		: "r" (addr), "r" (dpsc), "r" (freq)
		: "cc", "r0", "r1", "r2", "memory");
	return result;
}

int mon_power_on(int core_id, void *ep)
{
	int result;

	asm volatile (
		"stmfd  r13!, {lr}\n"
		"mov r1, %1\n"
		"mov r2, %2\n"
		"mov r0, #0\n"
		"smc	#0\n"
		"ldmfd  r13!, {lr}\n"
		: "=&r" (result)
		: "r" (core_id), "r" (ep)
		: "cc", "r0", "r1", "r2", "memory");
	return  result;
}

int mon_power_off(int core_id)
{
	int result;

	asm volatile (
		"stmfd  r13!, {lr}\n"
		"mov r1, %1\n"
		"mov r0, #1\n"
		"smc	#1\n"
		"ldmfd  r13!, {lr}\n"
		: "=&r" (result)
		: "r" (core_id)
		: "cc", "r0", "r1", "memory");
	return  result;
}

#ifdef CONFIG_TI_SECURE_DEVICE
#define KS2_HS_SEC_HEADER_LEN	0x60
#define KS2_AUTH_CMD		130

/**
 * k2_hs_bm_auth() - Invokes security functions using a
 * proprietary TI interface. This binary and source for
 * this is available in the secure development package or
 * SECDEV. For details on how to access this please refer
 * doc/README.ti-secure
 *
 * @cmd: Secure monitor command
 * @arg1: Argument for command
 *
 * returns non-zero value on success, zero on error
 */
static int k2_hs_bm_auth(int cmd, void *arg1)
{
	int result;

	asm volatile (
		"stmfd  r13!, {r4-r12, lr}\n"
		"mov r0, %1\n"
		"mov r1, %2\n"
		"smc #2\n"
		"ldmfd r13!, {r4-r12, lr}\n"
		: "=&r" (result)
		: "r" (cmd), "r" (arg1)
		: "cc", "r0", "r1", "memory");

	return  result;
}

static void k2_hs_auth(void *addr)
{
	int ret = 0;

	ret = k2_hs_bm_auth(KS2_AUTH_CMD, addr);
	if (ret == 0)
		hang();
}

void board_fit_image_post_process(void **p_image, size_t *p_size)
{
	void *dst = *p_image;
	void *src = dst + KS2_HS_SEC_HEADER_LEN;

	k2_hs_auth(*p_image);

	/*
	* Overwrite the image headers  after authentication
	* and decryption. Update size to reflect removal
	* of header.
	*/
	*p_size -= KS2_HS_SEC_HEADER_LEN;
	memcpy(dst, src, *p_size);
}
#endif
