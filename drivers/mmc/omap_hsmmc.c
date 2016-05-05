/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Sukumar Ghorai <s-ghorai@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation's version 2 of
 * the License.
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

#include <config.h>
#include <common.h>
#include <malloc.h>
#include <mmc.h>
#include <part.h>
#include <i2c.h>
#include <linux/compat.h>
#include <twl4030.h>
#include <twl6030.h>
#include <palmas.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/mmc_host_def.h>
#include <asm/arch/sys_proto.h>
#include <errno.h>

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

/* simplify defines to OMAP_HSMMC_USE_GPIO */
#if (defined(CONFIG_OMAP_GPIO) && !defined(CONFIG_SPL_BUILD)) || \
	(defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_GPIO_SUPPORT))
#define OMAP_HSMMC_USE_GPIO
#else
#undef OMAP_HSMMC_USE_GPIO
#endif

#ifdef OMAP_HSMMC_IPINFO
#define ADMA_SUPPORT
#endif

#if !defined(CONFIG_SPL_BUILD) && !defined(CONFIG_MMC_NO_HIGH_SPEED)
#define HIGH_SPEED_SUPPORT
#endif

/* common definitions for all OMAPs */
#define SYSCTL_SRC	(1 << 25)
#define SYSCTL_SRD	(1 << 26)

struct omap_hsmmc_data {
	struct hsmmc *base_addr;
	struct mmc_config cfg;
	uint bus_width;
	uint clock;
#ifdef OMAP_HSMMC_USE_GPIO
	int cd_gpio;
	int wp_gpio;
#endif

	uint iov;
	u8 controller_flags;
#ifdef HIGH_SPEED_SUPPORT
	uint timing;
#endif
#ifdef ADMA_SUPPORT
	struct omap_hsmmc_adma_desc *adma_desc_table;
	uint desc_slot;
#endif
};

#ifndef CONFIG_HSMMC1_IOV
#define CONFIG_HSMMC1_IOV	IOV_1V8
#endif
#ifndef CONFIG_HSMMC2_IOV
#define CONFIG_HSMMC2_IOV	IOV_1V8
#endif
#ifndef CONFIG_HSMMC3_IOV
#define CONFIG_HSMMC3_IOV	IOV_1V8
#endif

#ifdef ADMA_SUPPORT
struct omap_hsmmc_adma_desc {
	u8 attr;
	u8 reserved;
	u16 len;
	u32 addr;
};

#define ADMA_MAX_LEN	63488

/* Decriptor table defines */
#define ADMA_DESC_ATTR_VALID		BIT(0)
#define ADMA_DESC_ATTR_END		BIT(1)
#define ADMA_DESC_ATTR_INT		BIT(2)
#define ADMA_DESC_ATTR_ACT1		BIT(4)
#define ADMA_DESC_ATTR_ACT2		BIT(5)

#define ADMA_DESC_TRANSFER_DATA		ADMA_DESC_ATTR_ACT2
#define ADMA_DESC_LINK_DESC	(ADMA_DESC_ATTR_ACT1 | ADMA_DESC_ATTR_ACT2)
#endif
/* If we fail after 1 second wait, something is really bad */
#define MAX_RETRY_MS	1000
#define OMAP_HSMMC_SUPPORTS_DUAL_VOLT		BIT(0)
#define OMAP_HSMMC_USE_ADMA			BIT(1)

static int mmc_read_data(struct hsmmc *mmc_base, char *buf, unsigned int size);
static int mmc_write_data(struct hsmmc *mmc_base, const char *buf,
			unsigned int siz);
static void omap_hsmmc_start_clock(struct hsmmc *mmc_base);
static void omap_hsmmc_stop_clock(struct hsmmc *mmc_base);
static void mmc_reset_controller_fsm(struct hsmmc *mmc_base, u32 bit);
static int omap_hsmmc_platform_fixup(struct mmc *mmc);

#ifdef OMAP_HSMMC_USE_GPIO
static int omap_mmc_setup_gpio_in(int gpio, const char *label)
{
	if (!gpio_is_valid(gpio))
		return -1;

	if (gpio_request(gpio, label) < 0)
		return -1;

	if (gpio_direction_input(gpio) < 0)
		return -1;

	return gpio;
}
#endif

#if defined(CONFIG_OMAP44XX) && defined(CONFIG_TWL6030_POWER)
static void omap4_vmmc_pbias_config(struct mmc *mmc)
{
	u32 value = 0;

	value = readl((*ctrl)->control_pbiaslite);
	value &= ~(MMC1_PBIASLITE_PWRDNZ | MMC1_PWRDNZ);
	writel(value, (*ctrl)->control_pbiaslite);
	/* set VMMC to 3V */
	twl6030_power_mmc_init();
	value = readl((*ctrl)->control_pbiaslite);
	value |= MMC1_PBIASLITE_VMODE | MMC1_PBIASLITE_PWRDNZ | MMC1_PWRDNZ;
	writel(value, (*ctrl)->control_pbiaslite);
}
#endif

#if defined(CONFIG_OMAP54XX) && defined(CONFIG_PALMAS_POWER)
static void omap5_pbias_config(struct mmc *mmc)
{
	u32 value = 0;

	value = readl((*ctrl)->control_pbias);
	value &= ~SDCARD_PWRDNZ;
	writel(value, (*ctrl)->control_pbias);
	udelay(10); /* wait 10 us */
	value &= ~SDCARD_BIAS_PWRDNZ;
	writel(value, (*ctrl)->control_pbias);

	palmas_mmc1_poweron_ldo();

	value = readl((*ctrl)->control_pbias);
	value |= SDCARD_BIAS_PWRDNZ;
	writel(value, (*ctrl)->control_pbias);
	udelay(150); /* wait 150 us */
	value |= SDCARD_PWRDNZ;
	writel(value, (*ctrl)->control_pbias);
	udelay(150); /* wait 150 us */
}
#endif

