/**
 * @file tcmi_skelresp_msg.h - TCMI test response message - can be used as skeleton
 *                             for response message.
 *       
 *                      
 * 
 *
 *
 * Date: 04/11/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_skelresp_msg.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#ifndef _TCMI_SKELRESP_MSG_H
#define _TCMI_SKELRESP_MSG_H

#include "tcmi_msg.h"
#include "tcmi_err_msg.h"

/** @defgroup tcmi_skelresp_msg_class tcmi_skelresp_msg class
 *
 * @ingroup tcmi_msg_class
 *
 * This class represents a test message and is intended to be used
 * as a skeleton when implementing new messages in the TCMI protocol.
 * To implement a new message the same steps need to be
 * done as in case of a request message. In addition the instance should be
 * aware of what messages it is a reply to.
 *
 * @{
 */

/** Compound structure, inherits from tcmi_msg_class */
struct tcmi_skelresp_msg {
	/** parent class instance */
	struct tcmi_msg super;
};




/** \<\<public\>\> Skeleton response message rx constructor. */
extern struct tcmi_msg* tcmi_skelresp_msg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> Test response message tx constructor. */
extern struct tcmi_msg* tcmi_skelresp_msg_new_tx(u_int32_t trans_id);




/** Message descriptor for the factory class, for error handling we used tcmi_errmsg_class */
#define TCMI_SKELRESP_MSG_DSC \
TCMI_MSG_DSC(TCMI_SKELRESP_MSG_ID, tcmi_skelresp_msg_new_rx, tcmi_err_msg_new_rx)

/** Casts to the tcmi_skelresp_msg instance. */
#define TCMI_SKELRESP_MSG(m) ((struct tcmi_skelresp_msg*)m)

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SKELRESP_MSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_skelresp_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_skelresp_msg_send(struct tcmi_msg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_msg_ops skelresp_msg_ops;

#endif /* TCMI_SKELRESP_MSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_SKELRESP_MSG_H */

