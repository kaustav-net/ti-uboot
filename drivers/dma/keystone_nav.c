/*
 * Multicore Navigator driver for TI Keystone 2 devices.
 *
 * (C) Copyright 2012-2014
 *     Texas Instruments Incorporated, <www.ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/ti-common/keystone_nav.h>

struct qm_config qm_memmap = {
	.stat_cfg	= CONFIG_KSNAV_QM_QUEUE_STATUS_BASE,
	.queue		= (void *)CONFIG_KSNAV_QM_MANAGER_QUEUES_BASE,
	.mngr_vbusm	= CONFIG_KSNAV_QM_BASE_ADDRESS,
	.i_lram		= CONFIG_KSNAV_QM_LINK_RAM_BASE,
	.proxy		= (void *)CONFIG_KSNAV_QM_MANAGER_Q_PROXY_BASE,
	.status_ram	= CONFIG_KSNAV_QM_STATUS_RAM_BASE,
	.mngr_cfg	= (void *)CONFIG_KSNAV_QM_CONF_BASE,
	.intd_cfg	= CONFIG_KSNAV_QM_INTD_CONF_BASE,
	.desc_mem	= (void *)CONFIG_KSNAV_QM_DESC_SETUP_BASE,
	.region_num	= CONFIG_KSNAV_QM_REGION_NUM,
	.pdsp_cmd	= CONFIG_KSNAV_QM_PDSP1_CMD_BASE,
	.pdsp_ctl	= CONFIG_KSNAV_QM_PDSP1_CTRL_BASE,
	.pdsp_iram	= CONFIG_KSNAV_QM_PDSP1_IRAM_BASE,
	.i_lram_size	= CONFIG_KSNAV_QM_LINK_RAM_SIZE,
	.start_queue	= CONFIG_KSNAV_QM_START_QUEUE,
	.num_queues	= CONFIG_KSNAV_QM_QUEUES_PER_QMGR,
	.qpool_num	= CONFIG_KSNAV_QM_QPOOL_NUM,
};

#ifdef CONFIG_SOC_K2HK
struct qm_config ks2_qm2_memmap = {
	.stat_cfg	= CONFIG_KSNAV_QM2_QUEUE_STATUS_BASE,
	.queue		= (void *)CONFIG_KSNAV_QM2_MANAGER_QUEUES_BASE,
	.mngr_vbusm	= CONFIG_KSNAV_QM2_BASE_ADDRESS,
	.i_lram		= CONFIG_KSNAV_QM2_LINK_RAM_BASE,
	.proxy		= (void *)CONFIG_KSNAV_QM2_MANAGER_Q_PROXY_BASE,
	.status_ram	= CONFIG_KSNAV_QM2_STATUS_RAM_BASE,
	.mngr_cfg	= (void *)CONFIG_KSNAV_QM2_CONF_BASE,
	.intd_cfg	= CONFIG_KSNAV_QM2_INTD_CONF_BASE,
	.desc_mem	= (void *)CONFIG_KSNAV_QM2_DESC_SETUP_BASE,
	.region_num	= CONFIG_KSNAV_QM2_REGION_NUM,
	.pdsp_cmd	= CONFIG_KSNAV_QM2_PDSP1_CMD_BASE,
	.pdsp_ctl	= CONFIG_KSNAV_QM2_PDSP1_CTRL_BASE,
	.pdsp_iram	= CONFIG_KSNAV_QM2_PDSP1_IRAM_BASE,
	.i_lram_size	= CONFIG_KSNAV_QM2_LINK_RAM_SIZE,
	.start_queue	= CONFIG_KSNAV_QM2_START_QUEUE,
	.num_queues	= CONFIG_KSNAV_QM2_QUEUES_PER_QMGR,
	.qpool_num	= CONFIG_KSNAV_QM2_QPOOL_NUM,
};
#endif

/*
 * We are going to use only one type of descriptors - host packet
 * descriptors. We staticaly allocate memory for them here
 */
struct qm_host_desc desc_pool[HDESC_NUM] __aligned(sizeof(struct qm_host_desc));

static struct qm_config *qm_cfg;
static struct qm_config *qm2_cfg;

inline int num_of_desc_to_reg(int num_descr)
{
	int j, num;

	for (j = 0, num = 32; j < 15; j++, num *= 2) {
		if (num_descr <= num)
			return j;
	}

	return 15;
}

static inline u32 qnum_to_logical_qmgr(u32 qnum)
{
	return qnum / CONFIG_KSNAV_QM_QS_PER_LOGICAL_QM;
}

