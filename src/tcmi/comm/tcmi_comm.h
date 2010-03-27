/**
 * @file tcmi_comm.h - TCMI communication communication component.
 *       
 *                      
 * 
 *
 *
 * Date: 04/16/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_comm.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#ifndef _TCMI_COMM_H
#define _TCMI_COMM_H

#include <asm/atomic.h>

#include "tcmi_msg.h"
#include <kkc/kkc_sock.h>

/** @defgroup tcmi_comm_class tcmi_comm class
 *
 * @ingroup tcmi_comm_group
 *
 * This class provides functionality to receive and deliver messages
 * via a specified connection. The base of each instance is a separate
 * thread receiving messages via specified connection. The user also
 * specifies a method along with a parameter to be called upon
 * reception of each message. This method is responsible for message
 * delivery.
 *
 * @{
 */

/** Message delivery callback method type */
typedef int deliver_method_t (void*, struct tcmi_msg*);

/** Compound structure that holds reference to the connection and the
 * user method. */
struct tcmi_comm {
	/** reference counter */
	atomic_t ref_count;

	/** communication socket used for receiving */
	struct kkc_sock *sock;

	/** message delivery method */
	deliver_method_t *deliver;

	/** object whose method is to be called for message
	 * delivery. */
	void *obj;

	/** preferred group of messages that will be received. */
	int msg_group;

	/** message receiving thread */
	struct task_struct *thread;
};

/** Casts to the tcmi_comm instance. */
#define TCMI_COMM(c) ((struct tcmi_comm*)c)

/** \<\<public\>\> TCMI communication component constructor. */
extern struct tcmi_comm* tcmi_comm_new(struct kkc_sock *sock, deliver_method_t *method,
				       void *obj, int msg_group);



/** \<\<public\>\> Decrements the reference counter. */
extern void tcmi_comm_put(struct tcmi_comm *self);

/** 
 * \<\<public\>\> Instance accessor, increments the reference count.
 *
 * @param *self - pointer to this comm instance
 */
static inline struct tcmi_comm* tcmi_comm_get(struct tcmi_comm *self)
{
	if (self) {
		atomic_inc(&self->ref_count);
	}
	return self;
}

/** This macro implements atomic check for thread stop request and
 * puts the task to sleep until such request arrives. Note the order
 * of operations, the change of the state must always be prior to
 * checking if it should stop. Any other order is incorrect and might
 * put the task to sleep indefinitely. Also, it is necessary to set
 * the state to uninterruptible, so that the thread won't get woken up
 * rightaway by a pending signal.
 */
#define wait_on_should_stop()				\
do {							\
	do {						\
		schedule();				\
		set_current_state(TASK_UNINTERRUPTIBLE);\
	} while(!kthread_should_stop());		\
	set_current_state(TASK_RUNNING);		\
} while(0)

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_COMM_PRIVATE
/** Receiving thread method. */
static int tcmi_comm_thread(void *comm);
/** Thread counter - used for receiving thread naming. */
static int tcmi_comm_counter;
#endif /* TCMI_COMM_PRIVATE */

/**
 * @}
 */

#endif /* _TCMI_COMM_H */
