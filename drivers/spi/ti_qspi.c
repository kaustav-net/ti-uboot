/*
 * TI QSPI driver
 *
 * Copyright (C) 2013, Texas Instruments, Incorporated
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR /PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/omap.h>
#include <malloc.h>
#include <spi.h>

struct qspi_slave {
	struct spi_slave slave;
	unsigned int mode;
	u32 cmd;
	u32 dc;
};

#define to_qspi_slave(s) container_of(s, struct qspi_slave, slave)

struct qspi_regs {
	u32 pid;
	u32 pad0[3];
	u32 sysconfig;
	u32 pad1[3];
	u32 intr_status_raw_set;
	u32 intr_status_enabled_clear;
	u32 intr_enable_set;
	u32 intr_enable_clear;
	u32 intc_eoi;
	u32 pad2[3];
	u32 spi_clock_cntrl;
	u32 spi_dc;
	u32 spi_cmd;
	u32 spi_status;
	u32 spi_data;
	u32 spi_setup0;
	u32 spi_setup1;
	u32 spi_setup2;
	u32 spi_setup3;
	u32 spi_switch;
	u32 spi_data1;
	u32 spi_data2;
	u32 spi_data3;
};

static struct qspi_regs *qspi = (struct qspi_regs *)QSPI_BASE;

#define QSPI_TIMEOUT			2000000

#define QSPI_FCLK			192000000

/* Clock Control */
#define QSPI_CLK_EN			(1 << 31)
#define QSPI_CLK_DIV_MAX		0xffff

/* Command */
#define QSPI_EN_CS(n)			(n << 28)
#define QSPI_WLEN(n)			((n-1) << 19)
#define QSPI_3_PIN			(1 << 18)
#define QSPI_RD_SNGL			(1 << 16)
#define QSPI_WR_SNGL			(2 << 16)
#define QSPI_INVAL			(4 << 16)

/* Device Control */
#define QSPI_DD(m, n)			(m << (3 + n*8))
#define QSPI_CKPHA(n)			(1 << (2 + n*8))
#define QSPI_CSPOL(n)			(1 << (1 + n*8))
#define QSPI_CKPOL(n)			(1 << (n*8))

/* Status */
#define QSPI_WC				(1 << 1)
#define QSPI_BUSY			(1 << 0)
#define QSPI_WC_BUSY			(QSPI_WC | QSPI_BUSY)
#define QSPI_XFER_DONE			QSPI_WC

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	return 1;
}

void spi_cs_activate(struct spi_slave *slave)
{
	/* CS handled in xfer */
	return;
}

void spi_cs_deactivate(struct spi_slave *slave)
{
	/* CS handled in xfer */
	return;
}

void spi_init(void)
{
	/* nothing to do */
}

void spi_set_speed(struct spi_slave *slave, uint hz)
{
	uint clk_div;

	if (!hz)
		clk_div = 0;
	else
		clk_div = (QSPI_FCLK / hz) - 1;

	debug("%s: hz: %d, clock divider %d\n", __func__, hz, clk_div);

	/* disable SCLK */
	writel(readl(&qspi->spi_clock_cntrl) & ~QSPI_CLK_EN, &qspi->spi_clock_cntrl);

	if (clk_div < 0) {
		debug("%s: clock divider < 0, using /1 divider\n", __func__);
		clk_div = 0;
	}

	if (clk_div > QSPI_CLK_DIV_MAX) {
		debug("%s: clock divider >%d , using /%d divider\n",
			__func__, QSPI_CLK_DIV_MAX, QSPI_CLK_DIV_MAX + 1);
		clk_div = QSPI_CLK_DIV_MAX;
	}

	/* enable SCLK */
	writel(QSPI_CLK_EN | clk_div, &qspi->spi_clock_cntrl);
	debug("%s: spi_clock_cntrl %08x\n", __func__, readl(&qspi->spi_clock_cntrl));
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
				  unsigned int max_hz, unsigned int mode)
{
	struct qspi_slave *qslave;

	qslave = spi_alloc_slave(struct qspi_slave, bus, cs);
	if (!qslave)
		return NULL;

	spi_set_speed(&qslave->slave, max_hz);
	qslave->mode = mode;
	debug("%s: bus:%i cs:%i mode:%i\n", __func__, bus, cs, mode);

	return &qslave->slave;
}

