/*
 * Texas Instruments System Control Interface (TISCI) Protocol
 *
 * Communication protocol with TI SCI hardware
 * The system works in a message response protocol
 * See: http://processors.wiki.ti.com/index.php/TISCI for details
 *
 * Copyright (C)  2018 Texas Instruments Incorporated - http://www.ti.com/
 * Based on drivers/firmware/ti_sci.h from Linux.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __TI_SCI_H
#define __TI_SCI_H

/* Generic Messages */
#define TI_SCI_MSG_ENABLE_WDT		0x0000
#define TI_SCI_MSG_WAKE_RESET		0x0001
#define TI_SCI_MSG_VERSION		0x0002
#define TI_SCI_MSG_WAKE_REASON		0x0003
#define TI_SCI_MSG_GOODBYE		0x0004
#define TI_SCI_MSG_SYS_RESET		0x0005
#define TI_SCI_MSG_BOARD_CONFIG		0x000b

/**
 * struct ti_sci_msg_hdr - Generic Message Header for All messages and responses
 * @type:	Type of messages: One of TI_SCI_MSG* values
 * @host:	Host of the message
 * @seq:	Message identifier indicating a transfer sequence
 * @flags:	Flag for the message
 */
struct ti_sci_msg_hdr {
	u16 type;
	u8 host;
	u8 seq;
#define TI_SCI_MSG_FLAG(val)			(1 << (val))
#define TI_SCI_FLAG_REQ_GENERIC_NORESPONSE	0x0
#define TI_SCI_FLAG_REQ_ACK_ON_RECEIVED		TI_SCI_MSG_FLAG(0)
#define TI_SCI_FLAG_REQ_ACK_ON_PROCESSED	TI_SCI_MSG_FLAG(1)
#define TI_SCI_FLAG_RESP_GENERIC_NACK		0x0
#define TI_SCI_FLAG_RESP_GENERIC_ACK		TI_SCI_MSG_FLAG(1)
	/* Additional Flags */
	u32 flags;
} __packed;

#endif /* __TI_SCI_H */
