// SPDX-License-Identifier: GPL-2.0+
/*
 *  Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com
 *  Author: Peter Ujfalusi <peter.ujfalusi@ti.com>
 */
#define pr_fmt(fmt) "udma: " fmt

#include <common.h>
#include <asm/io.h>
#include <asm/bitops.h>
#include <malloc.h>
#include <asm/dma-mapping.h>
#include <dm.h>
#include <dm/read.h>
#include <dm/of_access.h>
#include <dma.h>
#include <dma-uclass.h>
#include <linux/delay.h>
#include <linux/soc/ti/k3-navss-psilcfg.h>
#include <dt-bindings/dma/ti-udma.h>
#include <linux/soc/ti/k3-navss-ringacc.h>
#include <linux/soc/ti/cppi5.h>
#include <linux/soc/ti/ti-udma.h>
#include <linux/soc/ti/ti_sci_protocol.h>

#include "k3-udma-hwdef.h"

#define RINGACC_RING_USE_PROXY (0)

struct udma_chan;

enum udma_mmr {
	MMR_GCFG = 0,
	MMR_RCHANRT,
	MMR_TCHANRT,
	MMR_LAST,
};

static const char * const mmr_names[] = {
	"gcfg", "rchanrt", "tchanrt"
};

struct udma_tchan {
	void __iomem *reg_rt;

	int id;
	struct k3_nav_ring *t_ring; /* Transmit ring */
	struct k3_nav_ring *tc_ring; /* Transmit Completion ring */
};

struct udma_rchan {
	void __iomem *reg_rt;

	int id;
	struct k3_nav_ring *fd_ring; /* Free Descriptor ring */
	struct k3_nav_ring *r_ring; /* Receive ring*/
};

struct udma_rflow {
	int id;
};

struct udma_dev {
	struct device *dev;
	void __iomem *mmrs[MMR_LAST];

	struct udevice *psil_node;
	struct k3_nav_ringacc *ringacc;

	u32 features;

	int tchan_cnt;
	int echan_cnt;
	int rchan_cnt;
	int rflow_cnt;
	unsigned long *tchan_map;
	unsigned long *rchan_map;
	unsigned long *rflow_map;

	struct udma_tchan *tchans;
	struct udma_rchan *rchans;
	struct udma_rflow *rflows;

	struct udma_chan *channels;
	u32 psil_base;

	u32 ch_count;
	const struct ti_sci_handle *tisci;
	const struct ti_sci_rm_udmap_ops *tisci_udmap_ops;
	u32  tisci_dev_id;
};

struct udma_chan {
	struct udma_dev *ud;
	char name[20];

	struct udma_tchan *tchan;
	struct udma_rchan *rchan;
	struct udma_rflow *rflow;

	struct k3_nav_psil_entry *psi_link;

	u32 bcnt; /* number of bytes completed since the start of the channel */

	bool pkt_mode; /* TR or packet */
	bool needs_epib; /* EPIB is needed for the communication or not */
	u32 psd_size; /* size of Protocol Specific Data */
	u32 metadata_size; /* (needs_epib ? 16:0) + psd_size */
	int slave_thread_id;
	u32 src_thread;
	u32 dst_thread;
	u32 static_tr_type;

	u32 id;
	enum dma_direction dir;

	struct knav_udmap_host_desc_t *desc_tx;
	u32 hdesc_size;
	bool in_use;
	void	*desc_rx;
	void	*buf_rx;
};

#define UDMA_CH_1000(ch)		(ch * 0x1000)
#define UDMA_CH_100(ch)			(ch * 0x100)
#define UDMA_CH_40(ch)			(ch * 0x40)

#ifdef PKTBUFSRX
#define UDMA_TX_DESC_NUM PKTBUFSRX
#else
#define UDMA_TX_DESC_NUM 4
#endif

#ifdef PKTSIZE_ALIGN
#define UDMA_TX_BUF_SIZE PKTSIZE_ALIGN
#else
#define UDMA_TX_BUF_SIZE ALIGN(1522, ARCH_DMA_MINALIGN)
#endif

#ifdef K3_UDMA_DEBUG
#define	k3_udma_dbg(arg...) pr_err(arg)
#define	k3_udma_dev_dbg(dev, arg...) dev_err(dev, arg)
static void k3_udma_print_buf(ulong addr, const void *data, uint width,
			      uint count, uint linelen)
{
	print_buffer(addr, data, width, count, linelen);
}
#else
#define	k3_udma_dbg(arg...)
#define	k3_udma_dev_dbg(arg...)
static void k3_udma_print_buf(ulong addr, const void *data, uint width,
			      uint count, uint linelen)
{}
#endif

/* Generic register access functions */
static inline u32 udma_read(void __iomem *base, int reg)
{
	u32 v;

	v = __raw_readl(base + reg);
	k3_udma_dbg("READL(32): v(%08X)<--reg(%p)\n", v, base + reg);
	return v;
}

static inline void udma_write(void __iomem *base, int reg, u32 val)
{
	k3_udma_dbg("WRITEL(32): v(%08X)-->reg(%p)\n", val, base + reg);
	__raw_writel(val, base + reg);
}

static inline void udma_update_bits(void __iomem *base, int reg,
				    u32 mask, u32 val)
{
	u32 tmp, orig;

	orig = udma_read(base, reg);
	tmp = orig & ~mask;
	tmp |= (val & mask);

	if (tmp != orig)
		udma_write(base, reg, tmp);
}

/* TCHANRT */
static inline u32 udma_tchanrt_read(struct udma_tchan *tchan, int reg)
{
	if (!tchan)
		return 0;
	return udma_read(tchan->reg_rt, reg);
}

static inline void udma_tchanrt_write(struct udma_tchan *tchan,
				      int reg, u32 val)
{
	if (!tchan)
		return;
	udma_write(tchan->reg_rt, reg, val);
}

/* RCHANRT */
static inline u32 udma_rchanrt_read(struct udma_rchan *rchan, int reg)
{
	if (!rchan)
		return 0;
	return udma_read(rchan->reg_rt, reg);
}

static inline void udma_rchanrt_write(struct udma_rchan *rchan,
				      int reg, u32 val)
{
	if (!rchan)
		return;
	udma_write(rchan->reg_rt, reg, val);
}

static inline char *udma_get_dir_text(enum dma_direction dir)
{
	switch (dir) {
	case DMA_DEV_TO_MEM:
		return "DEV_TO_MEM";
	case DMA_MEM_TO_DEV:
		return "MEM_TO_DEV";
	case DMA_MEM_TO_MEM:
		return "MEM_TO_MEM";
	case DMA_DEV_TO_DEV:
		return "DEV_TO_DEV";
	default:
		break;
	}

	return "invalid";
}

static inline bool udma_is_chan_running(struct udma_chan *uc)
{
	u32 trt_ctl = 0;
	u32 rrt_ctl = 0;

	switch (uc->dir) {
	case DMA_DEV_TO_MEM:
		rrt_ctl = udma_rchanrt_read(uc->rchan, UDMA_RCHAN_RT_CTL_REG);
		k3_udma_dbg("%s: rrt_ctl: 0x%08x (peer: 0x%08x)\n",
			    __func__, rrt_ctl,
			    udma_rchanrt_read(uc->rchan,
					      UDMA_RCHAN_RT_PEER_RT_EN_REG));
		break;
	case DMA_MEM_TO_DEV:
		trt_ctl = udma_tchanrt_read(uc->tchan, UDMA_TCHAN_RT_CTL_REG);
		k3_udma_dbg("%s: trt_ctl: 0x%08x (peer: 0x%08x)\n",
			    __func__, trt_ctl,
			    udma_tchanrt_read(uc->tchan,
					      UDMA_TCHAN_RT_PEER_RT_EN_REG));
		break;
	case DMA_MEM_TO_MEM:
		trt_ctl = udma_tchanrt_read(uc->tchan, UDMA_TCHAN_RT_CTL_REG);
		rrt_ctl = udma_rchanrt_read(uc->rchan, UDMA_RCHAN_RT_CTL_REG);
		break;
	default:
		break;
	}

	if (trt_ctl & UDMA_CHAN_RT_CTL_EN || rrt_ctl & UDMA_CHAN_RT_CTL_EN)
		return true;

	return false;
}