static inline bool qmgr_manage_qnum(struct qm_config *qm, u32 qnum)
{
	u32 startq, endq;

	startq = qm->start_queue;
	endq = qm->start_queue + qm->num_queues - 1;

	return (startq <= qnum) && (qnum <= endq);
}

static inline struct qm_config *qnum_to_qmgr(u32 qnum)
{
	if (qm_cfg && qmgr_manage_qnum(qm_cfg, qnum))
		return qm_cfg;

	if (qm2_cfg && qmgr_manage_qnum(qm2_cfg, qnum))
		return qm2_cfg;

	return NULL;
}

int _qm_init(struct qm_config *cfg)
{
	u32 j;
	struct qm_host_desc *desc;

	cfg->mngr_cfg->link_ram_base0	= cfg->i_lram;
	cfg->mngr_cfg->link_ram_size0	= cfg->i_lram_size;
	cfg->mngr_cfg->link_ram_base1	= 0;
	cfg->mngr_cfg->link_ram_size1	= 0;
	cfg->mngr_cfg->link_ram_base2	= 0;

	cfg->desc_mem[0].base_addr = cfg->desc_pool_base;
	cfg->desc_mem[0].start_idx = 0;
	cfg->desc_mem[0].desc_reg_size =
		(((sizeof(struct qm_host_desc) >> 4) - 1) << 16) |
		num_of_desc_to_reg(HDESC_NUM);

	memset((void *)cfg->desc_pool_base, 0, cfg->desc_pool_size);

	desc = (struct qm_host_desc *)cfg->desc_pool_base;
	for (j = 0; j < cfg->num_desc; j++)
		qm_push(&desc[j], cfg->qpool_num);

	return QM_OK;
}

int qm_init(void)
{
	int ret;

	if (qm_cfg)
		return QM_OK;

	if (qm2_cfg)
		return QM_ERR;

	memset(desc_pool, 0, sizeof(desc_pool));
	qm_memmap.desc_pool_base = (u32)desc_pool;
	qm_memmap.desc_pool_size = sizeof(desc_pool);
	qm_memmap.num_desc = HDESC_NUM;

	qm_cfg = &qm_memmap;

	ret = _qm_init(qm_cfg);
	if (ret != QM_OK)
		qm_cfg = NULL;

	return ret;
}

static void _qm_close(struct qm_config *qm)
{
	u32	j;

	queue_close(qm->qpool_num);

	qm->mngr_cfg->link_ram_base0	= 0;
	qm->mngr_cfg->link_ram_size0	= 0;
	qm->mngr_cfg->link_ram_base1	= 0;
	qm->mngr_cfg->link_ram_size1	= 0;
	qm->mngr_cfg->link_ram_base2	= 0;

	for (j = 0; j < qm->region_num; j++) {
		qm->desc_mem[j].base_addr = 0;
		qm->desc_mem[j].start_idx = 0;
		qm->desc_mem[j].desc_reg_size = 0;
	}
}

void qm_close(void)
{
	if (!qm_cfg)
		return;

	_qm_close(qm_cfg);
	qm_cfg = NULL;
}

#ifdef CONFIG_SOC_K2HK
int qm2_init(void)
{
	int ret;

	if (qm2_cfg)
		return QM_OK;

	if (qm_cfg)
		return QM_ERR;

	memset(desc_pool, 0, sizeof(desc_pool));
	ks2_qm2_memmap.desc_pool_base = (u32)desc_pool;
	ks2_qm2_memmap.desc_pool_size = sizeof(desc_pool);
	ks2_qm2_memmap.num_desc = HDESC_NUM;

	qm2_cfg = &ks2_qm2_memmap;

	ret = _qm_init(qm2_cfg);
	if (ret != QM_OK)
		qm2_cfg = NULL;

	return ret;
}

void qm2_close(void)
{
	if (!qm2_cfg)
		return;

	_qm_close(qm2_cfg);
	qm2_cfg = NULL;
}
#else
int qm2_init(void)
{
	return QM_ERR;
}

void qm2_close(void)
{
}
#endif

void qm_push(struct qm_host_desc *hd, u32 qnum)
{
	u32 regd;
	struct qm_config *qm;

	qm = qnum_to_qmgr(qnum);
	if (!qm)
		return;

	qnum -= qm->start_queue;

	cpu_to_bus((u32 *)hd, sizeof(struct qm_host_desc)/4);
	regd = (u32)hd | ((sizeof(struct qm_host_desc) >> 4) - 1);
	writel(regd, &qm->queue[qnum].ptr_size_thresh);
}

