/**
 * @file tcmi_skel_procmsg.c - TCMI skeleton message - example message implementation.
 *       
 *                      
 * 
 *
 *
 * Date: 04/19/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_skel_procmsg.c,v 1.3 2007/09/02 10:54:25 stavam2 Exp $
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

#define TCMI_SKEL_PROCMSG_PRIVATE
#include "tcmi_skel_procmsg.h"


#include <dbg.h>



/** 
 * \<\<public\>\> Skeleton message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID that will be used for this error message
 * instance.
 * @return a new error message or NULL.
 */
struct tcmi_msg* tcmi_skel_procmsg_new_rx(u_int32_t msg_id)
{
	struct tcmi_skel_procmsg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_SKEL_PROCMSG_ID) {
		mdbg(ERR3, "Factory specified message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_SKEL_PROCMSG_ID);
		goto exit0;
	}
	/* Allocate the instance */
	if (!(msg = TCMI_SKEL_PROCMSG(kmalloc(sizeof(struct tcmi_skel_procmsg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate test request message");
		goto exit0;
	}
	/* Initialized the message for receiving. */
	if (tcmi_procmsg_init_rx(TCMI_PROCMSG(msg), TCMI_SKEL_PROCMSG_ID, &skel_procmsg_ops)) {
		mdbg(ERR3, "Error initializing test request message %x", msg_id);
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
 * \<\<public\>\> Skeleton message tx constructor.
 *
 * The testing message can have a transaction associated with it =
 * there is a response expected for this message by the
 * sender. Therefore, the user specifies the transaction slot vector.
 *
 * When performing the generic proc tx init, the response message ID is
 * specified (TCMI_RESPSKEL_PROCMSG_ID) so that it will be associated with the
 * transaction. Also, the generic tx initializer is supplied with the destination
 * process ID.
 *
 * @param *transactions - storage for the new transaction(NULL for one-way messages)
 * @param dst_pid - destination process PID
 * @return a new error ready for the transfer or NULL.
 */
struct tcmi_msg* tcmi_skel_procmsg_new_tx(struct tcmi_slotvec *transactions, pid_t dst_pid)
{
	struct tcmi_skel_procmsg *msg;

	if (!(msg = TCMI_SKEL_PROCMSG(kmalloc(sizeof(struct tcmi_skel_procmsg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate test request message");
		goto exit0;
	}

	/* Initialize the message for transfer */
	if (tcmi_procmsg_init_tx(TCMI_PROCMSG(msg), TCMI_SKEL_PROCMSG_ID, &skel_procmsg_ops, 
				 dst_pid, 0,
				 transactions, TCMI_SKELRESP_PROCMSG_ID,
				 TCMI_SKEL_PROCMSGTIMEOUT, TCMI_TRANSACTION_INVAL_ID)) {
		mdbg(ERR3, "Error initializing test request message message");
		goto exit1;
	}
	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}


/** @addtogroup tcmi_skel_procmsg_class
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
static int tcmi_skel_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	/* struct tcmi_skel_procmsg *self_msg = TCMI_SKEL_PROCMSG(self); */  
	mdbg(INFO2, "Test request process message received");
	
	return 0;
}

/**
 * \<\<private\>\> Receives the message via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for sending message data
 * @return 0 when successfully sent.
 */
static int tcmi_skel_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	/* struct tcmi_skel_procmsg *self_msg = TCMI_SKEL_PROCMSG(self); */
	mdbg(INFO2, "Test request process message sent");

	return 0;
}



/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops skel_procmsg_ops = {
	.recv = tcmi_skel_procmsg_recv,
	.send = tcmi_skel_procmsg_send
};


/**
 * @}
 */

