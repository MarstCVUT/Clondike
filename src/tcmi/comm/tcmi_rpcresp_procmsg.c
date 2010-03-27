/**
 * @file tcmi_rpcresp_procmsg.c - TCMI error message - carries an error status code.
 *       
 *                      
 * 
 *
 *
 * Date: 20/04/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_rpcresp_procmsg.c,v 1.6 2007/10/11 21:00:26 stavam2 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2007 Petr Malat
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
#include <linux/vmalloc.h>
#include <dbg.h>
#include <tcmi/task/tcmi_task.h>
#include "tcmi_transaction.h"
#include "tcmi_rpc_procmsg.h"

#define TCMI_RPCRESP_PROCMSG_PRIVATE
#include "tcmi_rpcresp_procmsg.h"


#define SIZE_OF_ELEM_SIZE (self_msg->nmemb*sizeof(*(self_msg->elem_size)))
#define SIZE_OF_ELEM_FREE (self_msg->nmemb*sizeof(*(self_msg->elem_free)))
#define SIZE_OF_ELEM_BASE (self_msg->nmemb*sizeof(*(self_msg->elem_base)))



/** 
 * \<\<public\>\> RPC response message rx constructor.
 *
 * @param msg_id - message ID, to check with the actual response
 * process message ID.
 *
 * @return a new rpc response process message or NULL.
 */
