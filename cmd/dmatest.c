// SPDX-License-Identifier: GPL-2.0+
/*
 *  Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com
 *
 */

#include <common.h>
#include <dm.h>
#include <dma.h>
#include <asm/dma-mapping.h>
#include <malloc.h>

#define DMATEST_DEFAULT_SIZE 128

static int do_dmatest(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long dummy;
	int i;
	void *src = dma_alloc_coherent(DMATEST_DEFAULT_SIZE, &dummy);
	void *dst = dma_alloc_coherent(DMATEST_DEFAULT_SIZE, &dummy);
	char *src_buf = src;

	memset(dst, 0, DMATEST_DEFAULT_SIZE);
	printf("src buf addr %p dst buf addr %p\n", src, dst);
	for (i = 0; i < DMATEST_DEFAULT_SIZE; i++)
		src_buf[i] = i;

	flush_dcache_range((unsigned long)src,
			   ALIGN((unsigned long)src + DMATEST_DEFAULT_SIZE,
				 ARCH_DMA_MINALIGN));

	dma_memcpy(dst, src, DMATEST_DEFAULT_SIZE);
	if (!memcmp(src_buf, dst, DMATEST_DEFAULT_SIZE))
		printf("DMATEST WAS A SUCCESS!!!!!\n");
	else
		printf("DMATEST FAILED!!!!!\n");

	return 0;
}

U_BOOT_CMD(
	dmatest, CONFIG_SYS_MAXARGS, 0, do_dmatest,
	"Test memcpy DMA",
	"dmatest\n"
)
