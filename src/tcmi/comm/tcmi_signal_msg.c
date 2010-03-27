/**
 * @file tcmi_signal_msg.c - TCMI skeleton message - example message implementation.
 *       
 *                      
 * 
 *
 *
 * Date: 04/11/2005
 *
 * Author: Petr Malat 
 *
 * $Id: tcmi_signal_msg.c,v 1.2 2007/09/02 10:54:25 stavam2 Exp $
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

#define TCMI_SIGNAL_MSG_PRIVATE
#include "tcmi_signal_msg.h"


#include <dbg.h>



/** 
 * \<\<public\>\> Skeleton message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID that will be used for this error message
 * instance.
 * @return a new error message or NULL.
 */
struct tcmi_msg* tcmi_signal_msg_new_rx(u_int32_t msg_id)
{
	struct tcmi_signal_msg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_SIGNAL_MSG_ID) {
		mdbg(ERR3, "Factory specified message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_SIGNAL_MSG_ID);
		goto exit0;
	}
	/* Allocate the instance */
	if (!(msg = TCMI_SIGNAL_MSG(kmalloc(sizeof(struct tcmi_signal_msg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate signal message");
		goto exit0;
	}
	/* Initialized the message for receiving. */
	if (tcmi_msg_init_rx(TCMI_MSG(msg), TCMI_SIGNAL_MSG_ID, &skel_msg_ops)) {
		mdbg(ERR3, "Error initializing signal message %x", msg_id);
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
 * The skeleton message can have a transaction associated with it =
 * there is a response expected for this message by the
 * sender. Therefore, the user specifies the transaction slot vector.
 *
 * When performing the generic tx init, the response message ID is
 * specified (TCMI_RESPSIGNAL_MSG_ID) so that it will be associated with the
 * transaction
 *
 * @param *transactions - storage for the new transaction(NULL for one-way messages)
 * @return a new error ready for the transfer or NULL.
 */
struct tcmi_msg* tcmi_signal_msg_new_tx(struct tcmi_slotvec *transactions)
{
	struct tcmi_signal_msg *msg;

	if (!(msg = TCMI_SIGNAL_MSG(kmalloc(sizeof(struct tcmi_signal_msg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate signal message");
		goto exit0;
	}

	/* Initialize the message for transfer */
	if (tcmi_msg_init_tx(TCMI_MSG(msg), TCMI_SIGNAL_MSG_ID, &skel_msg_ops, 
			     transactions, 0,//TCMI_SIGNALRESP_MSG_ID,
			     TCMI_SIGNAL_MSGTIMEOUT, TCMI_TRANSACTION_INVAL_ID)) {
		mdbg(ERR3, "Error initializing signal message");
		goto exit1;
	}
	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}


/** @addtogroup tcmi_signal_msg_class
 *
 * @{
 */

/**
 * \<\<private\>\> Receives the message via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully received.
 */
static int tcmi_signal_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock)
{
	struct tcmi_signal_msg *self_msg = TCMI_SIGNAL_MSG(self);   
	int err;

	err = kkc_sock_recv(sock, &self_msg->pid, 
			    sizeof(self_msg->pid) + sizeof(self_msg->info), KKC_SOCK_BLOCK);

	mdbg(INFO2, "Signal message received(%d)", err);
	
	return 0;
}

/**
 * \<\<private\>\> Receives the message via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for sending message data
 * @return 0 when successfully sent.
 */
static int tcmi_signal_msg_send(struct tcmi_msg *self, struct kkc_sock *sock)
{
	struct tcmi_signal_msg *self_msg = TCMI_SIGNAL_MSG(self);   
	int err;

	err = kkc_sock_send(sock, &self_msg->pid, 
			    sizeof(self_msg->pid) + sizeof(self_msg->info), KKC_SOCK_BLOCK);

	mdbg(INFO2, "Signal message send(%d)", err);
	

	return 0;
}

struct tcmi_msg *tcmi_signal_msg_new(pid_t pid, siginfo_t *info)
{
	struct tcmi_msg *self;

	self = tcmi_signal_msg_new_tx(NULL);

	if(self){
		TCMI_SIGNAL_MSG(self)->pid = pid;
		memcpy(&TCMI_SIGNAL_MSG(self)->info, info, sizeof(siginfo_t));
	}
	return self;
}

/** Message operations that support polymorphism. */
static struct tcmi_msg_ops skel_msg_ops = {
	.recv = tcmi_signal_msg_recv,
	.send = tcmi_signal_msg_send
};


/**
 * @}
 */

