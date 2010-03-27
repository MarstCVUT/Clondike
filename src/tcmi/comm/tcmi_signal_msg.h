/**
 * @file tcmi_signal_msg.h - TCMI signal communication message 
 *                      
 * 
 *
 *
 * Date: 04/11/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_signal_msg.h,v 1.2 2007/09/02 10:54:25 stavam2 Exp $
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
#ifndef _TCMI_SIGNAL_MSG_H
#define _TCMI_SIGNAL_MSG_H

#include "tcmi_msg.h"

/** @defgroup tcmi_signal_msg_class tcmi_signal_msg class
 *
 * @ingroup tcmi_msg_class
 *
 * This class represents a test message and is intended to be used
 * as a skeleton when implementing new messages in the TCMI protocol.
 * To implement a new message following steps need to be done:
 * - Implement the constructors
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
 * @{
 */

/** Compound structure, inherits from tcmi_msg_class */
struct tcmi_signal_msg {
	/** parent class instance */
	struct tcmi_msg super;
	/** Target PID */
	pid_t pid;
	/** Siginfo structure (we can use directly this structure as it is padded to be exactly 128 bytes) */
	siginfo_t info;
} __attribute__((__packed__));


/** \<\<public\>\> Skeleton message constructor for receiving. */
extern struct tcmi_msg* tcmi_signal_msg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> Skeleton message constructor for transferring. */
extern struct tcmi_msg* tcmi_signal_msg_new_tx(struct tcmi_slotvec *transactions);

/** \<\<public\>\> Construktor for sending with initialization */
struct tcmi_msg *tcmi_signal_msg_new(pid_t pid, siginfo_t *info);

/** Message descriptor for the factory class, there is no error
 * handling as this is the starting request */
#define TCMI_SIGNAL_MSG_DSC TCMI_MSG_DSC(TCMI_SIGNAL_MSG_ID, tcmi_signal_msg_new_rx, NULL)
/** Skeleton message transaction timeout set to 5 seconds*/
#define TCMI_SIGNAL_MSGTIMEOUT 5*HZ

/** Casts to the tcmi_signal_msg instance. */
#define TCMI_SIGNAL_MSG(m) ((struct tcmi_signal_msg*)m)


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SIGNAL_MSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_signal_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_signal_msg_send(struct tcmi_msg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_msg_ops skel_msg_ops;

#endif /* TCMI_SIGNAL_MSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_SIGNAL_MSG_H */