void qm_buff_push(struct qm_host_desc *hd, u32 qnum,
		    void *buff_ptr, u32 buff_len)
{
	hd->orig_buff_len = buff_len;
	hd->buff_len = buff_len;
	hd->orig_buff_ptr = (u32)buff_ptr;
	hd->buff_ptr = (u32)buff_ptr;
	qm_push(hd, qnum);
}

struct qm_host_desc *qm_pop(u32 qnum)
{
	u32 uhd;
	struct qm_config *qm;

	qm = qnum_to_qmgr(qnum);
	if (!qm)
		return NULL;

	qnum -= qm->start_queue;

	uhd = readl(&qm->queue[qnum].ptr_size_thresh) & ~0xf;
	if (uhd)
		cpu_to_bus((u32 *)uhd, sizeof(struct qm_host_desc)/4);

	return (struct qm_host_desc *)uhd;
}

struct qm_host_desc *qm_pop_from_free_pool(void)
{
	return qm_pop(qm_cfg->qpool_num);
}

void queue_close(u32 qnum)
{
	struct qm_host_desc *hd;

	while ((hd = qm_pop(qnum)))
		;
}

/**
 * DMA API
 */

static int ksnav_rx_disable(struct pktdma_cfg *pktdma)
{
	u32 j, v, k;

	for (j = 0; j < pktdma->rx_ch_num; j++) {
		v = readl(&pktdma->rx_ch[j].cfg_a);
		if (!(v & CPDMA_CHAN_A_ENABLE))
			continue;

		writel(v | CPDMA_CHAN_A_TDOWN, &pktdma->rx_ch[j].cfg_a);
		for (k = 0; k < TDOWN_TIMEOUT_COUNT; k++) {
			udelay(100);
			v = readl(&pktdma->rx_ch[j].cfg_a);
			if (!(v & CPDMA_CHAN_A_ENABLE))
				continue;
		}
		/* TODO: teardown error on if TDOWN_TIMEOUT_COUNT is reached */
	}

	/* Clear all of the flow registers */
	for (j = 0; j < pktdma->rx_flow_num; j++) {
		writel(0, &pktdma->rx_flows[j].control);
		writel(0, &pktdma->rx_flows[j].tags);
		writel(0, &pktdma->rx_flows[j].tag_sel);
		writel(0, &pktdma->rx_flows[j].fdq_sel[0]);
		writel(0, &pktdma->rx_flows[j].fdq_sel[1]);
		writel(0, &pktdma->rx_flows[j].thresh[0]);
		writel(0, &pktdma->rx_flows[j].thresh[1]);
		writel(0, &pktdma->rx_flows[j].thresh[2]);
	}

	return QM_OK;
}

static int ksnav_tx_disable(struct pktdma_cfg *pktdma)
{
	u32 j, v, k;

	for (j = 0; j < pktdma->tx_ch_num; j++) {
		v = readl(&pktdma->tx_ch[j].cfg_a);
		if (!(v & CPDMA_CHAN_A_ENABLE))
			continue;

		writel(v | CPDMA_CHAN_A_TDOWN, &pktdma->tx_ch[j].cfg_a);
		for (k = 0; k < TDOWN_TIMEOUT_COUNT; k++) {
			udelay(100);
			v = readl(&pktdma->tx_ch[j].cfg_a);
			if (!(v & CPDMA_CHAN_A_ENABLE))
				continue;
		}
		/* TODO: teardown error on if TDOWN_TIMEOUT_COUNT is reached */
	}

	return QM_OK;
}

