/**
 * @file tcmi_guest_started_procmsg.c - response from the guest task when it is succesfully started
 *       
 *                      
 * 
 *
 *
 * Date: 04/27/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_guest_started_procmsg.c,v 1.3 2007/09/02 10:54:25 stavam2 Exp $
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

#define TCMI_GUEST_STARTED_PROCMSG_PRIVATE
#include "tcmi_guest_started_procmsg.h"


#include <dbg.h>



/** 
 * \<\<public\>\> Guest started message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID, to check with the actual response
 * process message ID.
 *
 * @return a new guest started process message or NULL.
 */
struct tcmi_msg* tcmi_guest_started_procmsg_new_rx(u_int32_t msg_id)
{
	struct tcmi_guest_started_procmsg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_GUEST_STARTED_PROCMSG_ID) {
		mdbg(ERR3, "Factory specified process message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_GUEST_STARTED_PROCMSG_ID);
		goto exit0;
	}
	if (!(msg = TCMI_GUEST_STARTED_PROCMSG(kmalloc(sizeof(struct tcmi_guest_started_procmsg), 
						      GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate guest started process message");
		goto exit0;
	}
	/* initialize the message for receiving */
	if (tcmi_procmsg_init_rx(TCMI_PROCMSG(msg), TCMI_GUEST_STARTED_PROCMSG_ID, 
				 &guest_started_procmsg_ops)) {
		mdbg(ERR3, "Error initializing process message %x", msg_id);
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
 * \<\<public\>\> Guest started process message tx constructor.
 *
 * The response message requires specifying the local PID(guest PID)
 * of the sender in addition to the destination PID used for routing.
 *
 * @param trans_id - transaction ID that this message is replying to.
 * @param dst_pid - destination process PID
 * @param guest_pid - PID of the sending process
 * @return a new test response message for the transfer or NULL.
 */
struct tcmi_msg* tcmi_guest_started_procmsg_new_tx(u_int32_t trans_id, pid_t dst_pid, 
						  pid_t guest_pid)
{
	struct tcmi_guest_started_procmsg *msg;

	if (!(msg = TCMI_GUEST_STARTED_PROCMSG(kmalloc(sizeof(struct tcmi_guest_started_procmsg), 
						  GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate testing message");
		goto exit0;
	}

	/* Initialize the message for transfer, no transaction
	 * required, no timout, no response ID */
	if (tcmi_procmsg_init_tx(TCMI_PROCMSG(msg), TCMI_GUEST_STARTED_PROCMSG_ID, &guest_started_procmsg_ops,
				 dst_pid, 0,
				 NULL, 0, 0, trans_id)) {
		mdbg(ERR3, "Error initializing testing response message");
		goto exit1;
	}
	msg->guest_pid = guest_pid;
	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}


/** @addtogroup tcmi_guest_started_procmsg_class
 *
 * @{
 */

/**
 * \<\<private\>\> Receives the message via a specified connection. 
 * Receiving the guest started process message requires reading guest PID from the
 * specified connection.
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully received.
 */
static int tcmi_guest_started_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_guest_started_procmsg *self_msg = TCMI_GUEST_STARTED_PROCMSG(self);

	err = kkc_sock_recv(sock, &self_msg->guest_pid, 
			    sizeof(self_msg->guest_pid), KKC_SOCK_BLOCK);

	mdbg(INFO3, "Received guest PID %d", self_msg->guest_pid);
	
	return err;
}

/**
 * \<\<private\>\> Sends the message via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully sent.
 */
static int tcmi_guest_started_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_guest_started_procmsg *self_msg = TCMI_GUEST_STARTED_PROCMSG(self);

	err = kkc_sock_send(sock, &self_msg->guest_pid, 
			    sizeof(self_msg->guest_pid), KKC_SOCK_BLOCK);

	mdbg(INFO3, "Sent guest PID %d", self_msg->guest_pid);

	return err;
}



/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops guest_started_procmsg_ops = {
	.recv = tcmi_guest_started_procmsg_recv,
	.send = tcmi_guest_started_procmsg_send
};


/**
 * @}
 */

