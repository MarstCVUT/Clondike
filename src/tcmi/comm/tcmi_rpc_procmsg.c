/**
 * @file tcmi_rpc_procmsg.c - TCMI rpc message.
 *       
 *                      
 * 
 *
 *
 * Date: 19/04/2007
 *
 * Author: Petr Malat
 * 
 * $Id: tcmi_rpc_procmsg.c,v 1.5 2008/01/17 14:36:44 stavam2 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2007  Petr Malat
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
#include <dbg.h>
#include "tcmi_transaction.h"

#define TCMI_RPC_PROCMSG_PRIVATE
#include "tcmi_rpc_procmsg.h"


#define SIZE_OF_ELEM_SIZE (self_msg->nmemb*sizeof(*(self_msg->elem_size)))
#define SIZE_OF_ELEM_BASE (self_msg->nmemb*sizeof(*(self_msg->elem_base)))

/** 
 * \<\<public\>\> RPC message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID that will be used for this RPC message
 * instance.
 * @return a new RPC message or NULL.
 */
struct tcmi_msg* tcmi_rpc_procmsg_new_rx(u_int32_t msg_id)
{
	struct tcmi_rpc_procmsg *msg;
	mdbg(INFO3, "in tcmi_rpc_procmsg_new_rx()");

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_RPC_PROCMSG_ID) {
		mdbg(ERR3, "Factory specified message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_RPC_PROCMSG_ID);
		goto exit0;
	}
	/* Allocate the instance */
	if (!(msg = TCMI_RPC_PROCMSG(kmalloc(sizeof(struct tcmi_rpc_procmsg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate RPC message");
		goto exit0;
	}
	/* Initialized the message for receiving. */
	if (tcmi_procmsg_init_rx(TCMI_PROCMSG(msg), TCMI_RPC_PROCMSG_ID, &rpc_procmsg_ops)) {
		mdbg(ERR3, "Error initializing RPC message %x", msg_id);
		goto exit1;
	}
	msg->free_data = 1;
	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
}

/** 
 * \<\<public\>\> RPC message tx constructor.
 *
 * When performing the generic proc tx init, the response message ID is
 * specified (TCMI_RESPRPC_PROCMSG_ID) so that it will be associated with the
 * transaction. Also, the generic tx initializer is supplied with the destination
 * process ID.
 *
 * @param *transactions - storage for the new transaction(NULL for one-way messages)
 * @param dst_pid - destination process PID
 * @return a new error ready for the transfer or NULL.
 */
struct tcmi_msg* tcmi_rpc_procmsg_new_tx(struct tcmi_slotvec *transactions, pid_t dst_pid)
{
	struct tcmi_rpc_procmsg *msg;

	if (!(msg = TCMI_RPC_PROCMSG(kmalloc(sizeof(struct tcmi_rpc_procmsg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate RPC message");
		goto exit0;
	}
	msg->free_data = 0;

	/* Initialize the message for transfer */
	if (tcmi_procmsg_init_tx(TCMI_PROCMSG(msg), TCMI_RPC_PROCMSG_ID, &rpc_procmsg_ops, 
				 dst_pid, 0,
				 transactions, TCMI_RPCRESP_PROCMSG_ID,
				 TCMI_RPC_PROCMSGTIMEOUT, TCMI_TRANSACTION_INVAL_ID)) {
		mdbg(ERR3, "Error initializing RPC message");
		goto exit1;
	}
	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}

/** @addtogroup tcmi_rpc_procmsg_class
 *
 * @{
 */

/**
 * \<\<private\>\> Receives the RPC message via a specified connection. 
 * Receiving the erro message requires reading the error code from the specified
 * connection 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully received.
 */
static int tcmi_rpc_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	int err, data_size, total, i;
	struct tcmi_rpc_procmsg *self_msg = TCMI_RPC_PROCMSG(self);

	err = kkc_sock_recv(sock, &self_msg->rpcnum, 
			    sizeof(self_msg->rpcnum) + sizeof(self_msg->nmemb), KKC_SOCK_BLOCK) ;
	if( err < 0 ) {
		mdbg(ERR3, "Cannot read rpc message header");
		return err;
	}
	total = err;
	
	if( self_msg->nmemb > 0 ){
		/* Allocate memory for array of elements size and array of pointers to elements */
		self_msg->elem_size = kmalloc(SIZE_OF_ELEM_SIZE + SIZE_OF_ELEM_BASE, GFP_KERNEL);
		self_msg->elem_base = (void*)self_msg->elem_size + SIZE_OF_ELEM_SIZE;
		self_msg->elem_base[0] = NULL;

		err = kkc_sock_recv(sock, self_msg->elem_size, SIZE_OF_ELEM_SIZE, KKC_SOCK_BLOCK);
		if( err < 0 ) {
			mdbg(ERR3, "Cannot read elem size of RPC#%d", self_msg->rpcnum);
			return err;
		}

		total += err;

		/* Allocate memory for data */
		for(data_size = 0, i = 0; i < self_msg->nmemb; data_size += self_msg->elem_size[i++]);
		self_msg->elem_base[0] = kmalloc( data_size, GFP_KERNEL );

		err = kkc_sock_recv(sock, self_msg->elem_base[0], data_size, KKC_SOCK_BLOCK);
		if( err < 0 ) {
			mdbg(ERR3, "Cannot read elements of RPC#%d (data_size %d)", self_msg->rpcnum, data_size);
			return err;
		}
		total += err;

		for( i = 1; i < self_msg->nmemb; i++ )
			self_msg->elem_base[i] = self_msg->elem_base[i-1] + self_msg->elem_size[i-1];
	}


	mdbg(INFO3, "Received: RPC#%d message (%d Bytes, %d params)", self_msg->rpcnum, total, self_msg->nmemb);
	
	return 0;
}

/**
 * \<\<private\>\> Sends the RPC message via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for sending message data
 * @return 0 when successfully sent.
 */
static int tcmi_rpc_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	int err, total, i;
	struct tcmi_rpc_procmsg *self_msg = TCMI_RPC_PROCMSG(self); 

	err = kkc_sock_send(sock, &self_msg->rpcnum, 
			    sizeof(self_msg->rpcnum) + sizeof(self_msg->nmemb), KKC_SOCK_BLOCK) ;
	if( err < 0 ) {
		mdbg(ERR3, "Error sending RPC#%d message. Error code: %d", self_msg->rpcnum, err);
		return err;
	}

	total = err;

	if( self_msg->nmemb > 0 ){
		err = kkc_sock_send(sock, self_msg->elem_size, SIZE_OF_ELEM_SIZE, KKC_SOCK_BLOCK);
		if( err < 0 ) {
			mdbg(ERR3, "Error sending RPC#%d message elem size. Error code: %d", self_msg->rpcnum, err);
			return err;
		}
		total += err;

		for( i = 0; i < self_msg->nmemb; i++ ){
			err = kkc_sock_send(sock, self_msg->elem_base[i],
					self_msg->elem_size[i], KKC_SOCK_BLOCK);

			if( err < 0 ) {
				mdbg(ERR3, "Error sending RPC#%d message elem[%d]. Error code: %d", self_msg->rpcnum, i, err);	
				return err;
			}
			total += err;
		}
	}
	mdbg(INFO3, "Sent: RPC#%d message (%d Bytes)", self_msg->rpcnum, total);

	return 0;
}

/**
 * \<\<private\>\> Frees RPC message resources
 *
 * @param *self - this message instance
 */
static void tcmi_rpc_procmsg_free(struct tcmi_procmsg *self)
{
	struct tcmi_rpc_procmsg *self_msg = TCMI_RPC_PROCMSG(self);
	if ( !self_msg )
		return;

	if( self_msg->nmemb > 0 ){
		if( self_msg->free_data == 1 ) kfree( self_msg->elem_base[0] ); /* frees all data */
		kfree( self_msg->elem_size );    /* elem_base and elem_size are allocated in one time */
	}
}


/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops rpc_procmsg_ops = {
	.recv = tcmi_rpc_procmsg_recv,
	.send = tcmi_rpc_procmsg_send,
	.free = tcmi_rpc_procmsg_free
};


/**
 * @}
 */

