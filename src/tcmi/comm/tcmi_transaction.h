/**
 * @file tcmi_transaction.h - TCMI communication transaction.
 *       
 *                      
 * 
 *
 *
 * Date: 04/09/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_transaction.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#ifndef _TCMI_TRANSACTION_H
#define _TCMI_TRANSACTION_H

#include <asm/atomic.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/wait.h>

#include <tcmi/lib/tcmi_slotvec.h>

/** @defgroup tcmi_transaction_class tcmi_transaction class
 *
 * @ingroup tcmi_comm_group
 *
 * A transaction in communication is setup when a request message
 * expects a certain response from the receiving party. This class
 * provides the functionality to create, start and complete such 
 * transaction.
 *
 * An example: A PEN node wants to register with a CCN. It will send
 * a registration request towards the CCN. A new transaction is
 * started on the PEN side by the thread creating the initial
 * message. The transaction is assigned following attributes:
 * - transaction ID
 * - response message ID
 * - response timeout
 * - flags -
 *   - anonymous - setting an anonymous flag means that the user doesn't
 *   wait for the response synchronously. This flag is checked by
 *   tcmi_msg.c::tcmi_msg_deliver() - see for details.
 *   - expired - the timer upon timout marks the transaction
 *   expired and sets it to aborted state. This flag is used
 *   when checking whether the timer went off or not (needed for
 *   transaction destruction).
 *
 * The CCN receives the registration request performs some
 * authentication tasks and sends a reply back. The response message
 * will carry the transaction ID. On PEN, the message is identified
 * based on the transaction ID and successfully received. The message
 * then completes the transaction, which in turn wakes up the thread
 * waiting for the message.  Message delivery is closely coupled to
 * transactions. See tcmi_msg.c::tcmi_msg_deliver().
 *
 * There are few stages in transaction lifetime:
 * - INIT - this is the phase when the transaction is instantiated
 * and inserted into a transaction slot vector, but not active yet
 * - RUNNING - transaction is activated and the timout timer
 * is running.
 * - COMPLETE - transaction has been successfully finished, the context
 * data is ready to be picked up by the transaction owner
 * - ABORTED - if the transaction times out, the timer handler marks
 * it aborted. The transaction owner is supposed to discard such transaction
 *
 * @{
 */

/** Describes the current state of the transaction:
 */
typedef enum {
	TCMI_TRANSACTION_INIT,
	TCMI_TRANSACTION_RUNNING,
	TCMI_TRANSACTION_COMPLETE,
	TCMI_TRANSACTION_ABORTED,
} tcmi_trans_state_t;

/** denotes an invalid transaction ID */
#define TCMI_TRANSACTION_INVAL_ID 0

/** Denotes a transaction, where nobody is waiting for the response */
#define TCMI_TRANS_FLAGS_ANONYMOUS 0x00000001
/** Denotes an expired transaction that hasn't been completed */
#define TCMI_TRANS_FLAGS_EXPIRED   0x00000002


/** Compound structure holding transaction attributes. */
struct tcmi_transaction {

	/** link for storing the transaction in a slot.  */
	tcmi_slot_node_t node;
	/** Slot where the transaction resides.  */
	struct tcmi_slot *slot;

	/** transaction ID. */
	u_int32_t id;
	
	/** response ID - verified upon transaction completion. */
	u_int32_t resp_id;


	/** transaction state. */
	tcmi_trans_state_t state;

	/** Flags describe additional attributes of the transaction */
	atomic_t flags;

	/** Sleeping processes, that are to be notified upon
	 * transaction time out or completion. */
	wait_queue_head_t wq;

	/** timer used for transaction timeouts. */
	struct timer_list timer;

	/** pointer to the transaction context data - assigned upon
	 * successful completion. */
	void *context;

	/** reference counter. */
	atomic_t ref_count;
	/** spin lock - serializes access to the transaction. */
	spinlock_t t_lock;
	
	/** Back-references to slotvec where the transaction is present.. used to get access to slotvec lock to guard agains lookup/put races. */
	struct tcmi_slotvec *transactions;
};