unsigned char mmc_board_init(struct mmc *mmc)
{
#if defined(CONFIG_OMAP34XX)
	t2_t *t2_base = (t2_t *)T2_BASE;
	struct prcm *prcm_base = (struct prcm *)PRCM_BASE;
	u32 pbias_lite;

	pbias_lite = readl(&t2_base->pbias_lite);
	pbias_lite &= ~(PBIASLITEPWRDNZ1 | PBIASLITEPWRDNZ0);
	writel(pbias_lite, &t2_base->pbias_lite);
#endif
#if defined(CONFIG_TWL4030_POWER)
	twl4030_power_mmc_init();
	mdelay(100);	/* ramp-up delay from Linux code */
#endif
#if defined(CONFIG_OMAP34XX)
	writel(pbias_lite | PBIASLITEPWRDNZ1 |
		PBIASSPEEDCTRL0 | PBIASLITEPWRDNZ0,
		&t2_base->pbias_lite);

	writel(readl(&t2_base->devconf0) | MMCSDIO1ADPCLKISEL,
		&t2_base->devconf0);

	writel(readl(&t2_base->devconf1) | MMCSDIO2ADPCLKISEL,
		&t2_base->devconf1);

	/* Change from default of 52MHz to 26MHz if necessary */
	if (!(mmc->cfg->host_caps & MMC_MODE_HS_52MHz))
		writel(readl(&t2_base->ctl_prog_io1) & ~CTLPROGIO1SPEEDCTRL,
			&t2_base->ctl_prog_io1);

	writel(readl(&prcm_base->fclken1_core) |
		EN_MMC1 | EN_MMC2 | EN_MMC3,
		&prcm_base->fclken1_core);

	writel(readl(&prcm_base->iclken1_core) |
		EN_MMC1 | EN_MMC2 | EN_MMC3,
		&prcm_base->iclken1_core);
#endif

#if defined(CONFIG_OMAP44XX) && defined(CONFIG_TWL6030_POWER)
	/* PBIAS config needed for MMC1 only */
	if (mmc->block_dev.dev == 0)
		omap4_vmmc_pbias_config(mmc);
#endif
#if defined(CONFIG_OMAP54XX) && defined(CONFIG_PALMAS_POWER)
	if (mmc->block_dev.dev == 0)
		omap5_pbias_config(mmc);
#endif

	return 0;
}

void mmc_init_stream(struct hsmmc *mmc_base)
{
	ulong start;

	writel(readl(&mmc_base->con) | INIT_INITSTREAM, &mmc_base->con);

	writel(MMC_CMD0, &mmc_base->cmd);
	start = get_timer(0);
	while (!(readl(&mmc_base->stat) & CC_MASK)) {
		if (get_timer(0) - start > MAX_RETRY_MS) {
			printf("%s: timedout waiting for cc!\n", __func__);
			return;
		}
	}
	writel(CC_MASK, &mmc_base->stat)
		;
	writel(MMC_CMD0, &mmc_base->cmd)
		;
	start = get_timer(0);
	while (!(readl(&mmc_base->stat) & CC_MASK)) {
		if (get_timer(0) - start > MAX_RETRY_MS) {
			printf("%s: timedout waiting for cc2!\n", __func__);
			return;
		}
	}
	writel(readl(&mmc_base->con) & ~INIT_INITSTREAM, &mmc_base->con);
}

#ifdef HIGH_SPEED_SUPPORT
static void omap_hsmmc_set_timing(struct mmc *mmc)
{
	u32 val;
	struct hsmmc *mmc_base;
	struct omap_hsmmc_data *priv = (struct omap_hsmmc_data *)mmc->priv;

	mmc_base = priv->base_addr;

	writel(readl(&mmc_base->con) & ~DDR, &mmc_base->con);
	val = readl(&mmc_base->ac12);
	val &= ~AC12_UHSMC_MASK;
	switch (mmc->timing) {
	case MMC_TIMING_MMC_HS200:
		val |= AC12_UHSMC_SDR104;
		break;
	case MMC_TIMING_SD_HS:
	case MMC_TIMING_MMC_HS:
		val |= AC12_UHSMC_RES;
		break;
	case MMC_TIMING_MMC_DDR52:
		val |= AC12_UHSMC_RES;
		writel(readl(&mmc_base->con) | DDR, &mmc_base->con);
		break;
	default:
		val |= AC12_UHSMC_RES;
		break;
	}
	writel(val, &mmc_base->ac12);
	priv->timing = mmc->timing;
}
#endif

static void omap_hsmmc_conf_bus_power(struct mmc *mmc)
{
	struct hsmmc *mmc_base;
	struct omap_hsmmc_data *priv = (struct omap_hsmmc_data *)mmc->priv;
	u32 val;

	mmc_base = priv->base_addr;

	val = readl(&mmc_base->hctl) & ~SDVS_MASK;

	switch (priv->iov) {
	case IOV_3V3:
		val |= SDVS_3V3;
		break;
	case IOV_3V0:
		val |= SDVS_3V0;
		break;
	case IOV_1V8:
		val |= SDVS_1V8;
		break;
	}

	writel(val, &mmc_base->hctl);
}

