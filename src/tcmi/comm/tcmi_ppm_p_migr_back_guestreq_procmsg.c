/**
 * @file tcmi_ppm_p_migr_back_guestreq_procmsg.c - migrate back request initiated by guest
 *       
 *                      
 * 
 *
 *
 * Date: 05/04/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ppm_p_migr_back_guestreq_procmsg.c,v 1.3 2007/09/02 10:54:25 stavam2 Exp $
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
#define TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_PRIVATE
#include "tcmi_ppm_p_migr_back_guestreq_procmsg.h"


#include <dbg.h>



/** 
 * \<\<public\>\> PPM_P message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID that will be used for this error message
 * instance.
 * @return a new error message or NULL.
 */
struct tcmi_msg* tcmi_ppm_p_migr_back_guestreq_procmsg_new_rx(u_int32_t msg_id)
{
	struct tcmi_ppm_p_migr_back_guestreq_procmsg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_ID) {
		mdbg(ERR3, "Factory specified message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_ID);
		goto exit0;
	}
	/* Allocate the instance */
	if (!(msg = TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG(kmalloc(sizeof(struct tcmi_ppm_p_migr_back_guestreq_procmsg), 
								 GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate test request message");
		goto exit0;
	}
	/* Initialize the message for receiving. */
	if (tcmi_procmsg_init_rx(TCMI_PROCMSG(msg), TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_ID, 
				 &ppm_p_migr_back_guestreq_procmsg_ops)) {
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
 * \<\<public\>\> Guest request for migration back tx constructor.
 *
 * Generates a one-way request message that contains 
 * the checkpoint name.
 *
 *
 * @param dst_pid - PID of the target process that will receive this message
 * @param *ckpt_name - checkpoint name that will be wrapped in the message
 * @return a new error ready for the transfer or NULL.
 */
struct tcmi_msg* tcmi_ppm_p_migr_back_guestreq_procmsg_new_tx(pid_t dst_pid, char *ckpt_name)
{
	struct tcmi_ppm_p_migr_back_guestreq_procmsg *msg;

	if (!(msg = TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG(kmalloc(sizeof(struct tcmi_ppm_p_migr_back_guestreq_procmsg), 
								 GFP_KERNEL)))) {
		mdbg(ERR3, "Can't migrate back guest request message");
		goto exit0;
	}
	msg->size = strlen(ckpt_name) + 1;

	if (!(msg->ckpt_name = (char*)kmalloc(msg->size, GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate memory for checkpoint name");
		goto exit1;
	}
	strcpy(msg->ckpt_name, ckpt_name);

	/* Initialize the message for transfer, no transaction
	 * required, no timout, no response ID */
	if (tcmi_procmsg_init_tx(TCMI_PROCMSG(msg), TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_ID,
				 &ppm_p_migr_back_guestreq_procmsg_ops,
				 dst_pid, 0,
				 NULL, 0, 0, TCMI_TRANSACTION_INVAL_ID)) {
		mdbg(ERR3, "Error initializing  migrate back guest request message");
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


/** @addtogroup tcmi_ppm_p_migr_back_guestreq_procmsg_class
 *
 * @{
 */

/**
 * \<\<private\>\> Receives the message via a specified connection.
 * Receiving the message requires reading checkpoint size.  Based on
 * the size, it allocates space for the checkpoint name string and reads it
 * from the socket.
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully received.
 */
static int tcmi_ppm_p_migr_back_guestreq_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	int err = -EINVAL;
	struct tcmi_ppm_p_migr_back_guestreq_procmsg *self_msg = TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG(self);
	
	/* Receive the remote PID and checkpoint name size */	
	if ((err = kkc_sock_recv(sock, &self_msg->size, 
				 sizeof(self_msg->size), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive pid and size");
		goto exit0;
	}


	if (!(self_msg->ckpt_name = (char*)kmalloc(self_msg->size, GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate memory for checkpoint name");
		goto exit0;
	}
	
	/* Receive the checkpoint name */	
	if ((err = kkc_sock_recv(sock, self_msg->ckpt_name, 
				 self_msg->size, KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive checkpoint name");
		goto exit0;
	}
	mdbg(INFO2, "PPM_P migrate back guest request received guest PID=%d, size=%d, ckptname='%s'",
	     tcmi_procmsg_dst_pid(self), self_msg->size, self_msg->ckpt_name);

	return 0;
	/* error handling*/
 exit0:
	return err;
}

/**
 * \<\<private\>\> Sends the PPM_P migrate back guest request
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for sending message data
 * @return 0 when successfully sent.
 */
static int tcmi_ppm_p_migr_back_guestreq_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_ppm_p_migr_back_guestreq_procmsg *self_msg = TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG(self);

	
	/* Send the checkpoint name size */	
	if ((err = kkc_sock_send(sock, &self_msg->size, 
				 sizeof(self_msg->size), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send checkpoint size");
		goto exit0;
	}

	/* Send the checkpoint name */	
	if ((err = kkc_sock_send(sock, self_msg->ckpt_name, 
				 self_msg->size, KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send checkpoint name");
		goto exit0;
	}

	mdbg(INFO2, "PPM_P migrate back guest request sent guest PID=%d, size=%d, ckptname='%s'",
	     tcmi_procmsg_dst_pid(self), self_msg->size, self_msg->ckpt_name);

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
static void tcmi_ppm_p_migr_back_guestreq_procmsg_free(struct tcmi_procmsg *self)
{
	struct tcmi_ppm_p_migr_back_guestreq_procmsg *self_msg = TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG(self);

	mdbg(INFO3, "Freeing PPM guest migrate back message PID=%d, size=%d, ckptname='%s'",
	     tcmi_procmsg_dst_pid(TCMI_PROCMSG(self)), self_msg->size, self_msg->ckpt_name);
	kfree(self_msg->ckpt_name);
}


/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops ppm_p_migr_back_guestreq_procmsg_ops = {
	.recv = tcmi_ppm_p_migr_back_guestreq_procmsg_recv,
	.send = tcmi_ppm_p_migr_back_guestreq_procmsg_send,
	.free = tcmi_ppm_p_migr_back_guestreq_procmsg_free
};


/**
 * @}
 */

