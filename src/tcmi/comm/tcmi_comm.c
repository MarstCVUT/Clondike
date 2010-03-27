/**
 * @file tcmi_comm.c - TCMI communication communication component.
 *       
 *                      
 * 
 *
 *
 * Date: 04/16/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_comm.c,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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

#include <linux/kthread.h>

#include "tcmi_msg_factory.h"

#define TCMI_COMM_PRIVATE
#include "tcmi_comm.h"

#include <dbg.h>



/** 
 * \<\<public\>\> TCMI communication component constructor.
 * - allocates the instance
 * - starts a new message receiving thread thread
 *
 * @param *sock - pointer to the socket that is to be used for the
 * communication
 * @param *deliver - method to be called upon message delivery
 * @param *obj - object whose method is to be called
 * @param msg_group - preferred group of messages that
 * are to be received - see tcmi_messages
 * @return new communication component or NULL
 */
struct tcmi_comm* tcmi_comm_new(struct kkc_sock *sock, deliver_method_t *deliver, 
				void *obj, int msg_group)
{
	struct tcmi_comm *comm;

	if (!sock || !deliver) {
		mdbg(ERR3, "Missing socket or delivery method");
		goto exit0;
	}
	if (!(comm = TCMI_COMM(kmalloc(sizeof(struct tcmi_comm), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate memory for communication component");
		goto exit0;
	}

	atomic_set(&comm->ref_count, 1);
	comm->sock = kkc_sock_get(sock);
	comm->deliver = deliver;
	comm->obj = obj;
	comm->msg_group = msg_group;


	comm->thread = kthread_run(tcmi_comm_thread, comm, 
				   "tcmi_commd_%02d", tcmi_comm_counter++);
	if (IS_ERR(comm->thread)) {
		minfo(ERR2, "Failed to create a message receiving thread!");
		goto exit1;
	}
	
	mdbg(INFO4, "Created TCMI comm, preferred message group %d, memory=%p", 
	     comm->msg_group, comm);
	return comm;

	/* error handling */
 exit1:
	kkc_sock_put(sock);
	kfree(comm);
 exit0:
	return NULL;
}



/** 
 * \<\<public\>\> Releases the instance. Decrements reference counter
 * and if it reaches 0, it will:
 * - stop the receiving thread by sending it a forced signal. The
 * signal interrupts any blocking receive operation. Then we
 * wait till the thread terminates.
 * - release the socket instance
 * - free the tcmi_comm instance
 *
 * @param *self - pointer to this instance
 */
void tcmi_comm_put(struct tcmi_comm *self)
{
	if (self && atomic_dec_and_test(&self->ref_count)) {
		mdbg(INFO4, "Destroying TCMI comm, preferred message group %d, memory=%p", 
		     self->msg_group, self);
		/* stop the transaction thread */
		force_sig(SIGTERM, self->thread);
		/* ensures synchronous thread termination */
		kthread_stop(self->thread);
		kkc_sock_put(self->sock);
		kfree(self);
	}
}

/** @addtogroup tcmi_comm_class
 *
 * @{
 */


/** 
 * \<\<private\>\> Messages receiving thread.
 * - receives any messages on the connection that is part of the tcmi_comm
 * instance
 * - as long as no invalid ID or other error occurs
 * or the main thread signals to quit it will be delivering messages to the
 * subscribed object via its delivery method.
 * - messages that are not successfully delivered are discarded
 *
 * The thread is always stopped synchronously with the instance
 * destruction, therefore there is no need to account an extra
 * reference for the thread.
 *
 * @param *comm - pointer to this communication instance
 * @return 0 upon successful termination.
 */
static int tcmi_comm_thread(void *comm)
{
	struct tcmi_comm *self = TCMI_COMM(comm);
	/* make local copies instance attributes, this saves one
	 * dereference for each */
	struct kkc_sock *sock = self->sock;
	deliver_method_t *deliver = self->deliver;
	int msg_group = self->msg_group;
	void *obj = self->obj;
	/* pointer to the message received */
	struct tcmi_msg *m; 
	u_int32_t msg_id;
	int err = 0;

	minfo(INFO3, "Receiving on socket local: '%s', remote: '%s'",
	      kkc_sock_getsockname2(sock), kkc_sock_getpeername2(sock));
	/* receive messages, checking for signals or stop request */
	while (!(kthread_should_stop() || signal_pending(current))) {
		if ((err = kkc_sock_recv(sock, &msg_id, sizeof(msg_id), KKC_SOCK_BLOCK)) < 0) {
			minfo(ERR3, "Error receiving message ID, error code: %d, terminating", err);
			break;
		}
		if (!(m = tcmi_msg_factory(msg_id, msg_group))) {
			minfo(ERR3, "Error building message %x, terminating", msg_id);
			break;
		}
		if ((err = tcmi_msg_recv(m, sock)) < 0) {
			minfo(ERR3, "Error receiving message %x, error %d, terminating", msg_id, err);
			break;
		}
		/* try to deliver, if it fails destroy the message */
		if (deliver(obj, m) < 0)
			tcmi_msg_put(m);
	}
	/* very important to sync with the the thread that is terminating us */
	wait_on_should_stop();
	mdbg(INFO3, "Message receiving thread terminating");
	return err;
}

/** Thread counter - used for receiving thread naming. */
static int tcmi_comm_counter = 0;

/**
 * @}
 */

