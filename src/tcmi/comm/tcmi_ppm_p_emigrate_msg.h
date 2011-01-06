/**
 * @file tcmi_ppm_p_emigrate_msg.h - PPM_P emigration message.
 *       
 *                      
 * 
 *
 *
 * Date: 04/27/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ppm_p_emigrate_msg.h,v 1.3 2007/09/02 10:54:25 stavam2 Exp $
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
#ifndef _TCMI_PPM_P_EMIGRATE_MSG_H
#define _TCMI_PPM_P_EMIGRATE_MSG_H

#include "tcmi_msg.h"

/** @defgroup tcmi_ppm_p_emigrate_msg_class tcmi_ppm_p_emigrate_msg class
 *
 * @ingroup tcmi_msg_class
 *
 * This class represents a message for preemptive process migration
 * via a physical checkpoint image.  The migrating process sends this
 * message to the PEN, that is responsible for creation of a guest
 * process and finishing the migration. Should the migration fail, an
 * error is generated and sent back as a reply.  This is a migration
 * control message that will be received by the migration manager and
 * passed onto the newly created guest. The response to this message
 * is however a process control message (\link
 * tcmi_guest_started_procmsg_class TCMI_GUEST_STARTED_PROCMSG
 * \endlink) that will be directly routed to the sending task.
 *
 * The sending task specifies it's PID, so that the receiving guest can use
 * it for further communication via process control connection. Also,
 * the checkpoint file name is specified as part of the message.
 *
 *
 *
 * @{
 */

/** Compound structure, inherits from tcmi_msg_class */
struct tcmi_ppm_p_emigrate_msg {
	/** parent class instance. */
	struct tcmi_msg super;
	/** groups pid and size, so they can be sent/received at once. */
	struct {
		/** remote PID,  used for further communication. */
		pid_t reply_pid;
		/** size of the checkpoint name in bytes (including
		    trailing zero) */
		int32_t size;
	} pid_and_size  __attribute__((__packed__));
	/** name of the checkpoint file. */
	char *ckpt_name;
};




/** \<\<public\>\> PPM phys. emigrate message constructor for receiving. */
extern struct tcmi_msg* tcmi_ppm_p_emigrate_msg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> PPM phys. emigrate message constructor for transferring. */
extern struct tcmi_msg* tcmi_ppm_p_emigrate_msg_new_tx(struct tcmi_slotvec *transactions, 
						       pid_t reply_pid, char *ckpt_name);


/** \<\<public\>\> Message descriptor for the factory class, there is no error
 * handling as this is the starting request */
#define TCMI_PPM_P_EMIGRATE_MSG_DSC TCMI_MSG_DSC(TCMI_PPM_P_EMIGRATE_MSG_ID, tcmi_ppm_p_emigrate_msg_new_rx, NULL)
/** Response time out is set to 60 seconds*/
#define TCMI_PPM_P_EMIGRATE_MSGTIMEOUT 60*HZ

/** Casts to the tcmi_ppm_p_emigrate_msg instance. */
#define TCMI_PPM_P_EMIGRATE_MSG(m) ((struct tcmi_ppm_p_emigrate_msg*)m)

/**
 * \<\<public\>\> Remote PID accessor.
 * 
 * @param *self - this message instance
 * @return remote PID
 */
static inline pid_t tcmi_ppm_p_emigrate_msg_reply_pid(struct tcmi_ppm_p_emigrate_msg *self)
{
	return self->pid_and_size.reply_pid;
}

/**
 * \<\<public\>\> Checkpoint name accessor.
 * 
 * @param *self - this message instance
 * @return checkpoint name string
 */
static inline char* tcmi_ppm_p_emigrate_msg_ckpt_name(struct tcmi_ppm_p_emigrate_msg *self)
{
	return self->ckpt_name;
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_PPM_P_EMIGRATE_MSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_ppm_p_emigrate_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_ppm_p_emigrate_msg_send(struct tcmi_msg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_msg_ops ppm_p_emigrate_msg_ops;

#endif /* TCMI_PPM_P_EMIGRATE_MSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_PPM_P_EMIGRATE_MSG_H */

