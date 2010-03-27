/**
 * @file tcmi_skel_procmsg.h - TCMI test process communication message - can be used as skeleton
 *                             for other process control messages.
 *       
 *                      
 * 
 *
 *
 * Date: 04/19/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_skel_procmsg.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#ifndef _TCMI_SKEL_PROCMSG_H
#define _TCMI_SKEL_PROCMSG_H

#include "tcmi_procmsg.h"

/** @defgroup tcmi_skel_procmsg_class tcmi_skel_procmsg class
 *
 * @ingroup tcmi_procmsg_class
 *
 * This class represents a test process control message and is intended to be used
 * as a skeleton when implementing new process control messages in the TCMI protocol.
 * To implement a new message following steps need to be done:
 * - Implement the constructors:
 *   - new_tx - functionality to allocate a message for transfer based on 
 *   custom parameters specific to each message type. Use init_tx for initialization
 *   of the super class instance data
 *   - new_rx - same, but for reception, therefore, no parameters are specified. If needed
 *   compare the supplied message ID, with the official message ID assigned to this type
 *   of message.
 *   - new_rx_err - A custom build of error message can be specified or some generic
 *   new_rx_err message constructor can be used.
 * - Choose a message identifier and append it to a proper group in tcmi_messages_id.h
 * - Declare a new message descriptor, append it into tcmi_messages_dsc.h 
 * - Implement custom operations if needed.
 *
 * @{
 */

/** Compound structure, inherits from tcmi_procmsg_class */
struct tcmi_skel_procmsg {
	/** parent class instance */
	struct tcmi_procmsg super;
};




/** \<\<public\>\> Skeleton process message constructor for receiving. */
extern struct tcmi_msg* tcmi_skel_procmsg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> Skeleton process message constructor for transferring. */
extern struct tcmi_msg* tcmi_skel_procmsg_new_tx(struct tcmi_slotvec *transactions, 
						 pid_t dst_pid);


/** Message descriptor for the factory class, there is no error
 * handling as this is the starting request */
#define TCMI_SKEL_PROCMSG_DSC TCMI_MSG_DSC(TCMI_SKEL_PROCMSG_ID, tcmi_skel_procmsg_new_rx, NULL)
/** Test message transaction timeout set to 5 seconds*/
#define TCMI_SKEL_PROCMSGTIMEOUT 5*HZ

/** Casts to the tcmi_skel_procmsg instance. */
#define TCMI_SKEL_PROCMSG(m) ((struct tcmi_skel_procmsg*)m)


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SKEL_PROCMSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_skel_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_skel_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops skel_procmsg_ops;

#endif /* TCMI_SKEL_PROCMSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_SKEL_PROCMSG_H */