static void omap_hsmmc_set_capabilities(struct mmc *mmc)
{
	struct hsmmc *mmc_base;
	struct omap_hsmmc_data *priv = (struct omap_hsmmc_data *)mmc->priv;
	u32 val;

	mmc_base = priv->base_addr;
	val = readl(&mmc_base->capa);

	if (priv->controller_flags & OMAP_HSMMC_SUPPORTS_DUAL_VOLT) {
		val |= (VS30_3V0SUP | VS18_1V8SUP);
		priv->iov = IOV_3V0;
	} else {
		switch (priv->iov) {
		case IOV_3V3:
			val |= VS33_3V3SUP;
			break;
		case IOV_3V0:
			val |= VS30_3V0SUP;
			break;
		case IOV_1V8:
			val |= VS18_1V8SUP;
			break;
		}
	}

	writel(val, &mmc_base->capa);
}

#ifdef HIGH_SPEED_SUPPORT
static void omap_hsmmc_disable_tuning(struct mmc *mmc)
{
	int val;
	struct hsmmc *mmc_base;

	mmc_base = ((struct omap_hsmmc_data *)mmc->priv)->base_addr;
	val = readl(&mmc_base->ac12);
	val &= ~(AC12_SCLK_SEL);
	writel(val, &mmc_base->ac12);

	val = readl(&mmc_base->dll);
	val &= ~(DLL_FORCE_VALUE | DLL_SWT);
	writel(val, &mmc_base->dll);
}

static void omap_hsmmc_set_dll(struct mmc *mmc, int count)
{
	int i;
	u32 val;
	struct hsmmc *mmc_base;

	mmc_base = ((struct omap_hsmmc_data *)mmc->priv)->base_addr;
	val = readl(&mmc_base->dll);
	val |= DLL_FORCE_VALUE;
	val &= ~(DLL_FORCE_SR_C_MASK << DLL_FORCE_SR_C_SHIFT);
	val |= (count << DLL_FORCE_SR_C_SHIFT);
	writel(val, &mmc_base->dll);

	val |= DLL_CALIB;
	writel(val, &mmc_base->dll);
	for (i = 0; i < 1000; i++) {
		if (readl(&mmc_base->dll) & DLL_CALIB)
			break;
	}
	val &= ~DLL_CALIB;
	writel(val, &mmc_base->dll);
}

static int omap_hsmmc_execute_tuning(struct mmc *mmc, uint opcode)
{
	struct hsmmc *mmc_base;

	u32 val;
	u8 cur_match, prev_match = 0;
	int ret;
	u32 phase_delay = 0;
	u32 start_window = 0, max_window = 0;
	u32 length = 0, max_len = 0;

	/* clock tuning is not needed for upto 52MHz */
	if (mmc->clock <= 52000000)
		return 0;

	mmc_base = ((struct omap_hsmmc_data *)mmc->priv)->base_addr;

	val = readl(&mmc_base->ac12);
	val |= AC12_V1V8_SIGEN;
	writel(val, &mmc_base->ac12);
	val = readl(&mmc_base->dll);
	val |= DLL_SWT;
	writel(val, &mmc_base->dll);
	while (phase_delay <= MAX_PHASE_DELAY) {
		omap_hsmmc_set_dll(mmc, phase_delay);

		cur_match = !mmc_send_tuning(mmc, opcode, NULL);

		if (cur_match) {
			if (prev_match) {
				length++;
			} else {
				start_window = phase_delay;
				length = 1;
			}
		}

		if (length > max_len) {
			max_window = start_window;
			max_len = length;
		}

		prev_match = cur_match;
		phase_delay += 4;
		udelay(100);
	val = readl(&mmc_base->dll);
	val &= ~DLL_FORCE_VALUE;
	writel(val, &mmc_base->dll);
	}

	if (!max_len) {
		ret = -EIO;
		goto tuning_error;
	}

	val = readl(&mmc_base->ac12);
	if (!(val & AC12_SCLK_SEL)) {
		ret = -EIO;
		goto tuning_error;
	}

	phase_delay = max_window + 4 * (max_len >> 1);
	omap_hsmmc_set_dll(mmc, phase_delay);

	mmc_reset_controller_fsm(mmc_base, SYSCTL_SRD);
	mmc_reset_controller_fsm(mmc_base, SYSCTL_SRC);

	return 0;

tuning_error:

	omap_hsmmc_disable_tuning(mmc);
	mmc_reset_controller_fsm(mmc_base, SYSCTL_SRD);
	mmc_reset_controller_fsm(mmc_base, SYSCTL_SRC);

	return ret;
}
#endif

static void mmc_enable_irq(struct mmc *mmc, struct mmc_cmd *cmd)
{
	u32 irq_mask = INT_EN_MASK;
	struct hsmmc *mmc_base;

	mmc_base = ((struct omap_hsmmc_data *)mmc->priv)->base_addr;

	/*
	 * TODO: Errata i802 indicates only DCRC interrupts can occur during
	 * tuning procedure and DCRC should be disabled. But see occurences
	 * of DEB, CIE, CEB, CCRC interupts during tuning procedure. These
	 * interrupts occur along with BRR, so the data is actually in the
	 * buffer. It has to be debugged why these interrutps occur
	 */
	if (cmd && cmd->cmdidx == MMC_SEND_TUNING_BLOCK_HS200)
		irq_mask &= ~(IE_DEB | IE_DCRC | IE_CIE | IE_CEB | IE_CCRC);

	writel(irq_mask, &mmc_base->ie);
}

