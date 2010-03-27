/**
 * @file tcmi_procmsg.h - TCMI process control communication message
 *       
 *                      
 * 
 *
 *
 * Date: 04/19/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_procmsg.h,v 1.3 2007/09/02 10:54:25 stavam2 Exp $
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
#ifndef _TCMI_PROCMSG_H
#define _TCMI_PROCMSG_H

#include <linux/types.h> 

#include "tcmi_msg.h"

/** @defgroup tcmi_procmsg_class tcmi_procmsg class
 *
 * @ingroup tcmi_msg_class
 *
 * This class represents a generic process message. Process message,
 * in addition to regular migration message, carries a target
 * process PID. This PID will be used by the receiving party for message
 * delivery. The only functionality provided by this class is
 * to send/receive the process ID and notify the specific procmsg
 * via send/receive/free operations.
 * 
 *
 * @{
 */

/** Compound structure, inherits from tcmi_msg_class */
struct tcmi_procmsg {
	/** parent class instance */
	struct tcmi_msg super;
	/** destination process PID */
	pid_t dst_pid;
	/** == 1, if the we should switch to migration mode after recieving this message */
	u_int8_t enforce_migmode;
	/** proc message operations */
	struct tcmi_procmsg_ops *msg_ops;
};


/** Process message operations that support polymorphism. */
struct tcmi_procmsg_ops {
	/** Receives the message via a specified connection. */
	int (*recv)(struct tcmi_procmsg*, struct kkc_sock*);
	/** Sends the message via a specified connection. */
	int (*send)(struct tcmi_procmsg*, struct kkc_sock*);
	/** Frees custom message resources. The destruction of the
	 * actual message instance is handled internally by this
	 * class */
	void (*free)(struct tcmi_procmsg*);
};


/** \<\<public\>\> Initializes the process message for receiving. */
extern int tcmi_procmsg_init_rx(struct tcmi_procmsg *self, u_int32_t msg_id,
				struct tcmi_procmsg_ops *procmsg_ops);

/** \<\<public\>\> Initializes the process message for transferring. */
extern int tcmi_procmsg_init_tx(struct tcmi_procmsg *self, u_int32_t msg_id, 
				struct tcmi_procmsg_ops *procmsg_ops,
				pid_t dst_pid, u_int8_t enforce_migmode,
				struct tcmi_slotvec *transactions, u_int32_t resp_msg_id,
				u_int timeout,
				u_int32_t resp_trans_id);

/** 
 * \<\<public\>\> Destination PID accessor.
 *
 * @param *self - this proc message instance
 * @return destination PID
 */
static inline pid_t tcmi_procmsg_dst_pid(struct tcmi_procmsg *self)
{
	return self->dst_pid;
}

/** 
 * \<\<public\>\> Getter of flag idicating whether we should switch to migmode or not
 *
 * @param *self - this proc message instance
 * @return 1 if we should switch to migmode, 0 otherwise
 */
static inline pid_t tcmi_procmsg_enforce_migmode(struct tcmi_procmsg *self)
{
	return self->enforce_migmode;
}


/** Casts to the tcmi_procmsg instance. */
#define TCMI_PROCMSG(m) ((struct tcmi_procmsg*)m)


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_PROCMSG_PRIVATE

/** Receives the process message via a specified connection. */
static int tcmi_procmsg_recv(struct tcmi_msg *self, struct kkc_sock *sock);

/** Sends the process message via a specified connection. */
static int tcmi_procmsg_send(struct tcmi_msg *self, struct kkc_sock *sock);

/** Frees process message resources. */
static void tcmi_procmsg_free(struct tcmi_msg *self);


/** Message operations that support polymorphism. */
static struct tcmi_msg_ops procmsg_ops;

#endif  /* TCMI_PROCMSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_PROCMSG_H */