void spi_free_slave(struct spi_slave *slave)
{
	struct qspi_slave *qslave = to_qspi_slave(slave);
	free(qslave);
}

int spi_claim_bus(struct spi_slave *slave)
{
	debug("%s: bus:%i cs:%i\n", __func__, slave->bus, slave->cs);

	writel(0, &qspi->spi_dc);
	writel(0, &qspi->spi_cmd);
	writel(0, &qspi->spi_data);

	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
	debug("%s: bus:%i cs:%i\n", __func__, slave->bus, slave->cs);

	writel(0, &qspi->spi_dc);
	writel(0, &qspi->spi_cmd);
	writel(0, &qspi->spi_data);
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
	     void *din, unsigned long flags)
{
	struct qspi_slave *qslave = to_qspi_slave(slave);
	uint words = bitlen >> 3; /* fixed 8-bit word length */
	const uchar *txp = dout;
	uchar *rxp = din;
	uint status;
	int timeout;

	debug("%s: bus:%i cs:%i bitlen:%i words:%i flags:%lx\n", __func__,
		slave->bus, slave->cs, bitlen, words, flags);
	if (bitlen == 0)
		return -1;

	if (bitlen % 8) {
		flags |= SPI_XFER_END;
		return -1;
	}

	/* setup command reg */
	qslave->cmd = 0;
	qslave->cmd |= QSPI_WLEN(8);
	qslave->cmd |= QSPI_EN_CS(slave->cs);
	if (flags & SPI_3WIRE)
		qslave->cmd |= QSPI_3_PIN;
	qslave->cmd |= 0xfff;

	/* setup device control reg */
	qslave->dc = 0;
	if (qslave->mode & SPI_CPHA)
		qslave->dc |= QSPI_CKPHA(slave->cs);
	if (qslave->mode & SPI_CPOL)
		qslave->dc |= QSPI_CKPOL(slave->cs);
	if (qslave->mode & SPI_CS_HIGH)
		qslave->dc |= QSPI_CSPOL(slave->cs);

	while (words--) {
		if (txp) {
			debug("tx cmd %08x dc %08x data %02x\n",
			      qslave->cmd | QSPI_WR_SNGL, qslave->dc, *txp);
			writel(*txp++, &qspi->spi_data);
			writel(qslave->dc, &qspi->spi_dc);
			writel(qslave->cmd | QSPI_WR_SNGL, &qspi->spi_cmd);
			status = readl(&qspi->spi_status);
			timeout = QSPI_TIMEOUT;
			while ((status & QSPI_WC_BUSY) != QSPI_XFER_DONE) {
				if (--timeout < 0) {
					printf("QSPI tx timed out\n");
					return -1;
				}
				status = readl(&qspi->spi_status);
			}
			debug("tx done, status %08x\n", status);
		}
		if (rxp) {
			debug("rx cmd %08x dc %08x\n",
			      qslave->cmd | QSPI_RD_SNGL, qslave->dc);
			writel(qslave->dc, &qspi->spi_dc);
			writel(qslave->cmd | QSPI_RD_SNGL, &qspi->spi_cmd);
			status = readl(&qspi->spi_status);
			timeout = QSPI_TIMEOUT;
			while ((status & QSPI_WC_BUSY) != QSPI_XFER_DONE) {
				if (--timeout < 0) {
					printf("QSPI rx timed out\n");
					return -1;
				}
				status = readl(&qspi->spi_status);
			}
			*rxp++ = readl(&qspi->spi_data);
			debug("rx done, status %08x, read %02x\n",
			      status, *(rxp-1));
		}
	}

	/* Terminate frame */
	if (flags & SPI_XFER_END)
		writel(qslave->cmd | QSPI_INVAL, &qspi->spi_cmd);

	return 0;
}