int ksnav_init(struct pktdma_cfg *pktdma, struct rx_buff_desc *rx_buffers)
{
	u32 j, v, qm, qm_offset, num_lqms = 4;
	struct qm_host_desc *hd;
	u8 *rx_ptr;

	if (pktdma == NULL || rx_buffers == NULL ||
	    rx_buffers->buff_ptr == NULL)
		return QM_ERR;

	pktdma->rx_flow = rx_buffers->rx_flow;

	if (qm_cfg)
		pktdma->qpool_num = qm_cfg->qpool_num;
	else if (qm2_cfg)
		pktdma->qpool_num = qm2_cfg->qpool_num;
	else
		return QM_ERR;

	/* init rx queue */
	rx_ptr = rx_buffers->buff_ptr;

	for (j = 0; j < rx_buffers->num_buffs; j++) {
		hd = qm_pop(pktdma->qpool_num);
		if (hd == NULL)
			return QM_ERR;

		qm_buff_push(hd, pktdma->rx_free_q,
			     rx_ptr, rx_buffers->buff_len);

		rx_ptr += rx_buffers->buff_len;
	}

	ksnav_rx_disable(pktdma);

	/* configure rx channels */
	qm = qnum_to_logical_qmgr(pktdma->rx_rcv_q);
	v = CPDMA_REG_VAL_MAKE_RX_FLOW_A(1, 1, 0, 0, 0, 0,
					 qm, pktdma->rx_rcv_q);
	writel(v, &pktdma->rx_flows[pktdma->rx_flow].control);
	writel(0, &pktdma->rx_flows[pktdma->rx_flow].tags);
	writel(0, &pktdma->rx_flows[pktdma->rx_flow].tag_sel);

	v = CPDMA_REG_VAL_MAKE_RX_FLOW_D(qm, pktdma->rx_free_q, qm,
					 pktdma->rx_free_q);

	writel(v, &pktdma->rx_flows[pktdma->rx_flow].fdq_sel[0]);
	writel(v, &pktdma->rx_flows[pktdma->rx_flow].fdq_sel[1]);
	writel(0, &pktdma->rx_flows[pktdma->rx_flow].thresh[0]);
	writel(0, &pktdma->rx_flows[pktdma->rx_flow].thresh[1]);
	writel(0, &pktdma->rx_flows[pktdma->rx_flow].thresh[2]);

	for (j = 0; j < pktdma->rx_ch_num; j++)
		writel(CPDMA_CHAN_A_ENABLE, &pktdma->rx_ch[j].cfg_a);

	/* configure tx channels */
	/* Disable loopback in the tx direction */
	writel(0, &pktdma->global->emulation_control);

	/* Set QM base address, only for K2x devices */
	qm_offset = CONFIG_KSNAV_QM_QS_PER_LOGICAL_QM *
				sizeof(struct qm_reg_queue);
	if (cpu_is_k2e())
		num_lqms = 2;
	for (j = 0; j < 4; j++)
		writel(CONFIG_KSNAV_QM_BASE_ADDRESS +
		       (j % num_lqms) * qm_offset,
		       &pktdma->global->qm_base_addr[j]);

	/* Enable all channels. The current state isn't important */
	for (j = 0; j < pktdma->tx_ch_num; j++)  {
		writel(0, &pktdma->tx_ch[j].cfg_b);
		writel(CPDMA_CHAN_A_ENABLE, &pktdma->tx_ch[j].cfg_a);
	}

	return QM_OK;
}

int ksnav_close(struct pktdma_cfg *pktdma)
{
	if (!pktdma)
		return QM_ERR;

	ksnav_tx_disable(pktdma);
	ksnav_rx_disable(pktdma);

	queue_close(pktdma->rx_free_q);
	queue_close(pktdma->rx_rcv_q);
	queue_close(pktdma->tx_snd_q);

	return QM_OK;
}

int ksnav_send(struct pktdma_cfg *pktdma, u32 *pkt,
	       int num_bytes, u32 dest_port)
{
	struct qm_host_desc *hd;

	hd = qm_pop(pktdma->qpool_num);
	if (!hd) {
		printf("_netcp_send: no desc qpool_num %u\n",
		       pktdma->qpool_num);
		return QM_ERR;
	}

	dest_port &= 0xf;
	hd->desc_info = num_bytes;
	if (pktdma->dest_port_info == PKT_INFO) {
		hd->packet_info = pktdma->qpool_num | (dest_port << 16);
	} else {
		hd->packet_info = pktdma->qpool_num;
		hd->tag_info = dest_port;
	}

	qm_buff_push(hd, pktdma->tx_snd_q, pkt, num_bytes);

	return QM_OK;
}

void *ksnav_recv(struct pktdma_cfg *pktdma, u32 **pkt, int *num_bytes)
{
	struct qm_host_desc *hd;

	hd = qm_pop(pktdma->rx_rcv_q);
	if (!hd)
		return NULL;

	*pkt = (u32 *)hd->buff_ptr;
	*num_bytes = hd->desc_info & 0x3fffff;

	return hd;
}

void ksnav_release_rxhd(struct pktdma_cfg *pktdma, void *hd)
{
	struct qm_host_desc *_hd = (struct qm_host_desc *)hd;

	_hd->buff_len = _hd->orig_buff_len;
	_hd->buff_ptr = _hd->orig_buff_ptr;

	qm_push(_hd, pktdma->rx_free_q);
}
