/**
 * @file tcmi_exit_procmsg.c - TCMI exit message, send by guest task from PEN
 *       
 *                      
 * 
 *
 *
 * Date: 04/27/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_exit_procmsg.c,v 1.3 2007/09/02 10:54:25 stavam2 Exp $
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

#define TCMI_EXIT_PROCMSG_PRIVATE
#include "tcmi_exit_procmsg.h"


#include <dbg.h>



/** 
 * \<\public\>\> Exit message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID - used for verification
 * instance.
 * @return a new error message or NULL.
 */
struct tcmi_msg* tcmi_exit_procmsg_new_rx(u_int32_t msg_id)
{
	struct tcmi_exit_procmsg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_EXIT_PROCMSG_ID) {
		mdbg(ERR3, "Factory specified message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_EXIT_PROCMSG_ID);
		goto exit0;
	}
	/* Allocate the instance */
	if (!(msg = TCMI_EXIT_PROCMSG(kmalloc(sizeof(struct tcmi_exit_procmsg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate test request message");
		goto exit0;
	}
	/* Initialized the message for receiving. */
	if (tcmi_procmsg_init_rx(TCMI_PROCMSG(msg), TCMI_EXIT_PROCMSG_ID, &exit_procmsg_ops)) {
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
 * \<\public\>\> Exit message tx constructor.
 *
 * The exit message has no transaction associated. The user specifies
 * the desired exit code and the destination PID only.
 *
 *
 * @param dst_pid - destination process PID
 * @param code - exit code 
 * @return a new error ready for the transfer or NULL.
 */
struct tcmi_msg* tcmi_exit_procmsg_new_tx(pid_t dst_pid, int32_t code)
{
	struct tcmi_exit_procmsg *msg;

	if (!(msg = TCMI_EXIT_PROCMSG(kmalloc(sizeof(struct tcmi_exit_procmsg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate test request message");
		goto exit0;
	}

	/* Initialize the message for transfer, no transaction to be
	 * created, no response expected, no timeout for response, not a reply to any transaction */
	if (tcmi_procmsg_init_tx(TCMI_PROCMSG(msg), TCMI_EXIT_PROCMSG_ID, &exit_procmsg_ops, 
				 dst_pid, 0,
				 NULL, 0, 
				 0, TCMI_TRANSACTION_INVAL_ID)) {
		mdbg(ERR3, "Error initializing test request message message");
		goto exit1;
	}
	msg->code = code;
	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}


/** @addtogroup tcmi_exit_procmsg_class
 *
 * @{
 */

/**
 * \<\<private\>\> Receives the message via a specified connection. 
 * Receiving the exit code requires reading the error code from the specified
 * connection 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully received.
 */
static int tcmi_exit_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_exit_procmsg *self_msg = TCMI_EXIT_PROCMSG(self);

	err = kkc_sock_recv(sock, &self_msg->code, 
			    sizeof(self_msg->code), KKC_SOCK_BLOCK);
	mdbg(INFO3, "Received exit code: %lu", (long)self_msg->code);
	
	return err;
}

/**
 * \<\<private\>\> Sends the exit code via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully sent.
 */
static int tcmi_exit_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_exit_procmsg *self_msg = TCMI_EXIT_PROCMSG(self); 

	err = kkc_sock_send(sock, &self_msg->code, 
			    sizeof(self_msg->code), KKC_SOCK_BLOCK);

	mdbg(INFO3, "Sent exit code: %lu", (long)self_msg->code);

	return err;
}



/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops exit_procmsg_ops = {
	.recv = tcmi_exit_procmsg_recv,
	.send = tcmi_exit_procmsg_send
};


/**
 * @}
 */