static int omap_hsmmc_init_setup(struct mmc *mmc)
{
	struct hsmmc *mmc_base;
	unsigned int reg_val;
	unsigned int dsor;
	ulong start;
	struct omap_hsmmc_data *priv = (struct omap_hsmmc_data *)mmc->priv;
#ifdef ADMA_SUPPORT
	struct hsmmc_ipinfo *mmc_ipinfo;
#endif

	mmc_base = priv->base_addr;
	mmc_board_init(mmc);

	writel(readl(&mmc_base->sysconfig) | MMC_SOFTRESET,
		&mmc_base->sysconfig);
	start = get_timer(0);
	while ((readl(&mmc_base->sysstatus) & RESETDONE) == 0) {
		if (get_timer(0) - start > MAX_RETRY_MS) {
			printf("%s: timedout waiting for cc2!\n", __func__);
			return TIMEOUT;
		}
	}
	writel(readl(&mmc_base->sysctl) | SOFTRESETALL, &mmc_base->sysctl);
	start = get_timer(0);
	while ((readl(&mmc_base->sysctl) & SOFTRESETALL) != 0x0) {
		if (get_timer(0) - start > MAX_RETRY_MS) {
			printf("%s: timedout waiting for softresetall!\n",
				__func__);
			return TIMEOUT;
		}
	}

	omap_hsmmc_set_capabilities(mmc);
	omap_hsmmc_conf_bus_power(mmc);

#ifdef ADMA_SUPPORT
	mmc_ipinfo = (void*)((struct omap_hsmmc_data *)mmc->priv)->base_addr - sizeof(struct hsmmc_ipinfo);
	reg_val = readl(&mmc_ipinfo->hl_hwinfo);
	if (reg_val & MADMA_EN)
		priv->controller_flags |= OMAP_HSMMC_USE_ADMA;
#endif

	reg_val = readl(&mmc_base->con) & RESERVED_MASK;

	writel(CTPL_MMC_SD | reg_val | WPP_ACTIVEHIGH | CDP_ACTIVEHIGH |
		MIT_CTO | DW8_1_4BITMODE | MODE_FUNC | STR_BLOCK |
		HR_NOHOSTRESP | INIT_NOINIT | NOOPENDRAIN, &mmc_base->con);

	dsor = 240;
	mmc_reg_out(&mmc_base->sysctl, (ICE_MASK | DTO_MASK | CEN_MASK),
		(ICE_STOP | DTO_15THDTO));
	mmc_reg_out(&mmc_base->sysctl, ICE_MASK | CLKD_MASK,
		(dsor << CLKD_OFFSET) | ICE_OSCILLATE);
	start = get_timer(0);
	while ((readl(&mmc_base->sysctl) & ICS_MASK) == ICS_NOTREADY) {
		if (get_timer(0) - start > MAX_RETRY_MS) {
			printf("%s: timedout waiting for ics!\n", __func__);
			return TIMEOUT;
		}
	}
	writel(readl(&mmc_base->sysctl) | CEN_ENABLE, &mmc_base->sysctl);

	writel(readl(&mmc_base->hctl) | SDBP_PWRON, &mmc_base->hctl);

	mmc_enable_irq(mmc, NULL);
	mmc_init_stream(mmc_base);

	return 0;
}

/*
 * MMC controller internal finite state machine reset
 *
 * Used to reset command or data internal state machines, using respectively
 * SRC or SRD bit of SYSCTL register
 */
static void mmc_reset_controller_fsm(struct hsmmc *mmc_base, u32 bit)
{
	ulong start;

	mmc_reg_out(&mmc_base->sysctl, bit, bit);

	/*
	 * CMD(DAT) lines reset procedures are slightly different
	 * for OMAP3 and OMAP4(AM335x,OMAP5,DRA7xx).
	 * According to OMAP3 TRM:
	 * Set SRC(SRD) bit in MMCHS_SYSCTL register to 0x1 and wait until it
	 * returns to 0x0.
	 * According to OMAP4(AM335x,OMAP5,DRA7xx) TRMs, CMD(DATA) lines reset
	 * procedure steps must be as follows:
	 * 1. Initiate CMD(DAT) line reset by writing 0x1 to SRC(SRD) bit in
	 *    MMCHS_SYSCTL register (SD_SYSCTL for AM335x).
	 * 2. Poll the SRC(SRD) bit until it is set to 0x1.
	 * 3. Wait until the SRC (SRD) bit returns to 0x0
	 *    (reset procedure is completed).
	 */
#if defined(CONFIG_OMAP44XX) || defined(CONFIG_OMAP54XX) || \
	defined(CONFIG_AM33XX)
	if (!(readl(&mmc_base->sysctl) & bit)) {
		start = get_timer(0);
		while (!(readl(&mmc_base->sysctl) & bit)) {
			if (get_timer(0) - start > MAX_RETRY_MS)
				return;
		}
	}
#endif
	start = get_timer(0);
	while ((readl(&mmc_base->sysctl) & bit) != 0) {
		if (get_timer(0) - start > MAX_RETRY_MS) {
			printf("%s: timedout waiting for sysctl %x to clear\n",
				__func__, bit);
			return;
		}
	}
}

#ifdef ADMA_SUPPORT
static int omap_hsmmc_adma_desc(struct mmc *mmc, char *buf, u16 len, bool end)
{
	struct omap_hsmmc_data *priv = (struct omap_hsmmc_data *)mmc->priv;
	struct omap_hsmmc_adma_desc *desc;
	u8 attr;

	desc = &priv->adma_desc_table[priv->desc_slot];

	attr = ADMA_DESC_ATTR_VALID | ADMA_DESC_TRANSFER_DATA;
	if (!end)
		priv->desc_slot++;
	else
		attr |= ADMA_DESC_ATTR_END;

	desc->len = len;
	desc->addr = (u32)buf;
	desc->reserved = 0;
	desc->attr = attr;

	return 0;
}

