/**
 * @file tcmi_p_emigrate_msg.h - PPM/NPM physical emigration message.
 *       
 *                      
 * 
 *
 *
 * Date: 04/27/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_p_emigrate_msg.h,v 1.2 2009-04-06 21:48:46 stavam2 Exp $
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
#ifndef _TCMI_P_EMIGRATE_MSG_H
#define _TCMI_P_EMIGRATE_MSG_H

#include "tcmi_msg.h"

/** @defgroup tcmi_p_emigrate_msg_class tcmi_p_emigrate_msg class
 *
 * @ingroup tcmi_msg_class
 *
 * This class represents a message for preemptive/non-preemtive process migration
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
 * @{
 */

/** Compound structure, inherits from tcmi_msg_class */
struct tcmi_p_emigrate_msg {
	/** parent class instance. */
	struct tcmi_msg super;
	/** groups pid and size, so they can be sent/received at once. */
	struct {
		/** remote PID,  used for further communication. */
		pid_t reply_pid;
		/** size of the checkpoint name in bytes (including
		    trailing zero) */
		int32_t size;
		/** size of the exec name in bytes (including
		    trailing zero) */
		int32_t exec_name_size;
		/** effective user id of a process as seen by the node where it is running */
		int16_t euid;
		/** effective group id of a process as seen by the node where it is running */
		int16_t egid;
		/** fsuid of the process on the core node.. may be needed for DFS mount before the checkpoint is read */
		int16_t fsuid;
		/** fsgid of the process on the core node.. may be needed for DFS mount before the checkpoint is read */
		int16_t fsgid;
	} pid_and_size  __attribute__((__packed__));

	/** name of the executable of the process. */
	char *exec_name;
	/** name of the checkpoint file. */
	char *ckpt_name;
};




/** \<\<public\>\> Phys. emigrate message constructor for receiving. */
extern struct tcmi_msg* tcmi_p_emigrate_msg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> Phys. emigrate message constructor for transferring. */
extern struct tcmi_msg* tcmi_p_emigrate_msg_new_tx(struct tcmi_slotvec *transactions, 
						       pid_t reply_pid, char *exec_name, char *ckpt_name, int16_t euid, int16_t egid, int16_t fsuid, int16_t fsgid);


/** \<\<public\>\> Message descriptor for the factory class, there is no error
 * handling as this is the starting request */
#define TCMI_P_EMIGRATE_MSG_DSC TCMI_MSG_DSC(TCMI_P_EMIGRATE_MSG_ID, tcmi_p_emigrate_msg_new_rx, NULL)
/** Response time out is set to 10 seconds*/
#define TCMI_P_EMIGRATE_MSGTIMEOUT (10*HZ)

/** Casts to the tcmi_p_emigrate_msg instance. */
#define TCMI_P_EMIGRATE_MSG(m) ((struct tcmi_p_emigrate_msg*)m)

/**
 * \<\<public\>\> Remote PID accessor.
 * 
 * @param *self - this message instance
 * @return remote PID
 */
static inline pid_t tcmi_p_emigrate_msg_reply_pid(struct tcmi_p_emigrate_msg *self)
{
	return self->pid_and_size.reply_pid;
}

/**
 * \<\<public\>\> Executable name accessor.
 * 
 * @param *self - this message instance
 * @return executable name string
 */
static inline char* tcmi_p_emigrate_msg_exec_name(struct tcmi_p_emigrate_msg *self)
{
	return self->exec_name;
}

/**
 * \<\<public\>\> Checkpoint name accessor.
 * 
 * @param *self - this message instance
 * @return checkpoint name string
 */
static inline char* tcmi_p_emigrate_msg_ckpt_name(struct tcmi_p_emigrate_msg *self)
{
	return self->ckpt_name;
}

/**
 * \<\<public\>\> Fsuid accesses
 * 
 * @param *self - this message instance
 * @return fsuid
 */
static inline int16_t tcmi_p_emigrate_msg_fsuid(struct tcmi_p_emigrate_msg *self)
{
	return self->pid_and_size.fsuid;
}

/**
 * \<\<public\>\> Fsgid accesses
 * 
 * @param *self - this message instance
 * @return fsgid
 */
static inline int16_t tcmi_p_emigrate_msg_fsgid(struct tcmi_p_emigrate_msg *self)
{
	return self->pid_and_size.fsgid;
}

/**
 * \<\<public\>\> Fsuid accesses
 * 
 * @param *self - this message instance
 * @return euid
 */
static inline int16_t tcmi_p_emigrate_msg_euid(struct tcmi_p_emigrate_msg *self)
{
	return self->pid_and_size.euid;
}

/**
 * \<\<public\>\> Fsgid accesses
 * 
 * @param *self - this message instance
 * @return egid
 */
static inline int16_t tcmi_p_emigrate_msg_egid(struct tcmi_p_emigrate_msg *self)
{
	return self->pid_and_size.egid;
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_P_EMIGRATE_MSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_p_emigrate_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_p_emigrate_msg_send(struct tcmi_msg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_msg_ops p_emigrate_msg_ops;

#endif /* TCMI_P_EMIGRATE_MSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_P_EMIGRATE_MSG_H */

