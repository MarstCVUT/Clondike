/**
 * @file tcmi_procmsg.c - TCMI process control communication message
 *       
 *                      
 * 
 *
 *
 * Date: 04/19/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_procmsg.c,v 1.3 2007/09/02 10:54:25 stavam2 Exp $
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

#define TCMI_PROCMSG_PRIVATE
#include "tcmi_procmsg.h"


#include <dbg.h>



/** 
 * \<\<public\>\> Initializes the process message for receiving.  The
 * message operations specific to a particular process message class
 * are stored in the instance, Regular tcmi message is supplied with
 * generic proc message operations that handle the polymorphism
 * further.
 *
 * @param *self - pointer to this message instance
 * @param msg_id - ID of a particular message type
 * @param *msg_ops - pointer to a particular process message 
 * operations
 * @return 0 upon success
 */
int tcmi_procmsg_init_rx(struct tcmi_procmsg *self, u_int32_t msg_id,
			 struct tcmi_procmsg_ops *msg_ops)
{
	self->msg_ops = msg_ops;
	/* Initialized the message for receiving. */
	return tcmi_msg_init_rx(TCMI_MSG(self), msg_id, &procmsg_ops); 
}

/** 
 * \<\<public\>\> Initializes the message for transferring.  Similar
 * to rx version - the message operations specific to a particular
 * process message class are stored in the instance, Regular tcmi
 * message is supplied with generic proc message operations that
 * handle the polymorphism further.
 *
 * @param *self - pointer to this message instance
 * @param msg_id - ID of a particular message type
 * @param *msg_ops - pointer to a particular process message specific operations
 * @param dst_pid - destination process ID - filled in the header
 * @param enfoce_migmode - do we require switch to migration mode upon recieving of this message?
 * @param *transactions - storage for the new transaction(NULL for one-way messages)
 * @param resp_msg_id - response message ID - required for the transaction
 * if the transaction expires(NULL for one-way messages)
 * @param timeout - transaction time out (0 for one-way messages)
 * @param resp_trans_id - response transaction ID - used when
 * this message is a response to a particular request. The user must set
 * this ID based on the value ofr req_trans_id of the previously received message.
 * @return 0 upon success
 */
int tcmi_procmsg_init_tx(struct tcmi_procmsg *self, u_int32_t msg_id, 
			 struct tcmi_procmsg_ops *msg_ops,
			 pid_t dst_pid, u_int8_t enforce_migmode,
			 struct tcmi_slotvec *transactions, u_int32_t resp_msg_id,
			 u_int timeout,
			 u_int32_t resp_trans_id)
{
	self->dst_pid = dst_pid;
	self->msg_ops = msg_ops;
	self->enforce_migmode = enforce_migmode;
	/* Initialize the message for transfer */
	return tcmi_msg_init_tx(TCMI_MSG(self), msg_id, &procmsg_ops,
				transactions, resp_msg_id,
				timeout, resp_trans_id);
}


/** @addtogroup tcmi_procmsg_class
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
static int tcmi_procmsg_recv(struct tcmi_msg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_procmsg *self_proc = TCMI_PROCMSG(self);
	struct tcmi_procmsg_ops *ops = self_proc->msg_ops;

	if ((err = kkc_sock_recv(sock, &self_proc->dst_pid, 
				 sizeof(self_proc->dst_pid), 
				 KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive dest. PID");
		goto exit0;
	}

	if ((err = kkc_sock_recv(sock, &self_proc->enforce_migmode, 
				 sizeof(self_proc->enforce_migmode), 
				 KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive enforce migmode flag");
		goto exit0;
	}

	mdbg(INFO3, "Received process message (PID=%u)", self_proc->dst_pid);
	if (ops && ops->recv)
		err = ops->recv(self_proc, sock);

	/* error handling */
 exit0:
	return err;
}

/**
 * \<\<private\>\> Sends the message via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for sending message data
 * @return 0 when successfully sent.
 */
static int tcmi_procmsg_send(struct tcmi_msg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_procmsg *self_proc = TCMI_PROCMSG(self);
	struct tcmi_procmsg_ops *ops = self_proc->msg_ops;

	if ((err = kkc_sock_send(sock, &self_proc->dst_pid, 
				 sizeof(self_proc->dst_pid), 
				 KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send dest. PID");
		goto exit0;
	}

	if ((err = kkc_sock_send(sock, &self_proc->enforce_migmode, 
				 sizeof(self_proc->enforce_migmode), 
				 KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send enforce migmode flag");
		goto exit0;
	}

	mdbg(INFO3, "Sent process message (PID=%u)", self_proc->dst_pid);
	if (ops && ops->send)
		err = ops->send(self_proc, sock);

	/* error handling */
 exit0:
	return err;
}

/**
 * \<\<private\>\> Frees custom message resources
 * All work is delegated to a specific process message method
 *
 * @param *self - this message instance
 */
static void tcmi_procmsg_free(struct tcmi_msg *self)
{
	struct tcmi_procmsg *self_proc = TCMI_PROCMSG(self);
	struct tcmi_procmsg_ops *ops = self_proc->msg_ops;

	mdbg(INFO3, "Freeing process message (PID=%u)", self_proc->dst_pid);
	if (ops && ops->free)
		ops->free(self_proc);
}


/** Message operations that support polymorphism. */
static struct tcmi_msg_ops procmsg_ops = {
	.recv = tcmi_procmsg_recv,
	.send = tcmi_procmsg_send,
	.free = tcmi_procmsg_free
};


/**
 * @}
 */