static int omap_hsmmc_prepare_adma_table(struct mmc *mmc, struct mmc_data *data)
{
	uint total_len = data->blocksize * data->blocks;
	uint desc_count = (total_len / ADMA_MAX_LEN) + 1;
	struct omap_hsmmc_data *priv = (struct omap_hsmmc_data *)mmc->priv;
	int i = desc_count;
	char *buf;

	priv->desc_slot = 0;
	priv->adma_desc_table = (struct omap_hsmmc_adma_desc *)
				memalign(ARCH_DMA_MINALIGN, desc_count *
				sizeof(struct omap_hsmmc_adma_desc));

	if (data->flags & MMC_DATA_READ)
		buf = data->dest;
	else
		buf = (char *)data->src;

	while (--i) {
		omap_hsmmc_adma_desc(mmc, buf, ADMA_MAX_LEN, false);
		buf += ADMA_MAX_LEN;
		total_len -= ADMA_MAX_LEN;
	}

	omap_hsmmc_adma_desc(mmc, buf, total_len, true);

	flush_dcache_range((long)priv->adma_desc_table,
			   (long)priv->adma_desc_table +
			   ROUND(desc_count *
			   sizeof(struct omap_hsmmc_adma_desc),
			   ARCH_DMA_MINALIGN));
	return 0;
}

static void omap_hsmmc_prepare_data(struct mmc *mmc, struct mmc_data *data)
{
	struct hsmmc *mmc_base;
	struct omap_hsmmc_data *priv = (struct omap_hsmmc_data *)mmc->priv;
	u32 val;
	char *buf;

	mmc_base = ((struct omap_hsmmc_data *)mmc->priv)->base_addr;
	omap_hsmmc_prepare_adma_table(mmc, data);

	if (data->flags & MMC_DATA_READ)
		buf = data->dest;
	else
		buf = (char *)data->src;

	val = readl(&mmc_base->hctl);
	val |= DMA_SELECT;
	writel(val, &mmc_base->hctl);

	val = readl(&mmc_base->con);
	val |= DMA_MASTER;
	writel(val, &mmc_base->con);

	writel((u32)priv->adma_desc_table, &mmc_base->admasal);

	/* TODO: This shouldn't be required for read. However I don't seem
	 * to get valid data without this.
	 */
	flush_dcache_range((u32)buf,
			   (u32)buf +
			   ROUND(data->blocksize * data->blocks,
			   ARCH_DMA_MINALIGN));
}

static void omap_hsmmc_dma_cleanup(struct mmc *mmc)
{
	struct hsmmc *mmc_base;
	struct omap_hsmmc_data *priv = (struct omap_hsmmc_data *)mmc->priv;
	u32 val;

	mmc_base = ((struct omap_hsmmc_data *)mmc->priv)->base_addr;

	val = readl(&mmc_base->con);
	val &= ~DMA_MASTER;
	writel(val, &mmc_base->con);

	val = readl(&mmc_base->hctl);
	val &= ~DMA_SELECT;
	writel(val, &mmc_base->hctl);

	kfree(priv->adma_desc_table);
}
#endif

static int omap_hsmmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd,
			struct mmc_data *data)
{
	struct hsmmc *mmc_base;
	unsigned int flags, mmc_stat;
	ulong start;
	struct omap_hsmmc_data *priv = (struct omap_hsmmc_data *)mmc->priv;

	mmc_base = priv->base_addr;

	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
		return 0;

	start = get_timer(0);
	while ((readl(&mmc_base->pstate) & (DATI_MASK | CMDI_MASK)) != 0) {
		if (get_timer(0) - start > MAX_RETRY_MS) {
			printf("%s: timedout waiting on cmd inhibit to clear\n",
					__func__);
			return TIMEOUT;
		}
	}
	writel(0xFFFFFFFF, &mmc_base->stat);
	start = get_timer(0);
	while (readl(&mmc_base->stat)) {
		if (get_timer(0) - start > MAX_RETRY_MS) {
			printf("%s: timedout waiting for STAT (%x) to clear\n",
				__func__, readl(&mmc_base->stat));
			return TIMEOUT;
		}
	}
	/*
	 * CMDREG
	 * CMDIDX[13:8]	: Command index
	 * DATAPRNT[5]	: Data Present Select
	 * ENCMDIDX[4]	: Command Index Check Enable
	 * ENCMDCRC[3]	: Command CRC Check Enable
	 * RSPTYP[1:0]
	 *	00 = No Response
	 *	01 = Length 136
	 *	10 = Length 48
	 *	11 = Length 48 Check busy after response
	 */
	/* Delay added before checking the status of frq change
	 * retry not supported by mmc.c(core file)
	 */
	if (cmd->cmdidx == SD_CMD_APP_SEND_SCR)
		udelay(50000); /* wait 50 ms */

	if (!(cmd->resp_type & MMC_RSP_PRESENT))
		flags = 0;
	else if (cmd->resp_type & MMC_RSP_136)
		flags = RSP_TYPE_LGHT136 | CICE_NOCHECK;
	else if (cmd->resp_type & MMC_RSP_BUSY)
		flags = RSP_TYPE_LGHT48B;
	else
		flags = RSP_TYPE_LGHT48;

