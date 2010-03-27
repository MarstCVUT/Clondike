/**
 * @file tcmi_queue.h - Implements an SMP safe queue
 *                      
 * 
 *
 *
 * Date: 04/10/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_queue.h,v 1.3 2007/09/02 10:54:43 stavam2 Exp $
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
#ifndef _TCMI_QUEUE_H
#define _TCMI_QUEUE_H

#include <linux/list.h>
#include <asm/atomic.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>

#include <dbg.h>

/** @defgroup tcmi_queue_class tcmi_queue class 
 *
 * @ingroup tcmi_lib_group
 *
 * The queue is implemented using a doubly linked list - provided by kernel.
 * In addition, it contains a wait queue for process that want to be
 * notified upon addition of a new element.
 * Basic methods:
 * - add - appends an element to the tail of the queue
 * - empty - true if the queue is empty
 * - remove_head - returns the element from the head of the queue and removes it -
 *              this operation is atomic.
 * - wait_on_empty - the calling process is put a sleep, until the queue
 * contains at least 1 element. 
 *
 * @{
 */

/** Denotes whether the queue has been staticaly or dynamically
 * created. This is needed if somebody issues a put on a static
 * queue, so that no kfree is performed */
typedef enum {
	TCMI_QUEUE_STATIC,
	TCMI_QUEUE_DYNAMIC
} tcmi_queue_type_t;

/** compound structure */
struct tcmi_queue {
	/** spin lock - serializes access to the queue */
	spinlock_t q_lock;

	/** The actual queue */
	struct list_head queue;

	/** reference counter for dynamically created queues */
	atomic_t ref_count;

	/** describes how the queue has been created */
	tcmi_queue_type_t type;

	/** Sleeping processes, that are to be notified upon addition
	 * of a new element */
	wait_queue_head_t wq;
};




/**
 * \<\<public\>\> Queue initializer - for static allocation. The
 * reference counter is set to 1.
 *
 * @param *self - pointer to this queue instance
 *  
 */
static inline void tcmi_queue_init(struct tcmi_queue *self)
{
	atomic_set(&self->ref_count, 1);
	spin_lock_init(&self->q_lock);
	INIT_LIST_HEAD(&self->queue);
	self->type = TCMI_QUEUE_STATIC;
	init_waitqueue_head(&self->wq);
}


/**
 * \<\<public\>\> Queue constructor.
 *
 * @return a new queue or NULL
 */
static inline struct tcmi_queue* tcmi_queue_new(void)
{
	struct tcmi_queue *q;
	q = kmalloc(sizeof(struct tcmi_queue), GFP_ATOMIC);
	if (!q) {
		mdbg(ERR3, "Can't allocated memory for a queue");
		goto exit0;
	}
	tcmi_queue_init(q);
	/* mark the queue dynamic, so that it is released on the last 'put' */
	q->type = TCMI_QUEUE_DYNAMIC;

	mdbg(INFO4, "Allocated new queue %p", q);
	return q;
	/* error handling */
 exit0:
	return NULL;
}

/** 
 * \<\<public\>\> Queue instance accessor. Increments the reference
 * counter
 * 
 * @param *self - pointer to this queue instance
 * @return - self
 */
static inline struct tcmi_queue* tcmi_queue_get(struct tcmi_queue *self)
{
	if (!self)
		return NULL;
	atomic_inc(&self->ref_count);
	return self;
}

/**
 * \<\<public\>\> Decrements the reference counter, when the count
 * reaches 0, the instance is destroyed.
 *
 * @param *self - pointer to this queue instance
 */
static inline void tcmi_queue_put(struct tcmi_queue *self)
{
	if (!self)
		return;

	if (atomic_dec_and_test(&self->ref_count)) {
		if (self->type == TCMI_QUEUE_STATIC)
			mdbg(INFO4, "Released last reference to a static queue");
		else {
			mdbg(INFO4, "Released last reference to a dynamic queue %p", 
			     self);
			kfree(self);
		}
	}

}

/**
 * \<\<public\>\> Locks the instance data. Spin lock used - no
 * sleeping allowed!
 *
 * @param *self - pointer to this queue instance
 */
static inline void tcmi_queue_lock(struct tcmi_queue *self)
{
	spin_lock(&self->q_lock);
}

/**
 * \<\<public\>\> Unlocks the instance data
 *
 * @param *self - pointer to this queue instance
 */