static int udma_pop_from_ring(struct udma_chan *uc, dma_addr_t *addr)
{
	struct k3_nav_ring *ring = NULL;
	int ret = -ENOENT;

	switch (uc->dir) {
	case DMA_DEV_TO_MEM:
		ring = uc->rchan->r_ring;
		break;
	case DMA_MEM_TO_DEV:
		ring = uc->tchan->tc_ring;
		break;
	case DMA_MEM_TO_MEM:
		ring = uc->rchan->r_ring;
		break;
	default:
		break;
	}

	if (ring && k3_nav_ringacc_ring_get_occ(ring))
		ret = k3_nav_ringacc_ring_pop(ring, addr);

	return ret;
}

static void udma_reset_rings(struct udma_chan *uc)
{
	struct k3_nav_ring *ring1 = NULL;
	struct k3_nav_ring *ring2 = NULL;

	switch (uc->dir) {
	case DMA_DEV_TO_MEM:
		ring1 = uc->rchan->fd_ring;
		ring2 = uc->rchan->r_ring;
		break;
	case DMA_MEM_TO_DEV:
		ring1 = uc->tchan->t_ring;
		ring2 = uc->tchan->tc_ring;
		break;
	case DMA_MEM_TO_MEM:
		ring1 = uc->tchan->t_ring;
		ring2 = uc->tchan->tc_ring;
		break;
	default:
		break;
	}

	if (ring1)
		k3_nav_ringacc_ring_reset_dma(ring1, 0);
	if (ring2)
		k3_nav_ringacc_ring_reset(ring2);
}

static void udma_reset_counters(struct udma_chan *uc)
{
	u32 val;

	if (uc->tchan) {
		val = udma_tchanrt_read(uc->tchan, UDMA_TCHAN_RT_BCNT_REG);
		udma_tchanrt_write(uc->tchan, UDMA_TCHAN_RT_BCNT_REG, val);

		val = udma_tchanrt_read(uc->tchan, UDMA_TCHAN_RT_SBCNT_REG);
		udma_tchanrt_write(uc->tchan, UDMA_TCHAN_RT_SBCNT_REG, val);

		val = udma_tchanrt_read(uc->tchan, UDMA_TCHAN_RT_PCNT_REG);
		udma_tchanrt_write(uc->tchan, UDMA_TCHAN_RT_PCNT_REG, val);

		val = udma_tchanrt_read(uc->tchan, UDMA_TCHAN_RT_PEER_BCNT_REG);
		udma_tchanrt_write(uc->tchan, UDMA_TCHAN_RT_PEER_BCNT_REG, val);
	}

	if (uc->rchan) {
		val = udma_rchanrt_read(uc->rchan, UDMA_RCHAN_RT_BCNT_REG);
		udma_rchanrt_write(uc->rchan, UDMA_RCHAN_RT_BCNT_REG, val);

		val = udma_rchanrt_read(uc->rchan, UDMA_RCHAN_RT_SBCNT_REG);
		udma_rchanrt_write(uc->rchan, UDMA_RCHAN_RT_SBCNT_REG, val);

		val = udma_rchanrt_read(uc->rchan, UDMA_RCHAN_RT_PCNT_REG);
		udma_rchanrt_write(uc->rchan, UDMA_RCHAN_RT_PCNT_REG, val);

		val = udma_rchanrt_read(uc->rchan, UDMA_RCHAN_RT_PEER_BCNT_REG);
		udma_rchanrt_write(uc->rchan, UDMA_RCHAN_RT_PEER_BCNT_REG, val);
	}

	uc->bcnt = 0;
}

