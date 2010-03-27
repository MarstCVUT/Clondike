/**
 * @file tcmi_guest_started_procmsg.h - response from the guest task when it is succesfully started
 *       
 *                      
 * 
 *
 *
 * Date: 04/27/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_guest_started_procmsg.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#ifndef _TCMI_GUEST_STARTED_PROCMSG_H
#define _TCMI_GUEST_STARTED_PROCMSG_H

#include "tcmi_procmsg.h"
#include "tcmi_err_procmsg.h"

/** @defgroup tcmi_guest_started_procmsg_class tcmi_guest_started_procmsg class
 *
 * @ingroup tcmi_procmsg_class
 *
 * This class represents a message that is sent to the migration initiator as a
 * response to a migration request.
 * The main purpose is to communicate the PID of the guest process to the migration
 * initiator. 
 *
 * @{
 */

/** Compound structure, inherits from tcmi_procmsg_class */
struct tcmi_guest_started_procmsg {
	/** parent class instance */
	struct tcmi_procmsg super;
	/** guest PID,  used for further communication. */
	pid_t guest_pid;
};




/** \<\<public\>\> Guest started message rx constructor. */
extern struct tcmi_msg* tcmi_guest_started_procmsg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> Guest started message tx constructor. */
extern struct tcmi_msg* tcmi_guest_started_procmsg_new_tx(u_int32_t trans_id, pid_t dst_pid, 
							 pid_t guest_pid);




/** Message descriptor for the factory class, for error handling we used tcmi_errmsg_class */
#define TCMI_GUEST_STARTED_PROCMSG_DSC \
TCMI_MSG_DSC(TCMI_GUEST_STARTED_PROCMSG_ID, tcmi_guest_started_procmsg_new_rx, tcmi_err_procmsg_new_rx)

/** Casts to the tcmi_guest_started_procmsg instance. */
#define TCMI_GUEST_STARTED_PROCMSG(m) ((struct tcmi_guest_started_procmsg*)m)

/**
 * \<\<public\>\> Stub PID accessor.
 * 
 * @param *self - this message instance
 * @return guest PID
 */
static inline pid_t tcmi_guest_started_procmsg_guest_pid(struct tcmi_guest_started_procmsg *self)
{
	return self->guest_pid;
}


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_GUEST_STARTED_PROCMSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_guest_started_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_guest_started_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops guest_started_procmsg_ops;

#endif /* TCMI_GUEST_STARTED_PROCMSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_GUEST_STARTED_PROCMSG_H */