static inline void tcmi_queue_unlock(struct tcmi_queue *self)
{
	spin_unlock(&self->q_lock);
}

/**
 * @return 1 when queue is empty
 */
static inline int tcmi_queue_empty(struct tcmi_queue *self)
{
	return list_empty(&self->queue);
}

/**
 * \<\<public\>\> Adds an element to the queue - lock free version,
 * assumes the lock is already held.  Wakes up a task from the wait
 * queue, so that it can pickup the new element.
 *
 * @param *self - pointer to this queue instance
 * @param *element - element to be added to the queue
 */
static inline void __tcmi_queue_add(struct tcmi_queue *self, struct list_head *element)
{
	list_add_tail(element, &self->queue);
	wake_up(&self->wq);
}

/**
 * \<\<public\>\> Locks the queue and appends a new element to the tail.
 * @param *self - pointer to this queue instance
 * @param *element - element to be added to the queue
 */
static inline void tcmi_queue_add(struct tcmi_queue *self, struct list_head *element)
{
	tcmi_queue_lock(self);
	__tcmi_queue_add(self, element);
	tcmi_queue_unlock(self);
}

/**
 * \<\<public\>\> Removes an element in the head of the queue, assumes
 * the lock is held already.
 *
 * @param *self - pointer to this queue instance
 * @return - element in the head of the queue
 */
static inline struct list_head* __tcmi_queue_remove(struct tcmi_queue *self)
{
	struct list_head *element;
	if (tcmi_queue_empty(self))
		return NULL;
	else {
		element = self->queue.next;
		list_del_init(element);
		return (element);
	}
}

/**
 * \<\<public\>\> Locks the queue and removes removes an element in
 * the head of the queue, assumes the lock is held already.
 *
 * @param *self - pointer to this queue instance
 * @return - element in the head of the queue
 */
static inline struct list_head* tcmi_queue_remove(struct tcmi_queue *self)
{
	struct list_head *element;
	tcmi_queue_lock(self);
	element = __tcmi_queue_remove(self);
	tcmi_queue_unlock(self);
	return element;
}

/** 
 * \<\<public\>\> Removes an entry from the head of the queue.
 *
 * @param self - pointer to this queue instance
 * @param object - pointer that will hold the head element
 * @param member - name of the member within the object used for queueing
 */
#define tcmi_queue_remove_entry(self, object, member)			\
do {									\
	struct list_head *__q_entry;					\
	object = NULL;							\
	if ((__q_entry = tcmi_queue_remove(self)))			\
	    object = list_entry(__q_entry, typeof(*object), member);	\
} while (0)

/** 
 * \<\<public\>\> Removes an entry from the head of the queue. Assumes
 * the queue has already been locked.
 *
 * @param self - pointer to this queue instance
 * @param object - pointer that will hold the head element
 * @param member - name of the member within the object used for queueing
 */
#define __tcmi_queue_remove_entry(self, object, member)			\
do {									\
	struct list_head *__q_entry;					\
	object = NULL;							\
	if ((__q_entry = __tcmi_queue_remove(self)))			\
	    object = list_entry(__q_entry, typeof(*object), member);	\
} while (0)

/**
 * \<\<public\>\> Puts the calling process to sleep as long as the
 * queue is empty.
 *
 * @param *self - pointer to this queue instance
 * @return -ERESTARTSYS on signal arrival
 */
static inline int tcmi_queue_wait_on_empty_interruptible(struct tcmi_queue *self)
{
	/* Sleep until the queue is non empty */
	return wait_event_interruptible_exclusive(self->wq, (!tcmi_queue_empty(self)));
}

/**
 * \<\<public\>\> Puts the calling process to sleep as long as the
 * queue is empty or the timeout expires.
 *
 * @param *self - pointer to this queue instance
 * @param timeout - specified timeout
 * @return -ERESTARTSYS on signal arrival or timeout (>0) if the timer went 
 * off
 */
static inline int tcmi_queue_wait_on_empty_interruptible_timout(struct tcmi_queue *self, 
								int timeout)
{
	/* Sleep until the queue is non empty */
	return wait_event_interruptible_timeout(self->wq, (!tcmi_queue_empty(self)), 
						timeout);
}

/**
 * @}
 */
#endif /* _TCMI_QUEUE_H */
