/**
 * @file tcmi_rpcresp_procmsg.h - TCMI RPC response process message 
 * 
 *
 *
 * Date: 20/04/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_rpcresp_procmsg.h,v 1.5 2007/10/11 21:00:26 stavam2 Exp $
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
#ifndef _TCMI_RPCRESP_PROCMSG_H
#define _TCMI_RPCRESP_PROCMSG_H

#include "tcmi_procmsg.h"
#include "tcmi_err_procmsg.h"
#include <tcmi/syscall/tcmi_rpc.h>

/** @defgroup tcmi_rpcresp_procmsg_class tcmi_rpcresp_procmsg class
 *
 * @ingroup tcmi_procmsg_class
 *
 * @TODO: Updadate comment
 * @{
 */

/** Compound structure, inherits from tcmi_procmsg_class */
struct tcmi_rpcresp_procmsg 
{
	/** parent class instance */
	struct tcmi_procmsg super;
	/** RPC number */
	u_int16_t rpcnum;
	/** Number of memory elements */
	u_int16_t nmemb;
	/** Return code of remote procedure */
	int64_t rtn_code;
	/** Array describing how to free elements on msg_put */
	u_int8_t *elem_free;
	/** Array with size of elements */
	u_int32_t *elem_size;
	/** Array of pointers to elements */ 
	void **elem_base;
} __attribute__((__packed__));

/** \<\<public\>\> RPC response process message rx constructor. */
extern struct tcmi_msg* tcmi_rpcresp_procmsg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> RPC response process message tx constructor. */
extern struct tcmi_msg* tcmi_rpcresp_procmsg_new_tx(u_int32_t trans_id, pid_t dst_pid);

/** \<\<public\>\> Get pointer do data
 * @param *self  - this message instance
 * @param index  - index of data which we want
 *
 * @return pointer to data or NULL if index is out of range
 */
static inline void* tcmi_rpcresp_procmsg_data_base(struct tcmi_rpcresp_procmsg *self, unsigned int index)
{
	if(index < self->nmemb)
		return self->elem_base[index];
	mdbg(ERR3, "Trying to acces beyond array boundary");
	return NULL;
}

/** \<\<public\>\> Get RPC parameter size. 
 * @param *self  - this message instance
 * @param index  - index of data which size we want
 *
 * @return RPC parameter size or 0 if index is out of range
 */
static inline unsigned long tcmi_rpcresp_procmsg_data_size(struct tcmi_rpcresp_procmsg *self, unsigned int index)
{
	if(index < self->nmemb)
		return self->elem_size[index];
	mdbg(ERR3, "Trying to acces beyond array boundary");
	return 0;
}

/** \<\<public\>\> Set if and how data will be freed on last put
 * @param *self  - this message instance
 * @param index  - index of data which put action we are going to set
 * @param action - one of  #TCMI_RPCRESP_PROCMSG_DONTFREE, #TCMI_RPCRESP_PROCMSG_KFREE or #TCMI_RPCRESP_PROCMSG_VFREE 
 */
static inline void tcmi_rpcresp_procmsg_free_on_put(struct tcmi_rpcresp_procmsg *self, unsigned int index, int action)
{
	if(index < self->nmemb)
		self->elem_free[index] = action;
	else
		mdbg(ERR3, "Trying to acces beyond array boundary");
}

/** \<\<public\>\> Don't free data on put */
#define TCMI_RPCRESP_PROCMSG_DONTFREE 1
/** \<\<public\>\> Free data using kfree on put */
#define TCMI_RPCRESP_PROCMSG_KFREE 2
/** \<\<public\>\> Free data usinf vfree on put */
#define TCMI_RPCRESP_PROCMSG_VFREE 3

/** \<\<public\>\> Creates and sends rpc_proc_msg and returns response */
struct tcmi_msg* tcmi_rpcresp_procmsg_get_response(unsigned int rpcnum, ...);

/** \<\<public\>\> Create response to rpc message */
struct tcmi_msg* tcmi_rpcresp_procmsg_create(struct tcmi_msg *m, long rtn_code, ...);

/** \<\<public\>\> Return RPC return code. */
static inline long tcmi_rpcresp_procmsg_rtn(struct tcmi_rpcresp_procmsg *self)
{
	return self->rtn_code;
}

/** \<\<public\>\> Return RPC number. */
static inline int tcmi_rpcresp_procmsg_num(struct tcmi_rpcresp_procmsg *self)
{
	return self->rpcnum;
}

/** Message descriptor for the factory class, for error handling we used tcmi_errmsg_class */
#define TCMI_RPCRESP_PROCMSG_DSC \
TCMI_MSG_DSC(TCMI_RPCRESP_PROCMSG_ID, tcmi_rpcresp_procmsg_new_rx, tcmi_err_procmsg_new_rx)

/** Casts to the tcmi_rpcresp_procmsg instance. */
#define TCMI_RPCRESP_PROCMSG(m) ((struct tcmi_rpcresp_procmsg*)m)

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_RPCRESP_PROCMSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_rpcresp_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_rpcresp_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Frees RPC message resources */
static void tcmi_rpcresp_procmsg_free(struct tcmi_procmsg *self);

/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops rpcresp_procmsg_ops;

#endif /* TCMI_RPCRESP_PROCMSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_RPCRESP_PROCMSG_H */

