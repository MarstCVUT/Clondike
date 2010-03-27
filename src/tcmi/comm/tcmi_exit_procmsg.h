/**
 * @file tcmi_exit_procmsg.h - TCMI exit message, send by guest task from PEN
 *       
 *                      
 * 
 *
 *
 * Date: 04/27/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_exit_procmsg.h,v 1.3 2007/09/02 10:54:25 stavam2 Exp $
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
#ifndef _TCMI_EXIT_PROCMSG_H
#define _TCMI_EXIT_PROCMSG_H

#include "tcmi_procmsg.h"

/** @defgroup tcmi_exit_procmsg_class tcmi_exit_procmsg class
 *
 * @ingroup tcmi_procmsg_class
 *
 * This class represents an exit message that will be sent out by
 * a guest process upon exit. The receiving shadow process has to take
 * appropriate actions.
 *
 * @{
 */

/** Compound structure, inherits from tcmi_procmsg_class */
struct tcmi_exit_procmsg {
	/** parent class instance. */
	struct tcmi_procmsg super;
	/** exit code the message is carrying. */
	int32_t code;
};




/** \<\<public\>\> Exit process message constructor for receiving. */
extern struct tcmi_msg* tcmi_exit_procmsg_new_rx(u_int32_t msg_id);

/** \<\<public\>\>  Exit process message constructor for transferring. */
extern struct tcmi_msg* tcmi_exit_procmsg_new_tx(pid_t dst_pid, int32_t code);


/** Message descriptor for the factory class, there is no error
 * handling as this is the starting request */
#define TCMI_EXIT_PROCMSG_DSC TCMI_MSG_DSC(TCMI_EXIT_PROCMSG_ID, tcmi_exit_procmsg_new_rx, NULL)

/** Casts to the tcmi_exit_procmsg instance. */
#define TCMI_EXIT_PROCMSG(m) ((struct tcmi_exit_procmsg*)m)

/**
 * Exit code accessor.
 * 
 * @param *self - this message instance
 * @return exit code
 */
static inline int32_t tcmi_exit_procmsg_code(struct tcmi_exit_procmsg *self) 
{
	return self->code;
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_EXIT_PROCMSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_exit_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_exit_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops exit_procmsg_ops;

#endif /* TCMI_EXIT_PROCMSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_EXIT_PROCMSG_H */