static inline int udma_stop_hard(struct udma_chan *uc)
{
	k3_udma_dbg("%s: ENTER (chan%d)\n", __func__, uc->id);

	switch (uc->dir) {
	case DMA_DEV_TO_MEM:
		udma_rchanrt_write(uc->rchan, UDMA_RCHAN_RT_PEER_RT_EN_REG, 0);
		udma_rchanrt_write(uc->rchan, UDMA_RCHAN_RT_CTL_REG, 0);
		break;
	case DMA_MEM_TO_DEV:
		udma_tchanrt_write(uc->tchan, UDMA_TCHAN_RT_CTL_REG, 0);
		udma_tchanrt_write(uc->tchan, UDMA_TCHAN_RT_PEER_RT_EN_REG, 0);
		break;
	case DMA_MEM_TO_MEM:
		udma_rchanrt_write(uc->rchan, UDMA_RCHAN_RT_CTL_REG, 0);
		udma_tchanrt_write(uc->tchan, UDMA_TCHAN_RT_CTL_REG, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int udma_start(struct udma_chan *uc)
{
	/* Channel is already running, no need to proceed further */
	if (udma_is_chan_running(uc))
		goto out;

	k3_udma_dbg("%s: chan:%d dir:%s (static_tr_type: %d)\n",
		    __func__, uc->id, udma_get_dir_text(uc->dir),
		    uc->static_tr_type);

	/* Make sure that we clear the teardown bit, if it is set */
	udma_stop_hard(uc);
	/* Reset the rings for a new start */
	udma_reset_rings(uc);

	/* Reset all counters */
	udma_reset_counters(uc);

	switch (uc->dir) {
	case DMA_DEV_TO_MEM:
		udma_rchanrt_write(uc->rchan, UDMA_RCHAN_RT_CTL_REG,
				   UDMA_CHAN_RT_CTL_EN);

		/* Enable remote */
		udma_rchanrt_write(uc->rchan, UDMA_RCHAN_RT_PEER_RT_EN_REG,
				   UDMA_PEER_RT_EN_ENABLE);

		k3_udma_dbg("%s(rx): RT_CTL:0x%08x PEER RT_ENABLE:0x%08x\n",
			    __func__,
			    udma_rchanrt_read(uc->rchan,
					      UDMA_RCHAN_RT_CTL_REG),
			    udma_rchanrt_read(uc->rchan,
					      UDMA_RCHAN_RT_PEER_RT_EN_REG));
		break;
	case DMA_MEM_TO_DEV:
		/* Enable remote */
		udma_tchanrt_write(uc->tchan, UDMA_TCHAN_RT_PEER_RT_EN_REG,
				   UDMA_PEER_RT_EN_ENABLE);

		udma_tchanrt_write(uc->tchan, UDMA_TCHAN_RT_CTL_REG,
				   UDMA_CHAN_RT_CTL_EN);

		k3_udma_dbg("%s(tx): RT_CTL:0x%08x PEER RT_ENABLE:0x%08x\n",
			    __func__,
			    udma_rchanrt_read(uc->rchan,
					      UDMA_TCHAN_RT_CTL_REG),
			    udma_rchanrt_read(uc->rchan,
					      UDMA_TCHAN_RT_PEER_RT_EN_REG));
		break;
	case DMA_MEM_TO_MEM:
		udma_rchanrt_write(uc->rchan, UDMA_RCHAN_RT_CTL_REG,
				   UDMA_CHAN_RT_CTL_EN);
		udma_tchanrt_write(uc->tchan, UDMA_TCHAN_RT_CTL_REG,
				   UDMA_CHAN_RT_CTL_EN);

		break;
	default:
		return -EINVAL;
	}

	k3_udma_dbg("%s: DONE chan:%d\n", __func__, uc->id);
out:
	return 0;
}

static inline void udma_stop_mem2dev(struct udma_chan *uc, bool sync)
{
	int i = 0;
	u32 val;

	udma_tchanrt_write(uc->tchan, UDMA_TCHAN_RT_CTL_REG,
			   UDMA_CHAN_RT_CTL_EN |
			   UDMA_CHAN_RT_CTL_TDOWN);

	val = udma_tchanrt_read(uc->tchan, UDMA_TCHAN_RT_CTL_REG);

	while (sync && (val & UDMA_CHAN_RT_CTL_EN)) {
		val = udma_tchanrt_read(uc->tchan, UDMA_TCHAN_RT_CTL_REG);
		udelay(1);
		if (i > 1000) {
			printf(" %s TIMEOUT !\n", __func__);
			break;
		}
		i++;
	}

	val = udma_tchanrt_read(uc->tchan, UDMA_TCHAN_RT_PEER_RT_EN_REG);
	if (val & UDMA_PEER_RT_EN_ENABLE)
		printf("%s: peer not stopped TIMEOUT !\n", __func__);
}

static inline void udma_stop_dev2mem(struct udma_chan *uc, bool sync)
{
	int i = 0;
	u32 val;

	udma_rchanrt_write(uc->rchan, UDMA_RCHAN_RT_PEER_RT_EN_REG,
			   UDMA_PEER_RT_EN_ENABLE |
			   UDMA_PEER_RT_EN_TEARDOWN);

	val = udma_rchanrt_read(uc->rchan, UDMA_RCHAN_RT_CTL_REG);

	while (sync && (val & UDMA_CHAN_RT_CTL_EN)) {
		val = udma_rchanrt_read(uc->rchan, UDMA_RCHAN_RT_CTL_REG);
		udelay(1);
		if (i > 1000) {
			printf("%s TIMEOUT !\n", __func__);
			break;
		}
		i++;
	}

	val = udma_rchanrt_read(uc->rchan, UDMA_RCHAN_RT_PEER_RT_EN_REG);
	if (val & UDMA_PEER_RT_EN_ENABLE)
		printf("%s: peer not stopped TIMEOUT !\n", __func__);
}

static inline int udma_stop(struct udma_chan *uc)
{
	k3_udma_dbg("%s: chan:%d dir:%s\n",
		    __func__, uc->id, udma_get_dir_text(uc->dir));

	switch (uc->dir) {
	case DMA_DEV_TO_MEM:
		udma_stop_dev2mem(uc, true);
		break;
	case DMA_MEM_TO_DEV:
		udma_stop_mem2dev(uc, true);
		break;
	case DMA_MEM_TO_MEM:
		udma_rchanrt_write(uc->rchan, UDMA_RCHAN_RT_CTL_REG, 0);
		udma_tchanrt_write(uc->tchan, UDMA_TCHAN_RT_CTL_REG, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void udma_poll_completion(struct udma_chan *uc, dma_addr_t *paddr)
{
	int i = 1;

	while (udma_pop_from_ring(uc, paddr)) {
		udelay(1);
		if (!(i % 1000000))
			printf(".");
		i++;
	}
}

#define UDMA_RESERVE_RESOURCE(res)					\
static struct udma_##res *__udma_reserve_##res(struct udma_dev *ud,	\
					       int id)			\
{									\
	if (id >= 0) {							\
		if (test_bit(id, ud->res##_map)) {			\
			dev_err(ud->dev, "res##%d is in use\n", id);	\
			return ERR_PTR(-ENOENT);			\
		}							\
	} else {							\
		id = find_first_zero_bit(ud->res##_map, ud->res##_cnt); \
		if (id == ud->res##_cnt) {				\
			return ERR_PTR(-ENOENT);			\
		}							\
	}								\
									\
	__set_bit(id, ud->res##_map);					\
	return &ud->res##s[id];						\
}

UDMA_RESERVE_RESOURCE(tchan);
UDMA_RESERVE_RESOURCE(rchan);
UDMA_RESERVE_RESOURCE(rflow);

static int udma_get_tchan(struct udma_chan *uc)
{
	struct udma_dev *ud = uc->ud;

	if (uc->tchan) {
		dev_dbg(ud->dev, "chan%d: already have tchan%d allocated\n",
			uc->id, uc->tchan->id);
		return 0;
	}

	uc->tchan = __udma_reserve_tchan(ud, -1);
	if (IS_ERR(uc->tchan))
		return PTR_ERR(uc->tchan);

	k3_udma_dbg("chan%d: got tchan%d\n", uc->id, uc->tchan->id);

	if (udma_is_chan_running(uc)) {
		dev_warn(ud->dev, "chan%d: tchan%d is running!\n", uc->id,
			 uc->tchan->id);
		udma_stop(uc);
		if (udma_is_chan_running(uc))
			dev_err(ud->dev, "chan%d: won't stop!\n", uc->id);
	}

	return 0;
}

static int udma_get_rchan(struct udma_chan *uc)
{
	struct udma_dev *ud = uc->ud;

	if (uc->rchan) {
		dev_dbg(ud->dev, "chan%d: already have rchan%d allocated\n",
			uc->id, uc->rchan->id);
		return 0;
	}

	uc->rchan = __udma_reserve_rchan(ud, -1);
	if (IS_ERR(uc->rchan))
		return PTR_ERR(uc->rchan);

	k3_udma_dbg("chan%d: got rchan%d\n", uc->id, uc->rchan->id);

	if (udma_is_chan_running(uc)) {
		dev_warn(ud->dev, "chan%d: rchan%d is running!\n", uc->id,
			 uc->rchan->id);
		udma_stop(uc);
		if (udma_is_chan_running(uc))
			dev_err(ud->dev, "chan%d: won't stop!\n", uc->id);
	}

	return 0;
}

static int udma_get_chan_pair(struct udma_chan *uc)
{
	struct udma_dev *ud = uc->ud;
	int chan_id, end;

	if ((uc->tchan && uc->rchan) && uc->tchan->id == uc->rchan->id) {
		dev_info(ud->dev, "chan%d: already have %d pair allocated\n",
			 uc->id, uc->tchan->id);
		return 0;
	}

	if (uc->tchan) {
		dev_err(ud->dev, "chan%d: already have tchan%d allocated\n",
			uc->id, uc->tchan->id);
		return -EBUSY;
	} else if (uc->rchan) {
		dev_err(ud->dev, "chan%d: already have rchan%d allocated\n",
			uc->id, uc->rchan->id);
		return -EBUSY;
	}

	/* Can be optimized, but let's have it like this for now */
	end = min(ud->tchan_cnt, ud->rchan_cnt);
	for (chan_id = 0; chan_id < end; chan_id++) {
		if (!test_bit(chan_id, ud->tchan_map) &&
		    !test_bit(chan_id, ud->rchan_map))
			break;
	}

	if (chan_id == end)
		return -ENOENT;

	__set_bit(chan_id, ud->tchan_map);
	__set_bit(chan_id, ud->rchan_map);
	uc->tchan = &ud->tchans[chan_id];
	uc->rchan = &ud->rchans[chan_id];

	k3_udma_dbg("chan%d: got t/rchan%d pair\n", uc->id, chan_id);

	if (udma_is_chan_running(uc)) {
		dev_warn(ud->dev, "chan%d: t/rchan%d pair is running!\n",
			 uc->id, chan_id);
		udma_stop(uc);
		if (udma_is_chan_running(uc))
			dev_err(ud->dev, "chan%d: won't stop!\n", uc->id);
	}

	return 0;
}

static int udma_get_rflow(struct udma_chan *uc, int flow_id)
{
	struct udma_dev *ud = uc->ud;

	if (uc->rflow) {
		dev_dbg(ud->dev, "chan%d: already have rflow%d allocated\n",
			uc->id, uc->rflow->id);
		return 0;
	}

	if (!uc->rchan)
		dev_warn(ud->dev, "chan%d: does not have rchan??\n", uc->id);

	uc->rflow = __udma_reserve_rflow(ud, flow_id);
	if (IS_ERR(uc->rflow))
		return PTR_ERR(uc->rflow);

	k3_udma_dbg("chan%d: got rflow%d\n", uc->id, uc->rflow->id);
	return 0;
}

static void udma_put_rchan(struct udma_chan *uc)
{
	struct udma_dev *ud = uc->ud;

	if (uc->rchan) {
		dev_dbg(ud->dev, "chan%d: put rchan%d\n", uc->id,
			uc->rchan->id);
		__clear_bit(uc->rchan->id, ud->rchan_map);
		uc->rchan = NULL;
	}
}

static void udma_put_tchan(struct udma_chan *uc)
{
	struct udma_dev *ud = uc->ud;

	if (uc->tchan) {
		dev_dbg(ud->dev, "chan%d: put tchan%d\n", uc->id,
			uc->tchan->id);
		__clear_bit(uc->tchan->id, ud->tchan_map);
		uc->tchan = NULL;
	}
}

static void udma_put_rflow(struct udma_chan *uc)
{
	struct udma_dev *ud = uc->ud;

	if (uc->rflow) {
		dev_dbg(ud->dev, "chan%d: put rflow%d\n", uc->id,
			uc->rflow->id);
		__clear_bit(uc->rflow->id, ud->rflow_map);
		uc->rflow = NULL;
	}
}

static void udma_free_tx_resources(struct udma_chan *uc)
{
	if (!uc->tchan)
		return;

	k3_nav_ringacc_ring_free(uc->tchan->t_ring);
	k3_nav_ringacc_ring_free(uc->tchan->tc_ring);
	uc->tchan->t_ring = NULL;
	uc->tchan->tc_ring = NULL;

	udma_put_tchan(uc);
}

static int udma_alloc_tx_resources(struct udma_chan *uc)
{
	struct k3_nav_ring_cfg ring_cfg;
	struct udma_dev *ud = uc->ud;
	int ret;

	ret = udma_get_tchan(uc);
	if (ret)
		return ret;

	uc->tchan->t_ring = k3_nav_ringacc_request_ring(
				ud->ringacc, uc->tchan->id,
				RINGACC_RING_USE_PROXY);
	if (!uc->tchan->t_ring) {
		ret = -EBUSY;
		goto err_tx_ring;
	}

	uc->tchan->tc_ring = k3_nav_ringacc_request_ring(
				ud->ringacc, -1, RINGACC_RING_USE_PROXY);
	if (!uc->tchan->tc_ring) {
		ret = -EBUSY;
		goto err_txc_ring;
	}

	memset(&ring_cfg, 0, sizeof(ring_cfg));
	ring_cfg.size = 16;
	ring_cfg.elm_size = K3_NAV_RINGACC_RING_ELSIZE_8;
	ring_cfg.mode = K3_NAV_RINGACC_RING_MODE_MESSAGE;

	ret = k3_nav_ringacc_ring_cfg(uc->tchan->t_ring, &ring_cfg);
	ret |= k3_nav_ringacc_ring_cfg(uc->tchan->tc_ring, &ring_cfg);

	if (ret)
		goto err_ringcfg;

	return 0;

err_ringcfg:
	k3_nav_ringacc_ring_free(uc->tchan->tc_ring);
	uc->tchan->tc_ring = NULL;
err_txc_ring:
	k3_nav_ringacc_ring_free(uc->tchan->t_ring);
	uc->tchan->t_ring = NULL;
err_tx_ring:
	udma_put_tchan(uc);

	return ret;
}

static void udma_free_rx_resources(struct udma_chan *uc)
{
	if (!uc->rchan)
		return;

	k3_nav_ringacc_ring_free(uc->rchan->fd_ring);
	k3_nav_ringacc_ring_free(uc->rchan->r_ring);
	uc->rchan->fd_ring = NULL;
	uc->rchan->r_ring = NULL;

	udma_put_rflow(uc);
	udma_put_rchan(uc);
}

static int udma_alloc_rx_resources(struct udma_chan *uc)
{
	struct k3_nav_ring_cfg ring_cfg;
	struct udma_dev *ud = uc->ud;
	int fd_ring_id;
	int ret;

	ret = udma_get_rchan(uc);
	if (ret)
		return ret;

	ret = udma_get_rflow(uc, uc->rchan->id);
	if (ret) {
		ret = -EBUSY;
		goto err_rflow;
	}

	if (uc->dir == DMA_MEM_TO_MEM)
		fd_ring_id = -1;
	else
		fd_ring_id = ud->tchan_cnt + ud->echan_cnt + uc->rchan->id;

	uc->rchan->fd_ring = k3_nav_ringacc_request_ring(
				ud->ringacc, fd_ring_id,
				RINGACC_RING_USE_PROXY);
	if (!uc->rchan->fd_ring) {
		ret = -EBUSY;
		goto err_rx_ring;
	}

	uc->rchan->r_ring = k3_nav_ringacc_request_ring(
				ud->ringacc, -1, RINGACC_RING_USE_PROXY);
	if (!uc->rchan->r_ring) {
		ret = -EBUSY;
		goto err_rxc_ring;
	}

	memset(&ring_cfg, 0, sizeof(ring_cfg));
	ring_cfg.size = 16;
	ring_cfg.elm_size = K3_NAV_RINGACC_RING_ELSIZE_8;
	ring_cfg.mode = K3_NAV_RINGACC_RING_MODE_MESSAGE;

	ret = k3_nav_ringacc_ring_cfg(uc->rchan->fd_ring, &ring_cfg);
	ret |= k3_nav_ringacc_ring_cfg(uc->rchan->r_ring, &ring_cfg);

	if (ret)
		goto err_ringcfg;

	return 0;

err_ringcfg:
	k3_nav_ringacc_ring_free(uc->rchan->r_ring);
	uc->rchan->r_ring = NULL;
err_rxc_ring:
	k3_nav_ringacc_ring_free(uc->rchan->fd_ring);
	uc->rchan->fd_ring = NULL;
err_rx_ring:
	udma_put_rflow(uc);
err_rflow:
	udma_put_rchan(uc);

	return ret;
}

static int udma_alloc_tchan_sci_req(struct udma_chan *uc)
{
	struct udma_dev *ud = uc->ud;
	int tc_ring = k3_nav_ringacc_get_ring_id(uc->tchan->tc_ring);
	struct ti_sci_rm_udmap_tx_ch_alloc req;
	u32 ch_index, mode;
	int ret;

	if (uc->pkt_mode)
		mode = TI_SCI_RM_UDMAP_CHAN_TYPE_PKT_PBRR;
	else
		mode = TI_SCI_RM_UDMAP_CHAN_TYPE_3RDP_PBRR;

	req.secondary_host = TI_SCI_RM_NULL_U8;
	req.nav_id = ud->tisci_dev_id;
	req.index = uc->tchan->id;
	req.tx_pause_on_err = 0;
	req.tx_filt_einfo = 0;
	req.tx_filt_pswords = 0;
	req.tx_atype = 0;
	req.tx_chan_type = mode;
	req.tx_supr_tdpkt = 0;
	req.tx_fetch_size = 16;
	req.tx_credit_count = 0;
	req.txcq_qnum = tc_ring;
	req.tx_priority = TI_SCI_RM_NULL_U8;
	req.tx_qos = TI_SCI_RM_NULL_U8;
	req.tx_orderid = TI_SCI_RM_NULL_U8;
	req.fdepth = 0x80;
	req.tx_sched_priority = 0;
	req.share = 0;
	req.type = TI_SCI_RM_NULL_U8;

	ret = ud->tisci_udmap_ops->tx_ch_alloc(ud->tisci, &req,
					       &ch_index);
	if (ret) {
		dev_err(ud->dev, "tisci tx alloc failed %d\n", ret);
		return ret;
	}

	if (uc->tchan->id != ch_index) {
		ud->tisci_udmap_ops->tx_ch_free(
				ud->tisci, -1,
				ud->tisci_dev_id,
				uc->tchan->id);
		dev_err(ud->dev, "tx channel mismatch %u vs %u\n",
			ch_index, uc->tchan->id);
		return -EINVAL;
	}

	return 0;
}

static int udma_alloc_rchan_sci_req(struct udma_chan *uc)
{
	struct udma_dev *ud = uc->ud;
	int fd_ring = k3_nav_ringacc_get_ring_id(uc->rchan->fd_ring);
	int rx_ring = k3_nav_ringacc_get_ring_id(uc->rchan->r_ring);
	struct ti_sci_rm_udmap_rx_ch_alloc req;
	struct ti_sci_rm_udmap_rx_flow_cfg flow_req;
	u32 def_flow_index, rng_flow_start_index, rng_flow_cnt;
	u32 ch_index, mode;
	int ret;

	if (uc->pkt_mode)
		mode = TI_SCI_RM_UDMAP_CHAN_TYPE_PKT_PBRR;
	else
		mode = TI_SCI_RM_UDMAP_CHAN_TYPE_3RDP_PBRR;

	req.secondary_host = TI_SCI_RM_NULL_U8;
	req.nav_id = ud->tisci_dev_id;
	req.index = uc->rchan->id;
	req.rx_fetch_size = 16;
	req.rxcq_qnum = rx_ring;
	req.rx_priority = TI_SCI_RM_NULL_U8;
	req.rx_qos = TI_SCI_RM_NULL_U8;
	req.rx_orderid = TI_SCI_RM_NULL_U8;
	req.rx_sched_priority = 0;
	req.flowid_start = TI_SCI_RM_NULL_U16;
	req.flowid_cnt = 0;
	req.rx_pause_on_err = 0;
	req.rx_atype = 0;
	req.rx_chan_type = mode;
	req.rx_ignore_short = 0;
	req.rx_ignore_long = 0;
	req.share = 0;
	req.type = TI_SCI_RM_NULL_U8;

	ret = ud->tisci_udmap_ops->rx_ch_alloc(
			ud->tisci,
			&req, &ch_index, &def_flow_index,
			&rng_flow_start_index, &rng_flow_cnt);
	if (ret) {
		dev_err(ud->dev, "tisci rx alloc failed %d\n", ret);
		return ret;
	}

	if (uc->rchan->id != ch_index) {
		dev_err(ud->dev, "rx channel mismatch %u vs %u\n",
			ch_index, uc->rchan->id);
		goto rchan_free;
	}

	k3_udma_dbg("tisci rx gave us: ch_index: %u, def_flow_index: %u, rng_flow_start_index: %u, rng_flow_cnt: %u\n",
		    ch_index, def_flow_index,
		    rng_flow_start_index, rng_flow_cnt);

	if (uc->rflow->id != def_flow_index) {
		dev_err(ud->dev, "rflow mismatch %u vs %u\n",
			def_flow_index, uc->rflow->id);
		goto rchan_free;
	}

	flow_req.nav_id = ud->tisci_dev_id;
	flow_req.flow_index = def_flow_index;
	flow_req.rx_ch_index = uc->rchan->id;
	if (uc->needs_epib)
		flow_req.rx_einfo_present = 1;
	else
		flow_req.rx_einfo_present = 0;

	if (uc->psd_size)
		flow_req.rx_psinfo_present = 1;
	else
		flow_req.rx_psinfo_present = 0;

	flow_req.rx_error_handling = 0;
	flow_req.rx_desc_type = 0;
	flow_req.rx_sop_offset = 0;
	flow_req.rx_dest_qnum = rx_ring;
	flow_req.rx_ps_location = 0;
	flow_req.rx_src_tag_hi = 0;
	flow_req.rx_src_tag_lo = 0;
	flow_req.rx_dest_tag_hi = 0;
	flow_req.rx_dest_tag_lo = 0;
	flow_req.rx_src_tag_hi_sel = 2;
	flow_req.rx_src_tag_lo_sel = 4;
	flow_req.rx_dest_tag_hi_sel = 5;
	flow_req.rx_dest_tag_lo_sel = 4;
	flow_req.rx_size_thresh_en = 0;
	flow_req.rx_fdq0_sz0_qnum = fd_ring;
	flow_req.rx_fdq1_qnum = fd_ring;
	flow_req.rx_fdq2_qnum = fd_ring;
	flow_req.rx_fdq3_qnum = fd_ring;

	ret = ud->tisci_udmap_ops->rx_flow_cfg(ud->tisci,
			&flow_req);
	if (ret) {
		dev_err(ud->dev, "tisci flow config failed %d\n", ret);
		goto rchan_free;
	}

	return 0;

rchan_free:
	ud->tisci_udmap_ops->rx_ch_free(ud->tisci, -1,
					ud->tisci_dev_id,
					uc->rchan->id);
	return ret;
}

static int udma_alloc_chan_resources(struct udma_chan *uc)
{
	struct udma_dev *ud = uc->ud;
	int ret;

	k3_udma_dbg("%s: chan:%d as %s\n",
		    __func__, uc->id, udma_get_dir_text(uc->dir));

	switch (uc->dir) {
	case DMA_MEM_TO_MEM:
		/* Non synchronized - mem to mem type of transfer */
		ret = udma_get_chan_pair(uc);
		if (ret)
			return ret;

		ret = udma_alloc_tx_resources(uc);
		if (ret)
			goto err_free_res;

		ret = udma_alloc_rx_resources(uc);
		if (ret)
			goto err_free_res;

		uc->src_thread = ud->psil_base + uc->tchan->id;
		uc->dst_thread = (ud->psil_base + uc->rchan->id) | 0x8000;
		uc->pkt_mode = true;
		break;
	case DMA_MEM_TO_DEV:
		/* Slave transfer synchronized - mem to dev (TX) trasnfer */
		ret = udma_alloc_tx_resources(uc);
		if (ret)
			goto err_free_res;

		uc->src_thread = ud->psil_base + uc->tchan->id;
		uc->dst_thread = uc->slave_thread_id;
		if (!(uc->dst_thread & 0x8000))
			uc->dst_thread |= 0x8000;

		break;
	case DMA_DEV_TO_MEM:
		/* Slave transfer synchronized - dev to mem (RX) trasnfer */
		ret = udma_alloc_rx_resources(uc);
		if (ret)
			goto err_free_res;

		uc->src_thread = uc->slave_thread_id;
		uc->dst_thread = (ud->psil_base + uc->rchan->id) | 0x8000;

		break;
	default:
		/* Can not happen */
		k3_udma_dbg("%s: chan:%d invalid direction (%u)\n",
			    __func__, uc->id, uc->dir);
		return -EINVAL;
	}

	/* We have channel indexes and rings */
	if (uc->dir == DMA_MEM_TO_MEM) {
		ret = udma_alloc_tchan_sci_req(uc);
		if (ret)
			goto err_free_res;

		ret = udma_alloc_rchan_sci_req(uc);
		if (ret) {
			ud->tisci_udmap_ops->tx_ch_free(
					ud->tisci, -1,
					ud->tisci_dev_id,
					uc->tchan->id);
			goto err_free_res;
		}
	} else {
		/* Slave transfer */
		if (uc->dir == DMA_MEM_TO_DEV) {
			ret = udma_alloc_tchan_sci_req(uc);
			if (ret)
				goto err_free_res;
		} else {
			ret = udma_alloc_rchan_sci_req(uc);
			if (ret)
				goto err_free_res;
		}
	}

	/* PSI-L pairing */
	uc->psi_link = k3_nav_psil_request_link(ud->psil_node, uc->src_thread,
						uc->dst_thread);
	if (IS_ERR(uc->psi_link)) {
		dev_err(ud->dev, "k3_nav_psil_request_link fail\n");
		if (uc->tchan)
			ud->tisci_udmap_ops->tx_ch_free(ud->tisci, -1,
							ud->tisci_dev_id,
							uc->tchan->id);

		if (uc->rchan)
			ud->tisci_udmap_ops->rx_ch_free(ud->tisci, -1,
							ud->tisci_dev_id,
							uc->rchan->id);

		ret = PTR_ERR(uc->psi_link);
		goto err_free_res;
	}

	return 0;

err_free_res:
	udma_free_tx_resources(uc);
	udma_free_rx_resources(uc);
	uc->slave_thread_id = -1;
	return ret;
}

static void udma_free_chan_resources(struct udma_chan *uc)
{
	struct udma_dev *ud = uc->ud;

	/* Some configuration to UDMA-P channel: disable, reset, whatever */

	/* Release PSI-L pairing */
	k3_nav_psil_release_link(uc->psi_link);

	if (uc->tchan)
		ud->tisci_udmap_ops->tx_ch_free(ud->tisci, -1,
						ud->tisci_dev_id,
						uc->tchan->id);

	if (uc->rchan)
		ud->tisci_udmap_ops->rx_ch_free(ud->tisci, -1,
						ud->tisci_dev_id,
						uc->rchan->id);

	/* Reset the rings for a new start */
	udma_reset_rings(uc);
	udma_free_tx_resources(uc);
	udma_free_rx_resources(uc);

	uc->slave_thread_id = -1;
	uc->dir = DMA_MEM_TO_MEM;
}

static int udma_get_mmrs(struct udevice *dev)
{
	struct udma_dev *ud = dev_get_priv(dev);
	int i;

	for (i = 0; i < MMR_LAST; i++) {
		ud->mmrs[i] = (uint32_t *)devfdt_get_addr_name(dev,
				mmr_names[i]);
		if (!ud->mmrs[i])
			return -EINVAL;
	}

	return 0;
}

#define UDMA_MAX_CHANNELS	192

static int udma_probe(struct udevice *dev)
{
	struct dma_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct udma_dev *ud = dev_get_priv(dev);
	int i, ret;
	u32 cap2, cap3;
	struct udevice *tmp;
	struct udevice *tisci_dev = NULL;

	ret = udma_get_mmrs(dev);
	if (ret)
		return ret;

	ret = uclass_get_device_by_phandle(UCLASS_MISC, dev,
					   "ti,psi-proxy",
					   &ud->psil_node);
	if (!ud->psil_node) {
		dev_err(dev, "PSILCFG node is not found\n");
		return -ENODEV;
	}

	ret = uclass_get_device_by_phandle(UCLASS_MISC, dev,
					   "ti,ringacc", &tmp);
	ud->ringacc = dev_get_priv(tmp);
	if (IS_ERR(ud->ringacc))
		return PTR_ERR(ud->ringacc);

	ud->ch_count = dev_read_u32_default(dev, "dma-channels", 0);
	if (!ud->ch_count) {
		dev_info(dev, "Missing dma-channels property, using %u.\n",
			 UDMA_MAX_CHANNELS);
		ud->ch_count = UDMA_MAX_CHANNELS;
	}

	ud->psil_base = dev_read_u32_default(dev, "ti,psil-base", 0);
	if (!ud->psil_base) {
		dev_info(dev,
			 "Missing ti,psil-base property, using %d.\n", ret);
		return -EINVAL;
	}

	ret = uclass_get_device_by_name(UCLASS_FIRMWARE, "dmsc", &tisci_dev);
	if (ret) {
		debug("TISCI RA RM get failed (%d)\n", ret);
		ud->tisci = NULL;
		return 0;
	}
	ud->tisci = (struct ti_sci_handle *)
			 (ti_sci_get_handle_from_sysfw(tisci_dev));

	ret = dev_read_u32_default(dev, "ti,sci", 0);
	if (!ret) {
		dev_err(dev, "TISCI RA RM disabled\n");
		ud->tisci = NULL;
	}

	if (ud->tisci) {
		ud->tisci_dev_id = -1;
		ret = dev_read_u32(dev, "ti,sci-dev-id", &ud->tisci_dev_id);
		if (ret) {
			dev_err(dev, "ti,sci-dev-id read failure %d\n", ret);
			return ret;
		}
		ud->tisci_udmap_ops = &ud->tisci->ops.rm_udmap_ops;
	}

	cap2 = udma_read(ud->mmrs[MMR_GCFG], 0x28);
	cap3 = udma_read(ud->mmrs[MMR_GCFG], 0x2c);

	ud->rflow_cnt = cap3 & 0x3fff;
	ud->tchan_cnt = cap2 & 0x1ff;
	ud->echan_cnt = (cap2 >> 9) & 0x1ff;
	ud->rchan_cnt = (cap2 >> 18) & 0x1ff;
	if ((ud->tchan_cnt + ud->rchan_cnt) != ud->ch_count) {
		dev_info(dev,
			 "Channel count mismatch: %u != tchan(%u) + rchan(%u)\n",
			 ud->ch_count, ud->tchan_cnt, ud->rchan_cnt);
		ud->ch_count  = ud->tchan_cnt + ud->rchan_cnt;
	}

	dev_info(dev,
		 "Number of channels: %u (tchan: %u, echan: %u, rchan: %u dev-id %u)\n",
		 ud->ch_count, ud->tchan_cnt, ud->echan_cnt, ud->rchan_cnt,
		 ud->tisci_dev_id);
	dev_info(dev, "Number of rflows: %u\n", ud->rflow_cnt);

	ud->channels = devm_kcalloc(dev, ud->ch_count, sizeof(*ud->channels),
				    GFP_KERNEL);
	ud->tchan_map = devm_kcalloc(dev, BITS_TO_LONGS(ud->tchan_cnt),
				     sizeof(unsigned long), GFP_KERNEL);
	ud->tchans = devm_kcalloc(dev, ud->tchan_cnt,
				  sizeof(*ud->tchans), GFP_KERNEL);
	ud->rchan_map = devm_kcalloc(dev, BITS_TO_LONGS(ud->rchan_cnt),
				     sizeof(unsigned long), GFP_KERNEL);
	ud->rchans = devm_kcalloc(dev, ud->rchan_cnt,
				  sizeof(*ud->rchans), GFP_KERNEL);
	ud->rflow_map = devm_kcalloc(dev, BITS_TO_LONGS(ud->rflow_cnt),
				     sizeof(unsigned long), GFP_KERNEL);
	ud->rflows = devm_kcalloc(dev, ud->rflow_cnt,
				  sizeof(*ud->rflows), GFP_KERNEL);

	if (!ud->channels || !ud->tchan_map || !ud->rchan_map ||
	    !ud->rflow_map || !ud->tchans || !ud->rchans || !ud->rflows)
		return -ENOMEM;

	for (i = 0; i < ud->tchan_cnt; i++) {
		struct udma_tchan *tchan = &ud->tchans[i];

		tchan->id = i;
		tchan->reg_rt = ud->mmrs[MMR_TCHANRT] + UDMA_CH_1000(i);
	}

	for (i = 0; i < ud->rchan_cnt; i++) {
		struct udma_rchan *rchan = &ud->rchans[i];

		rchan->id = i;
		rchan->reg_rt = ud->mmrs[MMR_RCHANRT] + UDMA_CH_1000(i);
	}

	for (i = 0; i < ud->rflow_cnt; i++) {
		struct udma_rflow *rflow = &ud->rflows[i];

		rflow->id = i;
	}

	for (i = 0; i < ud->ch_count; i++) {
		struct udma_chan *uc = &ud->channels[i];

		uc->ud = ud;
		uc->id = i;
		uc->slave_thread_id = -1;
		uc->tchan = NULL;
		uc->rchan = NULL;
		uc->dir = DMA_MEM_TO_MEM;
		sprintf(uc->name, "UDMA chan%d\n", i);
		if (!i)
			uc->in_use = true;
	}

	k3_udma_dbg("UDMA(rev: 0x%08x) CAP0-3: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
		    udma_read(ud->mmrs[MMR_GCFG], 0),
		    udma_read(ud->mmrs[MMR_GCFG], 0x20),
		    udma_read(ud->mmrs[MMR_GCFG], 0x24),
		    udma_read(ud->mmrs[MMR_GCFG], 0x28),
		    udma_read(ud->mmrs[MMR_GCFG], 0x2c));

	uc_priv->supported = DMA_SUPPORTS_MEM_TO_MEM | DMA_SUPPORTS_MEM_TO_DEV;

	return ret;
}

static void udma_fill_rxfdq_ring(struct udma_chan *uc, void *src,
				 void *dst, size_t len)
{
	dma_addr_t dma_dst = (dma_addr_t)dst;
	unsigned long dummy;
	struct knav_udmap_host_desc_t *desc_rx;

	desc_rx = dma_alloc_coherent(sizeof(*desc_rx), &dummy);
	memset(desc_rx, 0, sizeof(*desc_rx));

	knav_udmap_hdesc_init(desc_rx, 0, 0);
	knav_udmap_hdesc_set_pktlen(desc_rx, len);
	knav_udmap_hdesc_attach_buf(desc_rx, dma_dst, len, dma_dst, len);

	flush_dcache_range((u64)desc_rx,
			   ALIGN((u64)desc_rx + sizeof(*desc_rx),
				 ARCH_DMA_MINALIGN));
	k3_nav_ringacc_ring_push(uc->rchan->fd_ring, &desc_rx);
}

static void udma_push_tx_ring(struct udma_chan *uc, void *src,
			      void *dst, size_t len)
{
	u32 tc_ring_id = k3_nav_ringacc_get_ring_id(uc->tchan->tc_ring);
	dma_addr_t dma_src = (dma_addr_t)src;
	unsigned long dummy;
	struct knav_udmap_host_desc_t *desc_tx;

	desc_tx = dma_alloc_coherent(sizeof(*desc_tx), &dummy);
	memset(desc_tx, 0, sizeof(*desc_tx));

	knav_udmap_hdesc_init(desc_tx, 0, 0);
	knav_udmap_hdesc_set_pktlen(desc_tx, len);
	knav_udmap_hdesc_attach_buf(desc_tx, dma_src, len, dma_src, len);
	knav_udmap_hdesc_set_pktids(&desc_tx->hdr, uc->id, 0x3fff);
	knav_udmap_desc_set_retpolicy(&desc_tx->hdr, 0, tc_ring_id);

	flush_dcache_range((u64)desc_tx,
			   ALIGN((u64)desc_tx + sizeof(*desc_tx),
				 ARCH_DMA_MINALIGN));
	k3_nav_ringacc_ring_push(uc->tchan->t_ring, &desc_tx);
}

static int udma_transfer(struct udevice *dev, int direction,
			 void *dst, void *src, size_t len)
{
	struct udma_dev *ud = dev_get_priv(dev);
	/* Hard code to channel 0 for now */
	struct udma_chan *uc = &ud->channels[0];
	dma_addr_t paddr;
	int ret;

	ret = udma_alloc_chan_resources(uc);
	if (ret)
		return ret;
	udma_fill_rxfdq_ring(uc, src, dst, len);

	udma_start(uc);
	udma_push_tx_ring(uc, src, dst, len);
	udma_poll_completion(uc, &paddr);
	udma_stop(uc);

	udma_free_chan_resources(uc);
	return 0;
}

static int udma_request(struct dma *dma)
{
	struct udma_dev *ud = dev_get_priv(dma->dev);
	struct udma_chan *uc;
	unsigned long dummy;
	int ret;

	if (dma->id >= (ud->rchan_cnt + ud->tchan_cnt)) {
		dev_err(dma->dev, "invalid dma ch_id %lu\n", dma->id);
		return -EINVAL;
	}

	uc = &ud->channels[dma->id];
	ret = udma_alloc_chan_resources(uc);
	if (ret) {
		dev_err(dma->dev, "alloc dma res failed %d\n", ret);
		return -EINVAL;
	}

	uc->hdesc_size = knav_udmap_hdesc_calc_size(
				uc->needs_epib, uc->psd_size, 0);
	uc->hdesc_size = ALIGN(uc->hdesc_size, ARCH_DMA_MINALIGN);

	if (uc->dir == DMA_MEM_TO_DEV) {
		uc->desc_tx = dma_alloc_coherent(uc->hdesc_size, &dummy);
		memset(uc->desc_tx, 0, uc->hdesc_size);
	} else {
		uc->desc_rx = dma_alloc_coherent(
				uc->hdesc_size * UDMA_TX_DESC_NUM, &dummy);
		memset(uc->desc_rx, 0, uc->hdesc_size * UDMA_TX_DESC_NUM);
		uc->buf_rx = dma_alloc_coherent(
				UDMA_TX_BUF_SIZE * UDMA_TX_DESC_NUM, &dummy);
		memset(uc->buf_rx, 0, UDMA_TX_BUF_SIZE * UDMA_TX_DESC_NUM);
	}

	uc->in_use = true;

	return 0;
}

static int udma_free(struct dma *dma)
{
	struct udma_dev *ud = dev_get_priv(dma->dev);
	struct udma_chan *uc;

	if (dma->id >= (ud->rchan_cnt + ud->tchan_cnt)) {
		dev_err(dma->dev, "invalid dma ch_id %lu\n", dma->id);
		return -EINVAL;
	}
	uc = &ud->channels[dma->id];

	if (udma_is_chan_running(uc))
		udma_stop(uc);
	udma_free_chan_resources(uc);

	uc->in_use = false;

	return 0;
}

static int udma_enable(struct dma *dma)
{
	struct udma_dev *ud = dev_get_priv(dma->dev);
	struct udma_chan *uc;
	int ret;

	if (dma->id >= (ud->rchan_cnt + ud->tchan_cnt)) {
		dev_err(dma->dev, "invalid dma ch_id %lu\n", dma->id);
		return -EINVAL;
	}
	uc = &ud->channels[dma->id];

	ret = udma_start(uc);
	if (ret)
		return ret;

	if (uc->dir == DMA_DEV_TO_MEM) {
		u32 i;

		for (i = 0; i < UDMA_TX_DESC_NUM; i++) {
			struct knav_udmap_host_desc_t *desc_rx;
			dma_addr_t dma_dst;

			desc_rx = uc->desc_rx + (i * uc->hdesc_size);
			dma_dst = (dma_addr_t)(uc->buf_rx +
					(i * UDMA_TX_BUF_SIZE));

			knav_udmap_hdesc_reset_hbdesc(desc_rx);

			knav_udmap_hdesc_init(
				desc_rx,
				uc->needs_epib ?
				KNAV_UDMAP_INFO0_HDESC_EPIB_PRESENT : 0,
				uc->psd_size);
			knav_udmap_hdesc_set_pktlen(desc_rx, UDMA_TX_BUF_SIZE);
			knav_udmap_hdesc_attach_buf(
					desc_rx, dma_dst,
					UDMA_TX_BUF_SIZE, dma_dst,
					UDMA_TX_BUF_SIZE);

			flush_dcache_range((u64)desc_rx,
					   ALIGN((u64)desc_rx + uc->hdesc_size,
						 ARCH_DMA_MINALIGN));

			k3_nav_ringacc_ring_push(uc->rchan->fd_ring, &desc_rx);
		}
	}

	return ret;
}

static int udma_disable(struct dma *dma)
{
	struct udma_dev *ud = dev_get_priv(dma->dev);
	struct udma_chan *uc;
	int ret = 0;

	if (dma->id >= (ud->rchan_cnt + ud->tchan_cnt)) {
		dev_err(dma->dev, "invalid dma ch_id %lu\n", dma->id);
		return -EINVAL;
	}
	uc = &ud->channels[dma->id];

	if (udma_is_chan_running(uc))
		ret = udma_stop(uc);
	else
		dev_err(dma->dev, "%s not running\n", __func__);

	return ret;
}

static int udma_send(struct dma *dma, void *src, size_t len, void *metadata)
{
	struct udma_dev *ud = dev_get_priv(dma->dev);
	struct knav_udmap_host_desc_t *desc_tx;
	dma_addr_t dma_src = (dma_addr_t)src;
	struct ti_udma_drv_packet_data packet_data = { 0 };
	dma_addr_t paddr;
	struct udma_chan *uc;
	u32 tc_ring_id;
	int ret;

	if (!metadata)
		packet_data = *((struct ti_udma_drv_packet_data *)metadata);

	if (dma->id >= (ud->rchan_cnt + ud->tchan_cnt)) {
		dev_err(dma->dev, "invalid dma ch_id %lu\n", dma->id);
		return -EINVAL;
	}
	uc = &ud->channels[dma->id];

	if (uc->dir != DMA_MEM_TO_DEV)
		return -EINVAL;

	tc_ring_id = k3_nav_ringacc_get_ring_id(uc->tchan->tc_ring);

	desc_tx = uc->desc_tx;

	knav_udmap_hdesc_reset_hbdesc(desc_tx);

	knav_udmap_hdesc_init(
		desc_tx,
		uc->needs_epib ? KNAV_UDMAP_INFO0_HDESC_EPIB_PRESENT : 0,
		uc->psd_size);
	knav_udmap_hdesc_set_pktlen(desc_tx, len);
	knav_udmap_hdesc_attach_buf(desc_tx, dma_src, len, dma_src, len);
	knav_udmap_hdesc_set_pktids(&desc_tx->hdr, uc->id, 0x3fff);
	knav_udmap_desc_set_retpolicy(&desc_tx->hdr, 0, tc_ring_id);
	/* pass below information from caller */
	knav_udmap_hdesc_set_pkttype(desc_tx, packet_data.pkt_type);
	knav_udmap_desc_set_tags_ids(&desc_tx->hdr, 0, packet_data.dest_tag);

	k3_udma_print_buf((ulong)desc_tx, desc_tx, 4, uc->hdesc_size / 4, 0);

	flush_dcache_range((u64)dma_src,
			   ALIGN((u64)dma_src + len,
				 ARCH_DMA_MINALIGN));

	flush_dcache_range((u64)desc_tx,
			   ALIGN((u64)desc_tx + uc->hdesc_size,
				 ARCH_DMA_MINALIGN));
	ret = k3_nav_ringacc_ring_push(uc->tchan->t_ring, &uc->desc_tx);
	if (ret) {
		dev_err(dma->dev, "TX dma push fail ch_id %lu %d\n",
			dma->id, ret);
		return ret;
	}

	udma_poll_completion(uc, &paddr);

	desc_tx = (struct knav_udmap_host_desc_t *)paddr;

	return 0;
}

static int udma_receive(struct dma *dma, void **dst, void *metadata)
{
	struct udma_dev *ud = dev_get_priv(dma->dev);
	struct knav_udmap_host_desc_t *desc_rx;
	dma_addr_t paddr;
	dma_addr_t buf_dma;
	struct udma_chan *uc;
	u32 buf_dma_len, pkt_len;
	u32 port_id = 0;
	int ret;

	k3_udma_dev_dbg(dma->dev, "%s\n", __func__);

	if (dma->id >= (ud->rchan_cnt + ud->tchan_cnt)) {
		dev_err(dma->dev, "invalid dma ch_id %lu\n", dma->id);
		return -EINVAL;
	}
	uc = &ud->channels[dma->id];

	if (uc->dir != DMA_DEV_TO_MEM)
		return -EINVAL;

	ret = k3_nav_ringacc_ring_pop(uc->rchan->r_ring, &paddr);
	if (ret && ret != -ENODATA) {
		dev_err(dma->dev, "rx dma fail ch_id:%lu %d\n", dma->id, ret);
		return ret;
	} else if (ret == -ENODATA) {
		return 0;
	}

	desc_rx = (struct knav_udmap_host_desc_t *)paddr;
	k3_udma_print_buf((ulong)desc_rx, desc_rx, 4, uc->hdesc_size / 4, 0);

	/* invalidate cache data */
	invalidate_dcache_range((ulong)desc_rx,
				(ulong)(desc_rx + uc->hdesc_size));

	knav_udmap_hdesc_get_obuf(desc_rx, &buf_dma, &buf_dma_len);
	pkt_len = knav_udmap_hdesc_get_pktlen(desc_rx);

	/* invalidate cache data */
	invalidate_dcache_range((ulong)buf_dma,
				(ulong)(buf_dma + UDMA_TX_BUF_SIZE));

	knav_udmap_desc_get_tags_ids(&desc_rx->hdr, &port_id, NULL);
	k3_udma_dev_dbg(dma->dev, "%s rx port_id:%d\n", __func__, port_id);

	memcpy(*dst, (void *)buf_dma, pkt_len);

	ret = pkt_len;

	knav_udmap_hdesc_reset_hbdesc(desc_rx);

	knav_udmap_hdesc_init(
		desc_rx,
		uc->needs_epib ? KNAV_UDMAP_INFO0_HDESC_EPIB_PRESENT : 0,
		uc->psd_size);
	knav_udmap_hdesc_set_pktlen(desc_rx, UDMA_TX_BUF_SIZE);
	knav_udmap_hdesc_attach_buf(desc_rx, buf_dma,
				    UDMA_TX_BUF_SIZE, buf_dma,
				    UDMA_TX_BUF_SIZE);

	flush_dcache_range((u64)desc_rx,
			   ALIGN((u64)desc_rx + uc->hdesc_size,
				 ARCH_DMA_MINALIGN));

	k3_nav_ringacc_ring_push(uc->rchan->fd_ring, &paddr);

	return ret;
}

static int udma_of_xlate(struct dma *dma, struct ofnode_phandle_args *args)
{
	struct udma_dev *ud = dev_get_priv(dma->dev);
	struct udma_chan *uc = &ud->channels[0];
	ofnode chconf_node, slave_node;
	char prop[50];
	u32 val;

	for (val = 0; val < ud->ch_count; val++) {
		uc = &ud->channels[val];
		if (!uc->in_use)
			break;
	}

	if (val == ud->ch_count)
		return -EBUSY;

	uc->dir = DMA_DEV_TO_MEM;
	if (args->args[2] == UDMA_DIR_TX)
		uc->dir = DMA_MEM_TO_DEV;

	slave_node = ofnode_get_by_phandle(args->args[0]);
	if (!ofnode_valid(slave_node)) {
		dev_err(ud->dev, "slave node is missing\n");
		return -EINVAL;
	}

	snprintf(prop, sizeof(prop), "ti,psil-config%u", args->args[1]);
	chconf_node = ofnode_find_subnode(slave_node, prop);
	if (!ofnode_valid(chconf_node)) {
		dev_err(ud->dev, "Channel configuration node is missing\n");
		return -EINVAL;
	}

	if (!ofnode_read_u32(chconf_node, "linux,udma-mode", &val)) {
		if (val == UDMA_PKT_MODE)
			uc->pkt_mode = true;
	}

	if (!ofnode_read_u32(chconf_node, "statictr-type", &val))
		uc->static_tr_type = val;

	uc->needs_epib = ofnode_read_bool(chconf_node, "ti,needs-epib");
	if (!ofnode_read_u32(chconf_node, "ti,psd-size", &val))
		uc->psd_size = val;
	uc->metadata_size = (uc->needs_epib ? 16 : 0) + uc->psd_size;

	if (ofnode_read_u32(slave_node, "ti,psil-base", &val)) {
		dev_err(ud->dev, "ti,psil-base is missing\n");
		return -EINVAL;
	}

	uc->slave_thread_id = val + args->args[1];

	dma->id = uc->id;
	k3_udma_dbg("Allocated dma chn:%lu epib:%d psdata:%u meta:%u thread_id:%x\n",
		    dma->id, uc->needs_epib,
		    uc->psd_size, uc->metadata_size,
		    uc->slave_thread_id);

	return 0;
}

static const struct dma_ops udma_ops = {
	.transfer	= udma_transfer,
	.of_xlate	= udma_of_xlate,
	.request	= udma_request,
	.free		= udma_free,
	.enable		= udma_enable,
	.disable	= udma_disable,
	.send		= udma_send,
	.receive	= udma_receive,
};

static const struct udevice_id udma_ids[] = {
	{ .compatible = "ti,k3-navss-udmap" },
	{ }
};

U_BOOT_DRIVER(ti_edma3) = {
	.name	= "ti-udma",
	.id	= UCLASS_DMA,
	.of_match = udma_ids,
	.ops	= &udma_ops,
	.probe	= udma_probe,
	.priv_auto_alloc_size = sizeof(struct udma_dev),
};
