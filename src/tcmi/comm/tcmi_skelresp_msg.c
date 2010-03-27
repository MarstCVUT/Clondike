/**
 * @file tcmi_skelresp_msg.c - TCMI error message - carries an error status code.
 *       
 *                      
 * 
 *
 *
 * Date: 04/11/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_skelresp_msg.c,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2005  Jan Capek
 * 
 * TCMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * TCMI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <linux/slab.h>

#include "tcmi_transaction.h"

#define TCMI_SKELRESP_MSG_PRIVATE
#include "tcmi_skelresp_msg.h"


#include <dbg.h>



/** 
 * \<\<public\>\> Skeleton response message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID that will be used for this error message
 * instance.
 * @return a new error message or NULL.
 */
struct tcmi_msg* tcmi_skelresp_msg_new_rx(u_int32_t msg_id)
{
	struct tcmi_skelresp_msg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_SKELRESP_MSG_ID) {
		mdbg(ERR3, "Factory specified message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_SKELRESP_MSG_ID);
		goto exit0;
	}
	if (!(msg = TCMI_SKELRESP_MSG(kmalloc(sizeof(struct tcmi_skelresp_msg), 
					    GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate testing message");
		goto exit0;
	}
	/* Initialized the message for receiving, message ID is extended with error flags */
	if (tcmi_msg_init_rx(TCMI_MSG(msg), TCMI_SKELRESP_MSG_ID, &skelresp_msg_ops)) {
		mdbg(ERR3, "Error initializing testing message %x", msg_id);
		goto exit1;
	}

	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
}

/** 
 * \<\<public\>\> Skeleton response message tx constructor.
 *
 * The testing response message is a one-way message, no transaction
 * is needs to be started, the user only needs to specify the
 * transaction ID that it is the reply to.
 *
 * When performing the generic tx init, the response message ID is
 * specified (TCMI_SKELRESP_MSG_ID)
 *
 * @param trans_id - transaction ID that this message is replying to.
 * @return a new test response message for the transfer or NULL.
 */
struct tcmi_msg* tcmi_skelresp_msg_new_tx(u_int32_t trans_id)
{
	struct tcmi_skelresp_msg *msg;

	if (!(msg = TCMI_SKELRESP_MSG(kmalloc(sizeof(struct tcmi_skelresp_msg), 
					    GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate testing message");
		goto exit0;
	}

	/* Initialize the message for transfer, no transaction
	 * required, no timout, no response ID */
	if (tcmi_msg_init_tx(TCMI_MSG(msg), TCMI_SKELRESP_MSG_ID, &skelresp_msg_ops, 
			     NULL, 0, 0, trans_id)) {
		mdbg(ERR3, "Error initializing testing response message");
		goto exit1;
	}
	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}


/** @addtogroup tcmi_skelresp_msg_class
 *
 * @{
 */

/**
 * \<\<private\>\> Receives the message via a specified connection. 
 * Receiving the erro message requires reading the error code from the specified
 * connection 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully received.
 */
static int tcmi_skelresp_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock)
{
	/* struct tcmi_skelresp_msg *self_msg = TCMI_SKELRESP_MSG(self); */
	mdbg(INFO3, "Testing response message received");
	
	return 0;
}

/**
 * \<\<private\>\> Receives the message via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for sending message data
 * @return 0 when successfully sent.
 */
static int tcmi_skelresp_msg_send(struct tcmi_msg *self, struct kkc_sock *sock)
{
	/* struct tcmi_skelresp_msg *self_msg = TCMI_SKELRESP_MSG(self); */
	mdbg(INFO3, "Testing response message sent");

	return 0;
}



/** Message operations that support polymorphism. */
static struct tcmi_msg_ops skelresp_msg_ops = {
	.recv = tcmi_skelresp_msg_recv,
	.send = tcmi_skelresp_msg_send
};


/**
 * @}
 */