/** \<\<public\>\> Creates a new transaction. */
extern struct tcmi_transaction* tcmi_transaction_new(struct tcmi_slotvec *transactions, 
						     u_int32_t resp_id,
						     unsigned long timeout);
/** \<\<public\>\> Starts a transaction. */
extern int tcmi_transaction_start(struct tcmi_transaction *self, int flags);

/** \<\<public\>\> Aborting the transaction. */
extern void* tcmi_transaction_abort(struct tcmi_transaction *self);

/** \<\<public\>\> Completes a transaction and wakes up the processing thread. */
extern int tcmi_transaction_complete(struct tcmi_transaction *self, void *context, 
				     u_int32_t type_id);

/** \<\<public\>\> Tries to find a transaction based on its ID. */
extern struct tcmi_transaction* tcmi_transaction_lookup(struct tcmi_slotvec *transactions, 
							u_int32_t trans_id);

/** \<\<public\>\> Releases the instance - serialized version. */
extern void tcmi_transaction_put(struct tcmi_transaction *self);

/**
 * \<\<public\>\> Locks the instance data. Spin lock used - no sleeping allowed!
 *
 * @param *self - pointer to this transaction instance
 */
static inline void tcmi_transaction_lock(struct tcmi_transaction *self)
{
	spin_lock(&self->t_lock);
}

/**
 * \<\<public\>\> Unlocks the instance data
 *
 * @param *self - pointer to this transaction instance
 */
static inline void tcmi_transaction_unlock(struct tcmi_transaction *self)
{
	spin_unlock(&self->t_lock);
}


/** 
 * \<\<public\>\> Instance accessor. increments the reference counter.  The user is
 * now guaranteed, that the instance stays in memory while he is using
 * it. Assumes, the access is serialized by means of locking the slot
 * where the instance resides.
 *
 * @param *self - pointer to this instance
 * @return tcmi_transaction instance
 */
static inline struct tcmi_transaction* tcmi_transaction_get(struct tcmi_transaction *self)
{
	if (self)
		atomic_inc(&self->ref_count);
	return self;
}


/** 
 * \<\<public\>\> Transaction ID accessor.
 *
 * @param *self - pointer to this transaction instance
 * @return transaction ID
 */
static inline u_int32_t tcmi_transaction_id(struct tcmi_transaction *self)
{
	return self->id;
}

/** 
 * \<\<public\>\> Transaction state accessor.
 * The transaction should be locked by the caller, so that the transaction
 * state doesn't change when making decisions based on it.
 *
 * @param *self - pointer to this transaction instance
 * @return curren transaction state
 */
static inline tcmi_trans_state_t tcmi_transaction_state(struct tcmi_transaction *self)
{
	return self->state;
}


/**
 * \<\<public\>\> Transaction context accessor.
 *
 * @param *self - pointer to this transaction instance
 * @return pointer to the context associated with the transaction
 */
static inline void* tcmi_transaction_context(struct tcmi_transaction *self)
{
	return self->context;
}

/** 
 * \<\<public\>\> Sets flags in the transaction that denote additional
 * transaction attributes. Valid flags are:
 * - TCMI_TRANS_FLAGS_DELETED - transaction marked for deletion
 * - TCMI_TRANS_FLAGS_ANONYMOUS - transaction is anonymous, 
 * - TCMI_TRANS_FLAGS_EXPIRED - timer has expired.
 *
 * @param *self - pointer to this transaction instance
 * @param flags - additional flag setting 
 */
static inline void tcmi_transaction_set_flags(struct tcmi_transaction *self, int flags)
{
	atomic_set_mask(flags, &self->flags);
}

/** 
 * \<\<public\>\> Checks on a complete transaction.
 *
 * @param *self - pointer to this transaction instance
 * @return true if transaction has been successfully finished
 */
static inline int tcmi_transaction_is_complete(struct tcmi_transaction *self)
{
	return (self->state == TCMI_TRANSACTION_COMPLETE);
}

/** 
 * \<\<public\>\> Checks on aborted transaction.
 *
 * @param *self - pointer to this transaction instance
 * @return true if transaction has been aborted
 */