	/* enable default flags */
	flags =	flags | (CMD_TYPE_NORMAL | CICE_NOCHECK | CCCE_NOCHECK |
			MSBS_SGLEBLK);
	flags &= ~(ACEN_ENABLE | BCE_ENABLE | DE_ENABLE);

	if (cmd->resp_type & MMC_RSP_CRC)
		flags |= CCCE_CHECK;
	if (cmd->resp_type & MMC_RSP_OPCODE)
		flags |= CICE_CHECK;

	if (data) {
		if ((cmd->cmdidx == MMC_CMD_READ_MULTIPLE_BLOCK) ||
			 (cmd->cmdidx == MMC_CMD_WRITE_MULTIPLE_BLOCK)) {
			flags |= (MSBS_MULTIBLK | BCE_ENABLE | ACEN_ENABLE);
			data->blocksize = 512;
			writel(data->blocksize | (data->blocks << 16),
							&mmc_base->blk);
		} else
			writel(data->blocksize | NBLK_STPCNT, &mmc_base->blk);

		if (data->flags & MMC_DATA_READ)
			flags |= (DP_DATA | DDIR_READ);
		else
			flags |= (DP_DATA | DDIR_WRITE);

#ifdef ADMA_SUPPORT
		if ((priv->controller_flags & OMAP_HSMMC_USE_ADMA) &&
		    cmd->cmdidx != MMC_SEND_TUNING_BLOCK_HS200) {
			omap_hsmmc_prepare_data(mmc, data);
			flags |= DE_ENABLE;
		}
#endif
	}

	mmc_enable_irq(mmc, cmd);

	writel(cmd->cmdarg, &mmc_base->arg);
	udelay(20);		/* To fix "No status update" error on eMMC */
	writel((cmd->cmdidx << 24) | flags, &mmc_base->cmd);

	start = get_timer(0);
	do {
		mmc_stat = readl(&mmc_base->stat);
		if (get_timer(0) - start > MAX_RETRY_MS) {
			printf("%s : timeout: No status update\n", __func__);
			return TIMEOUT;
		}
	} while (!mmc_stat);

	if ((mmc_stat & IE_CTO) != 0) {
		mmc_reset_controller_fsm(mmc_base, SYSCTL_SRC);
		return TIMEOUT;
	} else if ((mmc_stat & ERRI_MASK) != 0)
		return -1;

	if (mmc_stat & CC_MASK) {
		writel(CC_MASK, &mmc_base->stat);
		if (cmd->resp_type & MMC_RSP_PRESENT) {
			if (cmd->resp_type & MMC_RSP_136) {
				/* response type 2 */
				cmd->response[3] = readl(&mmc_base->rsp10);
				cmd->response[2] = readl(&mmc_base->rsp32);
				cmd->response[1] = readl(&mmc_base->rsp54);
				cmd->response[0] = readl(&mmc_base->rsp76);
			} else
				/* response types 1, 1b, 3, 4, 5, 6 */
				cmd->response[0] = readl(&mmc_base->rsp10);
		}
	}

#ifdef ADMA_SUPPORT
	if ((priv->controller_flags & OMAP_HSMMC_USE_ADMA) && data &&
	    cmd->cmdidx != MMC_SEND_TUNING_BLOCK_HS200) {
		if (mmc_stat & IE_ADMAE) {
			omap_hsmmc_dma_cleanup(mmc);
			return -1;
		}

		do {
			mmc_stat = readl(&mmc_base->stat);
			if (mmc_stat & TC_MASK) {
				writel(readl(&mmc_base->stat) | TC_MASK,
				       &mmc_base->stat);
				break;
			}
		} while (1);

		omap_hsmmc_dma_cleanup(mmc);
		return 0;
	}
#endif

	if (data && (data->flags & MMC_DATA_READ)) {
		mmc_read_data(mmc_base,	data->dest,
				data->blocksize * data->blocks);
	} else if (data && (data->flags & MMC_DATA_WRITE)) {
		mmc_write_data(mmc_base, data->src,
				data->blocksize * data->blocks);
	}
	return 0;
}

static int mmc_read_data(struct hsmmc *mmc_base, char *buf, unsigned int size)
{
	unsigned int *output_buf = (unsigned int *)buf;
	unsigned int mmc_stat;
	unsigned int count;

	/*
	 * Start Polled Read
	 */
	count = (size > MMCSD_SECTOR_SIZE) ? MMCSD_SECTOR_SIZE : size;
	count /= 4;

	while (size) {
		ulong start = get_timer(0);
		do {
			mmc_stat = readl(&mmc_base->stat);
			if (get_timer(0) - start > MAX_RETRY_MS) {
				printf("%s: timedout waiting for status!\n",
						__func__);
				return TIMEOUT;
			}
		} while (mmc_stat == 0);

		if ((mmc_stat & (IE_DTO | IE_DCRC | IE_DEB)) != 0)
			mmc_reset_controller_fsm(mmc_base, SYSCTL_SRD);

		if ((mmc_stat & ERRI_MASK) != 0)
			return 1;

		if (mmc_stat & BRR_MASK) {
			unsigned int k;

			writel(readl(&mmc_base->stat) | BRR_MASK,
				&mmc_base->stat);
			for (k = 0; k < count; k++) {
				*output_buf = readl(&mmc_base->data);
				output_buf++;
			}
			size -= (count*4);
		}

		if (mmc_stat & BWR_MASK)
			writel(readl(&mmc_base->stat) | BWR_MASK,
				&mmc_base->stat);

		if (mmc_stat & TC_MASK) {
			writel(readl(&mmc_base->stat) | TC_MASK,
				&mmc_base->stat);
			break;
		}
	}
	return 0;
}

