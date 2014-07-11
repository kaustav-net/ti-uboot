/*
 * hw_edma_tpcc.h - register-level header file for EDMA_TPCC
 *
 * Copyright (C) 2014, Texas Instruments, Incorporated
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef HW_EDMA_TPCC_H_
#define HW_EDMA_TPCC_H_

#include <linux/compiler.h>

/* Register Definitions */

#define EDMA_TPCC_DCHMAPN(n)		(0x100 + (n * 4))
#define EDMA_TPCC_DMAQNUMN(n)		(0x240 + (n * 4))
#define EDMA_TPCC_QDMAQNUM		(0x260)
#define EDMA_TPCC_EMCR			(0x308)
#define EDMA_TPCC_EMCRH			(0x30c)
#define EDMA_TPCC_QEMCR			(0x314)
#define EDMA_TPCC_CCERRCLR		(0x31c)
#define EDMA_TPCC_DRAEM(n)		(0x340 + (n * 8))
#define EDMA_TPCC_DRAEHM(n)		(0x344 + (n * 8))
#define EDMA_TPCC_QRAEN(n)		(0x380 + (n * 4))
#define EDMA_TPCC_IESRH_RN(n)		(0x2064 + (n * 512))
#define EDMA_TPCC_ESR_RN(n)		(0x2010 + (n * 512))
#define EDMA_TPCC_ICRH_RN(n)		(0x2074 + (n * 512))
#define EDMA_TPCC_IESR_RN(n)		(0x2060 + (n * 512))
#define EDMA_TPCC_SECR_RN(n)		(0x2040 + (n * 512))
#define EDMA_TPCC_EESR_RN(n)		(0x2030 + (n * 512))
#define EDMA_TPCC_SECRH_RN(n)		(0x2044 + (n * 512))
#define EDMA_TPCC_EESRH_RN(n)		(0x2034 + (n * 512))
#define EDMA_TPCC_ICR_RN(n)		(0x2070 + (n * 512))
#define EDMA_TPCC_IPR_RN(n)		(0x2068 + (n * 512))
#define EDMA_TPCC_ESRH_RN(n)		(0x2014 + (n * 512))
#define EDMA_TPCC_OPT(n)		(0x4000 + (n * 32))

/* Field Definition Macros */
#define EDMA_TPCC_DCHMAPN_SHIFT		(5)
#define EDMA_TPCC_DMAQNUMN_SHIFT	(3)
#define EDMA_TPCC_OPT_TCC_SHIFT		(12)
#define EDMA_TPCC_OPT_TCC_MASK		(0x0003f000)
#define EDMA_TPCC_OPT_TCINTEN_MASK	(0x00100000)
#define EDMA_TPCC_OPT_SYNCDIM_MASK	(0x00000004)

/** DMAQNUM bits Clear */
#define EDMACC_DMAQNUM_CLR(ch_num)	( ~ (0x7 << (((ch_num) % 8) \
						* 4)))
/** DMAQNUM bits Set */
#define EDMACC_DMAQNUM_SET(ch_num,que_num)	((0x7 & (que_num)) << \
							(((ch_num) % 8) * 4))

/** OPT-TCC bitfield Clear */
#define EDMACC_OPT_TCC_CLR		(~EDMA_TPCC_OPT_TCC_MASK)

/** OPT-TCC bitfield Set */
#define EDMACC_OPT_TCC_SET(tcc)	(((EDMA_TPCC_OPT_TCC_MASK >> \
						EDMA_TPCC_OPT_TCC_SHIFT) & \
						(tcc)) << \
						EDMA_TPCC_OPT_TCC_SHIFT)

#define EDMA_SET_ALL_BITS		(0xFFFFFFFF)

#define EDMA_TRIG_MODE_MANUAL			(0)

/*
 * Number of PaRAM Entry fields
 * OPT, SRC, A_B_CNT, DST, SRC_DST_BIDX, LINK_BCNTRLD, SRC_DST_CIDX
 * and CCNT
 */
#define EDMACC_PARAM_ENTRY_FIELDS		(0x8)

#define SOC_EDMA_NUM_DMACH		64
#define SOC_EDMA_NUM_QDMACH		8

/**
 * @brief EDMA Parameter RAM Set in User Configurable format
 *
 * This is a mapping of the EDMA PaRAM set provided to the user
 * for ease of modification of the individual fields
 */
struct edma_param_entry {
	/** OPT field of PaRAM Set */
	u32 opt;

	/**
	 * @brief Starting byte address of Source
	 * For FIFO mode, src_addr must be a 256-bit aligned address.
	 */
	u32 src_addr;

	/**
	 * @brief Number of bytes in each Array (ACNT)
	 */
	u16 a_cnt;

	/**
	 * @brief Number of Arrays in each Frame (BCNT)
	 */
	u16 b_cnt;

	/**
	 * @brief Starting byte address of destination
	 * For FIFO mode, dest_addr must be a 256-bit aligned address.
	 * i.e. 5 LSBs should be 0.
	 */
	u32 dest_addr;

	/**
	 * @brief Index between consec. arrays of a Source Frame (SRCBIDX)
	 */
	s16 src_bidx;

	/**
	 * @brief Index between consec. arrays of a Destination Frame (DSTBIDX)
	 */
	s16 dest_bidx;

	/**
	 * @brief Address for linking (AutoReloading of a PaRAM Set)
	 * This must point to a valid aligned 32-byte PaRAM set
	 * A value of 0xFFFF means no linking
	 */
	u16 link_addr;

	/**
	 * @brief Reload value of the numArrInFrame (BCNT)
	 * Relevant only for A-sync transfers
	 */
	u16 b_cnt_reload;

	/**
	 * @brief Index between consecutive frames of a Source Block (SRCCIDX)
	 */
	s16 src_cidx;

	/**
	 * @brief Index between consecutive frames of a Dest Block (DSTCIDX)
	 */
	s16 dest_cidx;

	/**
	 * @brief Number of Frames in a block (CCNT)
	 */
	u16 c_cnt;
} __packed;

void edma_init(u32 que_num);
bool edma_request_channel(u32 ch_num, u32 tcc_num, u32 evt_qnum);
void edma_set_param(u32 param_id, struct edma_param_entry* new_param);
bool edma_enable_transfer(u32 ch_num);
u32 edma_get_intr_status(void);
void edma_clr_intr(u32 value);
#endif