static inline int tcmi_transaction_is_aborted(struct tcmi_transaction *self)
{
	return (self->state == TCMI_TRANSACTION_ABORTED);
}

/** 
 * \<\<public\>\> Checks on a running transaction.
 *
 * @param *self - pointer to this transaction instance
 * @return true if transaction is still running..
 */
static inline int tcmi_transaction_is_running(struct tcmi_transaction *self)
{
	return (self->state == TCMI_TRANSACTION_RUNNING);
}

/** 
 * \<\<public\>\> Checks on an anonymous transaction.
 *
 * @param *self - pointer to this transaction instance
 * @return true if transaction is marked as anonymous.
 */
static inline int tcmi_transaction_is_anon(struct tcmi_transaction *self)
{
	return atomic_read(&self->flags) & TCMI_TRANS_FLAGS_ANONYMOUS;
}


/**
 * \<\<public\>\> Waits on a transaction till it expires or is
 * completed.
 *
 * @param *self - pointer to this transaction instance
 * @return transaction status
 */
static inline int tcmi_transaction_wait(struct tcmi_transaction *self)
{
	int ret;
	/* Sleep until the transaction times out or completes (response arrival) */
	wait_event(self->wq, (tcmi_transaction_is_aborted(self) || 
			      tcmi_transaction_is_complete(self)));
	ret = tcmi_transaction_state(self);

	return ret;
}

/**
 * \<\<public\>\> Waits on a transaction till it expires or is
 * completed, the task can be prematurely woken up by a signal.
 *
 * @param *self - pointer to this transaction instance
 * @return transaction status or -ERESTARTSYS on signal arrival
 */
static inline int tcmi_transaction_wait_interruptible(struct tcmi_transaction *self)
{
	int ret = 0;
	/* Sleep until the transaction times out or completes (response arrival) */
	ret = wait_event_interruptible(self->wq, (tcmi_transaction_is_aborted(self) || 
						  tcmi_transaction_is_complete(self)));
	if (ret < 0) 
		mdbg(INFO3, "Signal arrived");
	else 
		ret = tcmi_transaction_state(self);
	return ret;
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_TRANSACTION_PRIVATE

/** Generates a transaction ID.*/
static void tcmi_transaction_gen_id(struct tcmi_transaction *self);

/** Callback for the kernel timer that handles transaction timeouts. */
static void tcmi_transaction_timer(unsigned long trans);

/** 
 * \<\<private\>\> Checks on expired transaction.
 *
 * @param *self - pointer to this transaction instance
 * @return true if transaction is marked as expired.
 */
static inline int tcmi_transaction_is_expired(struct tcmi_transaction *self)
{
	return atomic_read(&self->flags) & TCMI_TRANS_FLAGS_EXPIRED;
}

/** 
 * \<\<private\>\> Marks the transaction as expired. This method is
 * internally used by the timer when it goes off. It is needed when
 * complete/abort operations drop the timer and check if reference
 * counter needs to be adjusted.
 *
 * @param *self - pointer to this transaction instance
 */
static inline void tcmi_transaction_set_expired(struct tcmi_transaction *self)
{
	tcmi_transaction_set_flags(self, TCMI_TRANS_FLAGS_EXPIRED);
}


/**
 * \<\<private\>\> Computes the hash value of the transaction ID.
 * 
 * The hash is computed as: ((trans_id >> 16) ^ trans_id) & hashmask
 * @param trans_id - transaction ID to be hashed
 * @param hashmask - hashmask limiting the maximum hash value.
 */
static inline u_int tcmi_transaction_hash(u_int32_t trans_id, u_int hashmask)
{
	return (((trans_id >> 16) ^ trans_id) & hashmask);
}

/** 
 * \<\<private\>\> Notifies the transaction owner by waking up a
 * corresponding thread.
 * 
 * @param *self - pointer to this transaction instance
 */
static inline void tcmi_transaction_notify_owner(struct tcmi_transaction *self)
{

	mdbg(INFO4, "Waking up transaction processing thread");
	wake_up(&self->wq);
}

#endif /* TCMI_TRANSACTION_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_TRANSACTION_H */