static int mmc_write_data(struct hsmmc *mmc_base, const char *buf,
				unsigned int size)
{
	unsigned int *input_buf = (unsigned int *)buf;
	unsigned int mmc_stat;
	unsigned int count;

	/*
	 * Start Polled Write
	 */
	count = (size > MMCSD_SECTOR_SIZE) ? MMCSD_SECTOR_SIZE : size;
	count /= 4;

	while (size) {
		ulong start = get_timer(0);
		do {
			mmc_stat = readl(&mmc_base->stat);
			if (get_timer(0) - start > MAX_RETRY_MS) {
				printf("%s: timedout waiting for status!\n",
						__func__);
				return TIMEOUT;
			}
		} while (mmc_stat == 0);

		if ((mmc_stat & (IE_DTO | IE_DCRC | IE_DEB)) != 0)
			mmc_reset_controller_fsm(mmc_base, SYSCTL_SRD);

		if ((mmc_stat & ERRI_MASK) != 0)
			return 1;

		if (mmc_stat & BWR_MASK) {
			unsigned int k;

			writel(readl(&mmc_base->stat) | BWR_MASK,
					&mmc_base->stat);
			for (k = 0; k < count; k++) {
				writel(*input_buf, &mmc_base->data);
				input_buf++;
			}
			size -= (count*4);
		}

		if (mmc_stat & BRR_MASK)
			writel(readl(&mmc_base->stat) | BRR_MASK,
				&mmc_base->stat);

		if (mmc_stat & TC_MASK) {
			writel(readl(&mmc_base->stat) | TC_MASK,
				&mmc_base->stat);
			break;
		}
	}
	return 0;
}

static void omap_hsmmc_stop_clock(struct hsmmc *mmc_base)
{
	writel(readl(&mmc_base->sysctl) & ~CEN_ENABLE, &mmc_base->sysctl);
}

static void omap_hsmmc_start_clock(struct hsmmc *mmc_base)
{
	writel(readl(&mmc_base->sysctl) | CEN_ENABLE, &mmc_base->sysctl);
}

static void omap_hsmmc_set_clock(struct mmc *mmc)
{
	struct omap_hsmmc_data *priv_data = mmc->priv;
	struct hsmmc *mmc_base;
	unsigned int dsor = 0;
	ulong start;

	mmc_base = priv_data->base_addr;
	omap_hsmmc_stop_clock(mmc_base);

	/* TODO: Is setting DTO required here? */
	mmc_reg_out(&mmc_base->sysctl, (ICE_MASK | DTO_MASK),
		    (ICE_STOP | DTO_15THDTO));

	if (mmc->clock != 0) {
		dsor = DIV_ROUND_UP(MMC_CLOCK_REFERENCE * 1000000, mmc->clock);
		if (dsor > CLKD_MAX)
			dsor = CLKD_MAX;
	}

	mmc_reg_out(&mmc_base->sysctl, ICE_MASK | CLKD_MASK,
		    (dsor << CLKD_OFFSET) | ICE_OSCILLATE);

	start = get_timer(0);
	while ((readl(&mmc_base->sysctl) & ICS_MASK) == ICS_NOTREADY) {
		if (get_timer(0) - start > MAX_RETRY_MS) {
			printf("%s: timedout waiting for ics!\n", __func__);
			return;
		}
	}

	priv_data->clock = mmc->clock;
	omap_hsmmc_start_clock(mmc_base);
}

static void omap_hsmmc_set_bus_width(struct mmc *mmc)
{
	struct omap_hsmmc_data *priv_data = mmc->priv;
	struct hsmmc *mmc_base;

	mmc_base = priv_data->base_addr;
	/* configue bus width */
	switch (mmc->bus_width) {
	case 8:
		writel(readl(&mmc_base->con) | DTW_8_BITMODE,
			&mmc_base->con);
		break;

	case 4:
		writel(readl(&mmc_base->con) & ~DTW_8_BITMODE,
			&mmc_base->con);
		writel(readl(&mmc_base->hctl) | DTW_4_BITMODE,
			&mmc_base->hctl);
		break;

	case 1:
	default:
		writel(readl(&mmc_base->con) & ~DTW_8_BITMODE,
			&mmc_base->con);
		writel(readl(&mmc_base->hctl) & ~DTW_4_BITMODE,
			&mmc_base->hctl);
		break;
	}

	priv_data->bus_width = mmc->bus_width;
}

static void omap_hsmmc_set_ios(struct mmc *mmc)
{
	struct omap_hsmmc_data *priv_data = mmc->priv;

	if (priv_data->bus_width != mmc->bus_width)
		omap_hsmmc_set_bus_width(mmc);

	if (priv_data->clock != mmc->clock)
		omap_hsmmc_set_clock(mmc);

#ifdef HIGH_SPEED_SUPPORT
	if (priv_data->timing != mmc->timing)
		omap_hsmmc_set_timing(mmc);
#endif
}

#ifdef OMAP_HSMMC_USE_GPIO
static int omap_hsmmc_getcd(struct mmc *mmc)
{
	struct omap_hsmmc_data *priv_data = mmc->priv;
	int cd_gpio;

	/* if no CD return as 1 */
	cd_gpio = priv_data->cd_gpio;
	if (cd_gpio < 0)
		return 1;

	return gpio_get_value(cd_gpio);
}

