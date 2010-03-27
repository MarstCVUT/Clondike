/** @file kkc_sock_sleeper.h - Artificial object, that simplifies
 *                             sleeping on sockets
 *
 * Date: 04/18/2005
 *
 * Author: Jan Capek
 *
 * $Id: kkc_sock_sleeper.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
 *
 * This file is part of Kernel-to-Kernel Communication Library(KKC)
 * Copyleft (C) 2005  Jan Capek
 * 
 * KKC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * KKC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _KKC_SOCK_SLEEPER_H
#define _KKC_SOCK_SLEEPER_H

#include <linux/list.h>

#include "kkc_sock.h"


/** @defgroup kkc_sock_sleeper kkc_sock_sleeper class
 *
 * @ingroup kkc_class
 *
 * The main purpose of this class is to make it easier to create
 * and manage sleeping on KKC sockets. Usually when a thread 
 * is watching multiple sockets, it needs to create a separate
 * wait_queue_t element for it. The problem is where to store
 * all these queue elements as all its internal linking items
 * are fully used by the kernel wait queue. This class provides
 * a simple solution - a wrapper object.
 *
 * The object is intentionally designed for single threaded access(no
 * reference counting) as this should be sufficient.
 * 
 * @{
 */
/** Compound structure allowing external linking socket sleepers. */
struct kkc_sock_sleeper {
	/** linking element for public use. */
	struct list_head list;
	/** wait queue element passed to the socket. */
	wait_queue_t wait;
	/** socket associated with the sleeper */
	struct kkc_sock *sock;
};

/** 
 * \<\<public\>\> Creates a new sleeper element associated with a
 * particular socket and task. The specified task is added to the
 * socket wait queue
 *
 * @param *sock - target socket where to add new waiting process
 * @param *tsk - task that wants to sleep on socket's wait queue
 * @return new sleeper or NULL
 */
static inline struct kkc_sock_sleeper* kkc_sock_sleeper_add(struct kkc_sock *sock, 
							    struct task_struct *tsk)
{
	struct kkc_sock_sleeper *sleeper;

	if (!(sleeper = (struct kkc_sock_sleeper*)
	      kmalloc(sizeof(struct kkc_sock_sleeper), GFP_KERNEL))) {
		mdbg(ERR3, "Failed to allocate KKC sock sleeper");
		goto exit0;
	}
	/* initialize internal data */
	INIT_LIST_HEAD(&sleeper->list);
	init_waitqueue_entry(&sleeper->wait, tsk);
	sleeper->sock = sock;

	/* append the task to socket's wait queue */
	kkc_sock_add_wait_queue(sock, &sleeper->wait);

	mdbg(INFO3, "Added sleeper for socket local: '%s', remote: '%s'", 
	     kkc_sock_getsockname2(sock), kkc_sock_getpeername2(sock));

	return sleeper;
	/* error handling */
 exit0:
	return NULL;
}

/** 
 * \<\<public\>\> Removes a sleeper from its socket, from the list and
 * disposes it.
 *
 * @param *self - this socket sleeper instance
 */
static inline void kkc_sock_sleeper_remove(struct kkc_sock_sleeper *self)
{
	list_del_init(&self->list);
	kkc_sock_remove_wait_queue(self->sock, &self->wait);
	mdbg(INFO3, "Removed sleeper from socket local: '%s', remote: '%s'", 
	     kkc_sock_getsockname2(self->sock), kkc_sock_getpeername2(self->sock));
	kfree(self);
}


/** 
 * \<\<public\>\> Checks if the specified socket matches the socket
 * stored in the sleeper. If so, the sleeper removes itself from the
 * wait queue.
 *
 * @param *self - this socket sleeper instance
 * @param *sock - socket that is to be removed from a sleeper
 * @return true if successfully removed
 */
static inline int kkc_sock_sleeper_remove_match(struct kkc_sock_sleeper *self,
						struct kkc_sock *sock)
{
	int ret = 0;
	if (self->sock == sock) {
		mdbg(INFO4, "Socket matches, removing..");
		kkc_sock_sleeper_remove(self);
		ret = 1;
	}
	return ret;
}
/**
 * @}
 */


#endif /* _KKC_SOCK_SLEEPER_H */
