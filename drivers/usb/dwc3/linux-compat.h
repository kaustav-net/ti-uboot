/**
 * linux-compat.h - DesignWare USB3 Linux Compatibiltiy Adapter  Header
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com
 *
 * Authors: Kishon Vijay Abraham I <kishon@ti.com>
 *
 * Taken from Linux Kernel v3.16 (drivers/usb/dwc3/core.c) and ported
 * to uboot.
 *
 * SPDX-License-Identifier:     GPL-2.0
 *
 */

#ifndef __DWC3_LINUX_COMPAT__
#define __DWC3_LINUX_COMPAT__
#include <linux/compat.h>
#include <linux/usb/ch9.h>
#include <linux/usb/otg.h>
#include <linux/string.h>
#include <common.h>
#include <malloc.h>
#include <asm/errno.h>
#include <stdarg.h>

typedef int spinlock_t;
typedef u32 resource_size_t;
typedef enum irqreturn irqreturn_t;

#include <linux/ioport.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

struct device {
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define BUILD_BUG_ON_NOT_POWER_OF_2(n)

static inline size_t strlcat(char *dest, const char *src, size_t n)
{
	strcat(dest, src);
	return strlen(dest) + strlen(src);
}

static inline void *devm_kzalloc(struct device *dev, unsigned int size,
				 unsigned int flags)
{
	return kzalloc(size, flags);
}

static inline void *kmalloc_array(size_t n, size_t size, gfp_t flags)
{
	return kzalloc(n * size, flags);
}

#define GFP_KERNEL	0
#define MAX_ERRNO	4095
#define IS_ERR_VALUE(x)	((x) >= (unsigned long)-MAX_ERRNO && (x) < 0)

static inline void *ERR_PTR(long error)
{
	return (void *)error;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long)ptr;
}

static inline long IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

#define pr_debug(format)		debug(format)
#define dev_WARN(dev, format, arg...)	debug(format, ##arg)
#define WARN(val, format, arg...)	debug(format, ##arg)
#define WARN_ON_ONCE(val)		debug("Error %d\n", val)

#define IS_ALIGNED(x, a)	(((x) & ((typeof(x))(a) - 1)) == 0)
#define PTR_ALIGN(p, a)		((typeof(p))ALIGN((unsigned long)(p), (a)))

#define upper_32_bits(n)	((u32)(((n) >> 16) >> 16))
#define lower_32_bits(n)	((u32)(n))

#endif
