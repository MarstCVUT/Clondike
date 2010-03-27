/**
 * @file tcmi_ppm_p_emigrate_msg.c - PPM_P emigration message.
 *       
 *                      
 * 
 *
 *
 * Date: 04/27/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ppm_p_emigrate_msg.c,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#include <linux/string.h>

#include "tcmi_transaction.h"

#include "tcmi_skelresp_msg.h"
#define TCMI_PPM_P_EMIGRATE_MSG_PRIVATE
#include "tcmi_ppm_p_emigrate_msg.h"


#include <dbg.h>



/** 
 * \<\<public\>\> PPM_P message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID that will be used for this error message
 * instance.
 * @return a new error message or NULL.
 */
struct tcmi_msg* tcmi_ppm_p_emigrate_msg_new_rx(u_int32_t msg_id)
{
	struct tcmi_ppm_p_emigrate_msg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_PPM_P_EMIGRATE_MSG_ID) {
		mdbg(ERR3, "Factory specified message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_PPM_P_EMIGRATE_MSG_ID);
		goto exit0;
	}
	/* Allocate the instance */
	if (!(msg = TCMI_PPM_P_EMIGRATE_MSG(kmalloc(sizeof(struct tcmi_ppm_p_emigrate_msg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate test request message");
		goto exit0;
	}
	/* Initialized the message for receiving. */
	if (tcmi_msg_init_rx(TCMI_MSG(msg), TCMI_PPM_P_EMIGRATE_MSG_ID, &ppm_p_emigrate_msg_ops)) {
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
 * \<\<public\>\> PPM_P emigration message tx constructor.
 *
 * An emigration message expects a response that will be delivered back
 * to the originating task (usually a shadow process on CCN). Therefore,
 * there has to be a transaction associated with it.
 *
 * Response message ID is TCMI_GUEST_STARTED_PROCMSG_ID.
 *
 *
 * @param *transactions - storage for the new transaction
 *
 * @param reply_pid - denotes the pid that the reply to this message
 * should be directed to. It is a key information for the receiving
 * party in order to compose a valid response
 * @param ckpt_name - checkpoint pathname - this has to be a valid location
 * in the network filesystem, so that the receiving party can restart the process
 * @return a new error ready for the transfer or NULL.
 */
struct tcmi_msg* tcmi_ppm_p_emigrate_msg_new_tx(struct tcmi_slotvec *transactions, 
						pid_t reply_pid, char *ckpt_name)
{
	struct tcmi_ppm_p_emigrate_msg *msg;

	if (!(msg = TCMI_PPM_P_EMIGRATE_MSG(kmalloc(sizeof(struct tcmi_ppm_p_emigrate_msg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate test request message");
		goto exit0;
	}
	msg->pid_and_size.reply_pid = reply_pid;
	msg->pid_and_size.size = strlen(ckpt_name) + 1;

	if (!(msg->ckpt_name = (char*)kmalloc(msg->pid_and_size.size, GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate memory for checkpoint name");
		goto exit1;
	}
	strcpy(msg->ckpt_name, ckpt_name);
	/* Initialize the message for transfer */
	if (tcmi_msg_init_tx(TCMI_MSG(msg), TCMI_PPM_P_EMIGRATE_MSG_ID, &ppm_p_emigrate_msg_ops, 
			     transactions, TCMI_GUEST_STARTED_PROCMSG_ID,
			     TCMI_PPM_P_EMIGRATE_MSGTIMEOUT, TCMI_TRANSACTION_INVAL_ID)) {
		mdbg(ERR3, "Error initializing test request message message");
		goto exit2;
	}
	return TCMI_MSG(msg);

	/* error handling */
 exit2:
	kfree(msg->ckpt_name);
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}


/** @addtogroup tcmi_ppm_p_emigrate_msg_class
 *
 * @{
 */

/**
 * \<\<private\>\> Receives the message via a specified connection.
 * Receiving the message requires reading the remote PID and
 * checkpoint size.  Based on the size, allocate space for the
 * checkpoint name string and read it to from the specified connection
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully received.
 */
static int tcmi_ppm_p_emigrate_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock)
{
	int err = -EINVAL;
	struct tcmi_ppm_p_emigrate_msg *self_msg = TCMI_PPM_P_EMIGRATE_MSG(self);
	
	/* Receive the remote PID and checkpoint name size */	
	if ((err = kkc_sock_recv(sock, &self_msg->pid_and_size, 
				 sizeof(self_msg->pid_and_size), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive pid and size");
		goto exit0;
	}


	if (!(self_msg->ckpt_name = (char*)kmalloc(self_msg->pid_and_size.size, GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate memory for checkpoint name");
		goto exit0;
	}
	
	/* Receive the checkpoint name */	
	if ((err = kkc_sock_recv(sock, self_msg->ckpt_name, 
				 self_msg->pid_and_size.size, KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive checkpoint name");
		goto exit0;
	}
	mdbg(INFO2, "PPM emigrate message received PID=%d, size=%d, ckptname='%s'",
	     self_msg->pid_and_size.reply_pid, self_msg->pid_and_size.size, self_msg->ckpt_name);

	return 0;
	/* error handling*/
 exit0:
	return err;
}

/**
 * \<\<private\>\> Sends the PPM_P emigration message
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for sending message data
 * @return 0 when successfully sent.
 */
static int tcmi_ppm_p_emigrate_msg_send(struct tcmi_msg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_ppm_p_emigrate_msg *self_msg = TCMI_PPM_P_EMIGRATE_MSG(self);

	
	/* Receive the remote PID and checkpoint name size */	
	if ((err = kkc_sock_send(sock, &self_msg->pid_and_size, 
				 sizeof(self_msg->pid_and_size), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send pid and size");
		goto exit0;
	}

	/* Receive the checkpoint name */	
	if ((err = kkc_sock_send(sock, self_msg->ckpt_name, 
				 self_msg->pid_and_size.size, KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send checkpoint name");
		goto exit0;
	}

	mdbg(INFO2, "PPM emigrate message sent PID=%d, size=%d, ckptname='%s'",
	     self_msg->pid_and_size.reply_pid, self_msg->pid_and_size.size, self_msg->ckpt_name);

	return 0;
	/* error handling */
 exit0:
	return err;
	
}

/**
 * \<\<private\>\> Frees custom message resources.
 * The checkpoint image name string is released from memory.
 *
 * @param *self - this message instance
 */
static void tcmi_ppm_p_emigrate_msg_free(struct tcmi_msg *self)
{
	struct tcmi_ppm_p_emigrate_msg *self_msg = TCMI_PPM_P_EMIGRATE_MSG(self);

	mdbg(INFO3, "Freeing PPM emigrate message PID=%d, size=%d, ckptname='%s'",
	     self_msg->pid_and_size.reply_pid, self_msg->pid_and_size.size, self_msg->ckpt_name);
	kfree(self_msg->ckpt_name);
}


/** Message operations that support polymorphism. */
static struct tcmi_msg_ops ppm_p_emigrate_msg_ops = {
	.recv = tcmi_ppm_p_emigrate_msg_recv,
	.send = tcmi_ppm_p_emigrate_msg_send,
	.free = tcmi_ppm_p_emigrate_msg_free
};


/**
 * @}
 */