struct tcmi_msg* tcmi_rpcresp_procmsg_new_rx(u_int32_t msg_id)
{
	struct tcmi_rpcresp_procmsg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_RPCRESP_PROCMSG_ID) {
		mdbg(ERR3, "Factory specified process message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_RPCRESP_PROCMSG_ID);
		goto exit0;
	}
	if (!(msg = TCMI_RPCRESP_PROCMSG(kmalloc(sizeof(struct tcmi_rpcresp_procmsg), 
						  GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate RPC process message");
		goto exit0;
	}
	msg->elem_free = NULL;
	/* Initialized the message for receiving, message ID is extended with error flags */
	if (tcmi_procmsg_init_rx(TCMI_PROCMSG(msg), TCMI_RPCRESP_PROCMSG_ID, 
				 &rpcresp_procmsg_ops)) {
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
 * \<\<public\>\> Create response to rpc message
 *
 * @param m - message we are responding to
 * @param rtn_code - value returned by RPC
 * @param ... - size of the first arg., pointer to the first arg., size of the second arg. 
 * pointer to the second arg. etc... ended by 0
 *
 * @return a new rpc response process message or NULL.
 */
struct tcmi_msg* tcmi_rpcresp_procmsg_create(struct tcmi_msg *m, long rtn_code, ...){
	struct tcmi_rpcresp_procmsg *self_msg;
	u_int16_t nmemb, i;
	va_list args;

	if ( !(self_msg = TCMI_RPCRESP_PROCMSG( tcmi_rpcresp_procmsg_new_tx(tcmi_msg_req_id(m), 
						tcmi_task_remote_pid(current->tcmi.tcmi_task) ))) ){
		mdbg(ERR3, "Error creating RPC response message");
		return NULL;
	}

	va_start(args, rtn_code);
	for(nmemb = 0; nmemb < TCMI_RPC_MAXPARAMS && va_arg(args, size_t) != 0; nmemb++ ) {
		void* arg = va_arg(args, void*);
		mdbg(INFO3, "Arg addr: %p", arg);
		if ( arg == NULL ) {
			mdbg(ERR3, "Null argument[%d] in RPC!", nmemb);
			return NULL;
		}
	}	
	va_end(args);
	mdbg(INFO3, "creating RPC response message nmemb: %u, rtn: %ld", nmemb, rtn_code);

	self_msg->nmemb = nmemb;
	self_msg->rpcnum = tcmi_rpc_procmsg_num( TCMI_RPC_PROCMSG(m) );
	self_msg->rtn_code = rtn_code;

	if(nmemb > 0){
		if( (self_msg->elem_free = kmalloc(SIZE_OF_ELEM_SIZE + 
				SIZE_OF_ELEM_BASE + SIZE_OF_ELEM_FREE, GFP_KERNEL)) == NULL){
			mdbg(ERR3, "Error creating RPC response message");
			return NULL;
		}
		self_msg->elem_size = (void*)(self_msg->elem_free) + SIZE_OF_ELEM_FREE;
		self_msg->elem_base = (void*)(self_msg->elem_size) + SIZE_OF_ELEM_SIZE;

		va_start(args, rtn_code);
		for(i = 0; i < nmemb; i++ ){
			self_msg->elem_size[i] = (unsigned long)va_arg(args, size_t);
			self_msg->elem_base[i] = va_arg(args, void*);
			self_msg->elem_free[i] = TCMI_RPCRESP_PROCMSG_DONTFREE;
			mdbg(INFO3, "Adding parametr #%d: at %p size %lu", i, self_msg->elem_base[i], (unsigned long)self_msg->elem_size[i] );
		}
		va_end(args);
	}


	return TCMI_MSG(self_msg);
}

/** 
 * \<\<public\>\> Create and send rpc_proc_msg and return response
 *
 * @param rpcnum - RPC number
 * @param ... - size of the first arg., pointer to the first arg., size of the second arg. 
 * pointer to the second arg. etc... ended by 0
 *
 * @return a new rpc response process message or NULL.
 */
struct tcmi_msg* tcmi_rpcresp_procmsg_get_response(unsigned int rpcnum, ...){
	struct tcmi_msg *m, *r = NULL;
	va_list args;
	size_t arg_size;
	u_int32_t elem_size[TCMI_RPC_MAXPARAMS];
	//unsigned long elem_free[TCMI_RPC_MAXPARAMS];
       	void* elem_base[TCMI_RPC_MAXPARAMS];
	u_int16_t nmemb;
	struct tcmi_task* task;
	
	task = current->tcmi.tcmi_task;
	va_start(args, rpcnum);

	if ( (m = tcmi_rpc_procmsg_new_tx( tcmi_task_transactions(task), 
					tcmi_task_remote_pid(task) )) == NULL ) 
	{
		mdbg(ERR3, "Can't create rpc message");
		goto exit0;
	}

	for(nmemb = 0; nmemb < TCMI_RPC_MAXPARAMS && 
			(arg_size = va_arg(args, size_t)) != 0; nmemb++ ){
		elem_size[nmemb] = arg_size;
		elem_base[nmemb] = va_arg(args, void*);
		//elem_free[nmemb] = TCMI_RPCRESP_PROCMSG_DONTFREE;
		mdbg(INFO3, "Adding parametr #%d: at %p size %lu", nmemb, elem_base[nmemb], (unsigned long)elem_size[nmemb] );
	}	

	TCMI_RPC_PROCMSG(m)->rpcnum = rpcnum;
	TCMI_RPC_PROCMSG(m)->nmemb = nmemb;
	TCMI_RPC_PROCMSG(m)->elem_size = elem_size;
	TCMI_RPC_PROCMSG(m)->elem_base = elem_base;
	//TCMI_RPC_PROCMSG(m)->elem_free = elem_free;
	
	tcmi_task_send_and_receive_msg(task, m, &r);

	TCMI_RPC_PROCMSG(m)->elem_size = NULL;/* Prevent calling free on staticaly allocated resources*/
	TCMI_RPC_PROCMSG(m)->free_data = false;
	TCMI_RPC_PROCMSG(m)->nmemb = 0;

	tcmi_msg_put(m);

exit0:  va_end(args);
	return r;
}
/** 
 * \<\<public\>\> RPC response process message tx constructor.
 *
 * @param trans_id - transaction ID that this message is replying to.
 * @param dst_pid - destination process PID
 * @return a new test response message for the transfer or NULL.
 */
struct tcmi_msg* tcmi_rpcresp_procmsg_new_tx(u_int32_t trans_id, pid_t dst_pid)
{
	struct tcmi_rpcresp_procmsg *msg;

	if (!(msg = TCMI_RPCRESP_PROCMSG(kmalloc(sizeof(struct tcmi_rpcresp_procmsg), 
						  GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate RPC response message");
		goto exit0;
	}
	msg->nmemb = 0;
	msg->elem_free = NULL;
	/* Initialize the message for transfer, no transaction
	 * required, no timout, no response ID */
	if (tcmi_procmsg_init_tx(TCMI_PROCMSG(msg), TCMI_RPCRESP_PROCMSG_ID, &rpcresp_procmsg_ops,
				 dst_pid, 0,
				 NULL, 0, 0, trans_id)) {
		mdbg(ERR3, "Error initializing RPC response message");
		goto exit1;
	}
	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}


/** @addtogroup tcmi_rpcresp_procmsg_class
 *
 * @{
 */

/**
 * \<\<private\>\> Receives the message via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully received.
 *
 * WARNING: Memory for data is allocated at one time, so when freeing free() must be called 
 * on elem_base[0]. It's done automatically in last mdg_put();
 */
static int tcmi_rpcresp_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	/* struct tcmi_rpcresp_procmsg *self_msg = TCMI_RPCRESP_PROCMSG(self); */
	int err, data_size, total, i;
	struct tcmi_rpcresp_procmsg *self_msg = TCMI_RPCRESP_PROCMSG(self);

	err = kkc_sock_recv(sock, &self_msg->rpcnum, 
			    sizeof(self_msg->rpcnum) + sizeof(self_msg->nmemb) + 
			    sizeof(self_msg->rtn_code), KKC_SOCK_BLOCK);
	if( err < 0 ) {
		mdbg(ERR3, "Error receiving RPC message header. Error code: %d", err);
		return err;
	}
	total = err;
	
	if( self_msg->nmemb > 0 ){
		/* Allocate memory for array of elements size and array of pointers to elements */
		if( (self_msg->elem_free = kmalloc(SIZE_OF_ELEM_SIZE + 
				SIZE_OF_ELEM_BASE + SIZE_OF_ELEM_FREE, GFP_KERNEL)) == NULL){
			mdbg(ERR3, "Can't create rpc message");
		}
		self_msg->elem_size = (void*)(self_msg->elem_free) + SIZE_OF_ELEM_FREE;
		self_msg->elem_base = (void*)(self_msg->elem_size) + SIZE_OF_ELEM_SIZE;
		self_msg->elem_base[0] = NULL;

		err = kkc_sock_recv(sock, self_msg->elem_size, SIZE_OF_ELEM_SIZE, KKC_SOCK_BLOCK);
		if( err < 0 ) {
			mdbg(ERR3, "Error receiving RPC#%d message elem size. Error code: %d", self_msg->rpcnum, err);
			 return err;
		}
		total += err;

		/* Allocate memory for data */
		for(data_size = 0, i = 0; i < self_msg->nmemb; data_size += self_msg->elem_size[i++]);
		if( (self_msg->elem_base[0] = kmalloc( data_size, GFP_KERNEL ) ) == NULL){
			mdbg(ERR3, "Can't create rpc message2");
		}
		self_msg->elem_free[0] = TCMI_RPCRESP_PROCMSG_KFREE; /* We allocated data, so we have to free it */

		err = kkc_sock_recv(sock, self_msg->elem_base[0], data_size, KKC_SOCK_BLOCK);
		if( err < 0 ) {
			mdbg(ERR3, "Error receiving RPC#%d message elements. Error code: %d", self_msg->rpcnum, err);	
			return err;
		}
		total += err;

		for( i = 1; i < self_msg->nmemb; i++ ){
			self_msg->elem_base[i] = self_msg->elem_base[i-1] + self_msg->elem_size[i-1];
			self_msg->elem_free[i] = TCMI_RPCRESP_PROCMSG_DONTFREE;
		}
	}
	mdbg(INFO3, "Received RPC#%d response message (%d Bytes)", self_msg->rpcnum, total);
	
	return 0;
}

/**
 * \<\<private\>\> Receives the message via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for sending message data
 * @return 0 when successfully sent.
 */
static int tcmi_rpcresp_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	int err, total, i;
	struct tcmi_rpcresp_procmsg *self_msg = TCMI_RPCRESP_PROCMSG(self); 

	err = kkc_sock_send(sock, &self_msg->rpcnum, 
			    sizeof(self_msg->rpcnum) + sizeof(self_msg->nmemb) + 
			    sizeof(self_msg->rtn_code), KKC_SOCK_BLOCK);
	if( err < 0 ) {
		mdbg(ERR3, "Error sending RPC#%d message header. Error code: %d", self_msg->rpcnum, err);
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
				mdbg(ERR3, "Error sending RPC#%d message element[%d]. Error code: %d", self_msg->rpcnum, i, err);
				return err;
			}
			total += err;
		}
	}
	mdbg(INFO3, "Sent RPC#%d message (%d Bytes)", self_msg->rpcnum, total);

	return 0;
}

/**
 * \<\<private\>\> Frees RPC message resources
 *
 * @param *self - this message instance
 */
static void tcmi_rpcresp_procmsg_free(struct tcmi_procmsg *self)
{
	struct tcmi_rpcresp_procmsg *self_msg = TCMI_RPCRESP_PROCMSG(self);
	int i;
	if( self_msg->nmemb ){
		for( i = 0; i < self_msg->nmemb; i++ ){
			switch( self_msg->elem_free[i] ){
				case TCMI_RPCRESP_PROCMSG_DONTFREE:
					mdbg(INFO3, "Skipping unallocate of #%d data at %p size %lu", 
							i, self_msg->elem_base[i], (unsigned long)self_msg->elem_size[i]);
					break;
				case TCMI_RPCRESP_PROCMSG_KFREE:
					mdbg(INFO3, "Freeing (kfree) data #%d at %p size %lu", 
							i, self_msg->elem_base[i], (unsigned long)self_msg->elem_size[i]);
					kfree(self_msg->elem_base[i]);
					break;
				case TCMI_RPCRESP_PROCMSG_VFREE:
					mdbg(INFO3, "Freeing (vfree) data #%d at %p size %lu", 
							i, self_msg->elem_base[i], (unsigned long)self_msg->elem_size[i]);
					vfree(self_msg->elem_base[i]);
					break;
				default:
					mdbg(ERR3, "Unknown action in %p->elem_free[%d]", self_msg, i);
			}
		}
		kfree(self_msg->elem_free);
	}
}

/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops rpcresp_procmsg_ops = {
	.recv = tcmi_rpcresp_procmsg_recv,
	.send = tcmi_rpcresp_procmsg_send,
	.free = tcmi_rpcresp_procmsg_free
};


/**
 * @}
 */

