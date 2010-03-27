/**
 * @file tcmi_skelresp_procmsg.h - TCMI test response process message - can be used as skeleton
 *                                 for any response process message.
 *       
 *                      
 * 
 *
 *
 * Date: 04/19/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_skelresp_procmsg.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#ifndef _TCMI_SKELRESP_PROCMSG_H
#define _TCMI_SKELRESP_PROCMSG_H

#include "tcmi_procmsg.h"
#include "tcmi_err_procmsg.h"

/** @defgroup tcmi_skelresp_procmsg_class tcmi_skelresp_procmsg class
 *
 * @ingroup tcmi_procmsg_class
 *
 * This class represents a test process message and is intended to be used
 * as a skeleton response when implementing new response process messages in the TCMI protocol.
 * To implement a new message the same steps need to be
 * done as in case of a request message. In addition the instance should be
 * aware of what messages it is a reply to.
 * @{
 */

/** Compound structure, inherits from tcmi_procmsg_class */
struct tcmi_skelresp_procmsg {
	/** parent class instance */
	struct tcmi_procmsg super;
};




/** \<\<public\>\> Skeleton response process message rx constructor. */
extern struct tcmi_msg* tcmi_skelresp_procmsg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> Skeleton response process message tx constructor. */
extern struct tcmi_msg* tcmi_skelresp_procmsg_new_tx(u_int32_t trans_id, pid_t dst_pid);




/** Message descriptor for the factory class, for error handling we used tcmi_errmsg_class */
#define TCMI_SKELRESP_PROCMSG_DSC \
TCMI_MSG_DSC(TCMI_SKELRESP_PROCMSG_ID, tcmi_skelresp_procmsg_new_rx, tcmi_err_procmsg_new_rx)

/** Casts to the tcmi_skelresp_procmsg instance. */
#define TCMI_SKELRESP_PROCMSG(m) ((struct tcmi_skelresp_procmsg*)m)

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SKELRESP_PROCMSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_skelresp_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_skelresp_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops skelresp_procmsg_ops;

#endif /* TCMI_SKELRESP_PROCMSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_SKELRESP_PROCMSG_H */