static int omap_hsmmc_getwp(struct mmc *mmc)
{
	struct omap_hsmmc_data *priv_data = mmc->priv;
	int wp_gpio;

	/* if no WP return as 0 */
	wp_gpio = priv_data->wp_gpio;
	if (wp_gpio < 0)
		return 0;

	return gpio_get_value(wp_gpio);
}
#endif

static const struct mmc_ops omap_hsmmc_ops = {
	.send_cmd	= omap_hsmmc_send_cmd,
	.set_ios	= omap_hsmmc_set_ios,
	.init		= omap_hsmmc_init_setup,
#ifdef OMAP_HSMMC_USE_GPIO
	.getcd		= omap_hsmmc_getcd,
	.getwp		= omap_hsmmc_getwp,
#endif
#ifdef HIGH_SPEED_SUPPORT
	.execute_tuning = omap_hsmmc_execute_tuning,
#endif
};

int omap_mmc_init(int dev_index, uint host_caps_mask, uint f_max, int cd_gpio,
		int wp_gpio)
{
	struct mmc *mmc;
	struct omap_hsmmc_data *priv_data;
	struct mmc_config *cfg;
	uint host_caps_val;

	priv_data = malloc(sizeof(*priv_data));
	if (priv_data == NULL)
		return -1;

	host_caps_val = MMC_MODE_4BIT | MMC_MODE_HS_52MHz | MMC_MODE_HS;

	switch (dev_index) {
	case 0:
		priv_data->base_addr = (struct hsmmc *)OMAP_HSMMC1_BASE;
		break;
#ifdef OMAP_HSMMC2_BASE
	case 1:
		priv_data->base_addr = (struct hsmmc *)OMAP_HSMMC2_BASE;
#if (defined(CONFIG_OMAP44XX) || defined(CONFIG_OMAP54XX) || \
     defined(CONFIG_DRA7XX) || defined(CONFIG_AM57XX)) && \
		defined(CONFIG_HSMMC2_8BIT)
		/* Enable 8-bit interface for eMMC on OMAP4/5 or DRA7XX */
		host_caps_val |= MMC_MODE_8BIT;
#endif
#if defined(CONFIG_DRA7XX)
		if (is_dra7xx() || is_dra72x())
			host_caps_val |= MMC_MODE_HS200 | MMC_MODE_DDR_52MHz;
#endif
		break;
#endif
#ifdef OMAP_HSMMC3_BASE
	case 2:
		priv_data->base_addr = (struct hsmmc *)OMAP_HSMMC3_BASE;
#if (defined(CONFIG_DRA7XX) || defined(CONFIG_AM57XX)) && defined(CONFIG_HSMMC3_8BIT)
		/* Enable 8-bit interface for eMMC on DRA7XX */
		host_caps_val |= MMC_MODE_8BIT;
#endif
		break;
#endif
	default:
		priv_data->base_addr = (struct hsmmc *)OMAP_HSMMC1_BASE;
		return 1;
	}

#ifndef CONFIG_NO_HSMMC_DUALT_VOLT
	priv_data->controller_flags |= OMAP_HSMMC_SUPPORTS_DUAL_VOLT;
#endif

	switch (dev_index) {
	case 0:
		priv_data->iov = CONFIG_HSMMC1_IOV;
		break;
	case 1:
		priv_data->iov = CONFIG_HSMMC2_IOV;
		break;
	case 2:
		priv_data->iov = CONFIG_HSMMC3_IOV;
		break;
	}

#ifdef OMAP_HSMMC_USE_GPIO
	/* on error gpio values are set to -1, which is what we want */
	priv_data->cd_gpio = omap_mmc_setup_gpio_in(cd_gpio, "mmc_cd");
	priv_data->wp_gpio = omap_mmc_setup_gpio_in(wp_gpio, "mmc_wp");
#endif

	cfg = &priv_data->cfg;

	cfg->name = "OMAP SD/MMC";
	cfg->ops = &omap_hsmmc_ops;

	cfg->voltages = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;
	cfg->host_caps = host_caps_val & ~host_caps_mask;

	cfg->f_min = 400000;

	if (f_max != 0)
		cfg->f_max = f_max;
	else {
		if (cfg->host_caps & MMC_MODE_HS) {
			if (cfg->host_caps & MMC_MODE_HS200)
				cfg->f_max = 200000000;
			else if (cfg->host_caps & MMC_MODE_HS_52MHz)
				cfg->f_max = 52000000;
			else
				cfg->f_max = 26000000;
		} else
			cfg->f_max = 20000000;
		cfg->f_max = min(cfg->f_max, MMC_CLOCK_REFERENCE * 1000000);
	}

	cfg->b_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;

#if defined(CONFIG_OMAP34XX)
	/*
	 * Silicon revs 2.1 and older do not support multiblock transfers.
	 */
	if ((get_cpu_family() == CPU_OMAP34XX) && (get_cpu_rev() <= CPU_3XX_ES21))
		cfg->b_max = 1;
#endif
	mmc = mmc_create(cfg, priv_data);
	if (mmc == NULL)
		return -1;

	omap_hsmmc_platform_fixup(mmc);
	return 0;
}

__weak int platform_fixup_disable_uhs_mode(void)
{
	return 0;
}

static int omap_hsmmc_platform_fixup(struct mmc *mmc)
{
	struct omap_hsmmc_data *priv = (struct omap_hsmmc_data *)mmc->priv;
	struct mmc_config *cfg = &priv->cfg;

	if (platform_fixup_disable_uhs_mode())
		cfg->host_caps &= ~(MMC_MODE_DDR_52MHz | MMC_MODE_HS200);

	return 0;
}
