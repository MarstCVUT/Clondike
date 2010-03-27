/**
 * @file tcmi_rpc_procmsg.h - TCMI rpc process communication message - used for rpcs forwarding.
 *       
 *                      
 * 
 *
 *
 * Date: 19/04/2007
 *
 * Author: Petr Malat 
 *
 * $Id: tcmi_rpc_procmsg.h,v 1.4 2007/09/03 01:17:58 malatp1 Exp $
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
#ifndef _TCMI_RPC_PROCMSG_H
#define _TCMI_RPC_PROCMSG_H

#include "tcmi_procmsg.h"

/** @defgroup tcmi_rpc_procmsg_class tcmi_rpc_procmsg class
 *
 * @ingroup tcmi_procmsg_class
 *
 * @relates tcmi_rpc_class
 *
 * This class represents RPC process control message and is intended to be used
 * for system call forwarding .
 *
 * @{
 */

/** Compound structure, inherits from tcmi_procmsg_class */
struct tcmi_rpc_procmsg {
	/** parent class instance */
	struct tcmi_procmsg super;
	/** RPC number */
	u_int16_t rpcnum;
	/** Number of memory elements */
	u_int16_t nmemb;
	/** Array with size of elements */
	u_int32_t *elem_size;
	/** Array of pointers to elements */ 
	void **elem_base;
	/** Free data on put */
	bool free_data;  // We don't send this
} __attribute__((__packed__));


/** \<\<public\>\> RPC process message constructor for receiving. */
extern struct tcmi_msg* tcmi_rpc_procmsg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> RPC process message constructor for sending. */
extern struct tcmi_msg* tcmi_rpc_procmsg_new_tx(struct tcmi_slotvec *transactions, 
						 pid_t dst_pid);

/** \<\<public\>\> Return RPC parameter. */
static inline void* tcmi_rpc_procmsg_data_base(struct tcmi_rpc_procmsg *self, unsigned int index){
	if(index < self->nmemb)
		return self->elem_base[index];
	mdbg(ERR3, "Trying to acces beyond array boundary");
	return NULL;
}

/** \<\<public\>\> Return RPC parameter size. */
static inline unsigned long tcmi_rpc_procmsg_data_size(struct tcmi_rpc_procmsg *self, unsigned int index){
	if(index < self->nmemb)
		return self->elem_size[index];
	mdbg(ERR3, "Trying to acces beyond array boundary");
	return 0;
}

/** \<\<public\>\> Return RPC number. */
static inline int tcmi_rpc_procmsg_num (struct tcmi_rpc_procmsg *self){
	return self->rpcnum;
}

/** Message descriptor for the factory class, there is no error
 * handling as this is the starting request */
#define TCMI_RPC_PROCMSG_DSC TCMI_MSG_DSC(TCMI_RPC_PROCMSG_ID, tcmi_rpc_procmsg_new_rx, NULL)
/** RPC message transaction timeout set to 10 minutes*/
#define TCMI_RPC_PROCMSGTIMEOUT 10*60*HZ

/** Casts to the tcmi_rpc_procmsg instance. */
#define TCMI_RPC_PROCMSG(m) ((struct tcmi_rpc_procmsg*)m)


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_RPC_PROCMSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_rpc_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_rpc_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Frees RPC message resources */
static void tcmi_rpc_procmsg_free(struct tcmi_procmsg *self);

/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops rpc_procmsg_ops;

#endif /* TCMI_RPC_PROCMSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_RPC_PROCMSG_H */

