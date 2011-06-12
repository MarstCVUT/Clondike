/**
 * @file tcmi_transaction.c - TCMI communication transaction.
 *       
 *                      
 * 
 *
 *
 * Date: 04/10/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_transaction.c,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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

#include <linux/types.h>
#include <linux/random.h>
#include <linux/jiffies.h>

#define TCMI_TRANSACTION_PRIVATE
#include "tcmi_transaction.h"


#include <dbg.h>

/** 
 * \<\<public\>\> Creates a new transaction.
 * Instance is created as follows:
 * - allocate the instance
 * - generate a transaction ID (random number)
 * - initilialize wait queue for process(es) that is/are to be woken 
 * up when:
 *         - the transaction times out or
 *         - when it is sucessfully completed
 * - store timeout that will be activated when the transaction is
 * started.
 * - store the transaction in the transaction slot vector specified by
 * the user
 * - setup reference counter
 * - clear transaction flags - the transaction owner will adjust the
 * flags afterwards
 * - one instance reference is for the timer and one instance
 * reference is returned to the instantiator
 *
 * @param *transactions - storage for the transactions
 * @param resp_id - response message ID - required for the transaction so that match
 * the response upon completion.
 * @param timeout - transaction time out
 * @return new transaction or NULL
 */
struct tcmi_transaction* tcmi_transaction_new(struct tcmi_slotvec *transactions, 
					      u_int32_t resp_id,
					      unsigned long timeout)
{
	struct tcmi_transaction *trans;
	/* hash of the transaction ID */
	u_int hash;

	if (!(trans = kmalloc(sizeof(struct tcmi_transaction), GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate memory for a transaction");
		goto exit0;
	}
	tcmi_transaction_gen_id(trans);
	hash = tcmi_transaction_hash(trans->id, tcmi_slotvec_hashmask(transactions));

	trans->resp_id = resp_id;
	trans->state = TCMI_TRANSACTION_INIT;
	trans->transactions = transactions;
	init_waitqueue_head(&trans->wq);

	/* setup the timer that handles transaction timeout */
	init_timer(&trans->timer);
	/* the expiration has to be added the current value of jiffies
	 * when the timer is started, but this is upon transaction start */
	trans->timer.expires = timeout; 
	trans->timer.data = (unsigned long) trans;
	trans->timer.function = tcmi_transaction_timer;
	/* context will be filled in upon response arrival */
	trans->context = NULL;
	/* one reference goes the the transaction instantiator */
	atomic_set(&trans->ref_count, 1);
	atomic_set(&trans->flags, 0);
	spin_lock_init(&trans->t_lock);

	/* now, the transaction is ready to be inserted into the slot vector */
	if (!(trans->slot = tcmi_slotvec_insert_at(transactions, hash, &trans->node))) {
		minfo(ERR3, "Can't insert transaction into slot vector!");
		goto exit1;
	}
	mdbg(INFO4, "Allocated transaction ID=%x, memory=%p", 
	     tcmi_transaction_id(trans), trans);

	return trans;

	/* error handling */
 exit1:
	kfree(trans);
 exit0:
	return NULL;
}


/** 
 * \<\<public\>\> Starts a transaction.  The transaction is started
 * only if it is in INIT state.  The timer is started, so that a
 * timeout can be handled automatically. The timer is assigned an extra
 * reference for the transaction to prevent destruction of the transaction
 * before the timer goes off in case of timeout.
 *
 * @param *self - this transaction instance
 * @param flags - flags to be set on the running transaction
 * @return 0 upon successful start of the transaction 
 */
int tcmi_transaction_start(struct tcmi_transaction *self, int flags)
{
	int err = 0;
	tcmi_transaction_lock(self);
	if (self->state != TCMI_TRANSACTION_INIT) {
		mdbg(ERR3, "Can't start - transaction (ID=%x),"
		     " not in INIT state! state=%d", self->id, self->state);
		err = -EINVAL;
		goto exit0;
	}
	/* reference counter for the timer */
	tcmi_transaction_get(self);
	tcmi_transaction_set_flags(self, flags);
	self->timer.expires += jiffies;
	self->state = TCMI_TRANSACTION_RUNNING;
	mdbg(INFO3, "Starting transaction ID %x, response ID %x", 
	     self->id, self->resp_id);
	add_timer(&self->timer);

 exit0:
	tcmi_transaction_unlock(self);
	return err;
}

/** 
 * \<\<public\>\> Aborting the transaction requires:
 * - deleting the timer and checking if it hasn't expired
 * - if the timer has been deleted without expiring, it is necessary
 * to drop the reference that the timer was retaining
 * - proceed further if the transaction is in INIT or RUNNING state
 * and change the state to ABORTED.
 * - notifying the transaction owner
 *
 * @param *self - pointer to this transaction instance
 * @return if the transaction has already been completed, it
 * will return a valid context and doesn't change the transaction state.
 * An aborted transaction has still its context set to NULL.
 */
void* tcmi_transaction_abort(struct tcmi_transaction *self)
{
	int put_needed = 0;
  
	del_timer_sync(&self->timer);
	tcmi_transaction_lock(self);
	if (!tcmi_transaction_is_expired(self)) {
		mdbg(INFO3, "Transaction hasn't expired yet, dropping timer reference.");
		tcmi_transaction_set_expired(self);
		put_needed = 1;
	}
	/* Transaction can be aborted from RUNNING as well as from
	 * INIT state. */
	if (!tcmi_transaction_is_complete(self)) {
		mdbg(INFO3, "Aborting..");
		self->state = TCMI_TRANSACTION_ABORTED;
		tcmi_transaction_notify_owner(self);
	}
	tcmi_transaction_unlock(self);

	if ( put_needed ) {
	    tcmi_transaction_put(self);
	    return NULL;
	}
	
	return self->context;
}

/** 
 * \<\<public\>\> Completes a transaction and wakes up the processing
 * thread.  The completion requires:
 * - deleting the running timer and checking if it hasn't expired
 * - if the timer has been deleted without expiring, it is necessary
 * to drop the reference that the timer was retaining
 * - proceed further if the transaction is in running state ONLY
 * - verifying, that the response ID in the transaction matches
 * caller's expected response ID. The transaction is aborted if those
 * two ID's don't match as apparently a wrong response has
 * arrived. 
 * - upon successful completion, the context specified by the
 * user, is associated with the transaction. That way, the transaction
 * owner can read any data related to a particular transaction, when
 * it picks up the completed transaction.
 * - notifying the transaction owner
 *
 * Completing a transaction is performed under transaction lock being
 * held.
 *
 * @param *self - this transaction instance
 * @param *context - context that is to be assigned to 
 * the transaction upon successful completion.
 * @param resp_id - response message ID that must match the ID set
 * initially in the transaction.
 * @return 0 when sucessfully completed
 */
int tcmi_transaction_complete(struct tcmi_transaction *self, void *context, 
			      u_int32_t resp_id)
{
	/* default is error */
	int err = -EINVAL;
	int put_needed = 0;
	
	del_timer_sync(&self->timer);
	tcmi_transaction_lock(self);
	if (!tcmi_transaction_is_expired(self)) {
		mdbg(INFO3, "Transaction hasn't expired yet, dropping timer reference.");
		tcmi_transaction_set_expired(self);
		put_needed = 1;
	}
	mdbg(INFO3, "Transaction state: %d", tcmi_transaction_state(self));
	if (!tcmi_transaction_is_running(self)) {
		mdbg(INFO3, "Can't complete-transaction ID %x is not running!", self->id);
		goto exit0;
	}
	if (self->resp_id == resp_id) {
		mdbg(INFO3, "Successfully completing transaction ID %x", self->id);
		self->state = TCMI_TRANSACTION_COMPLETE;
		self->context = context;
		err = 0;
	}
	else {
		self->state = TCMI_TRANSACTION_ABORTED;
		mdbg(ERR4, "Aborting transaction ID %x, resp_id(%x)"
		     "doesn't match the expected resp_id(%x)!", 
		     self->id, resp_id, self->resp_id);
	}
	/* In every case notify the transaction owner*/
	tcmi_transaction_notify_owner(self);

 exit0:
	tcmi_transaction_unlock(self);
	
	if ( put_needed )
	    tcmi_transaction_put(self);

	return err;
}

/**
 * \<\<public\>\> Tries to find a transaction based on its ID.  This
 * is a class method that tries to find a transaction instance. It
 * computes a hash of the transaction ID and searches the slot at
 * index equal to the hash value. 
 * 
 * The access to the transaction is serialized via the slotvector that
 * is being searched.
 *
 * @param *transactions - slot vector where to perform the search
 * @param trans_id - ID used when searching for the transaction.
 * @return transaction if found or NULL
 */
struct tcmi_transaction* tcmi_transaction_lookup(struct tcmi_slotvec *transactions, 
						 u_int32_t trans_id)
{
	u_int hash;
	/* slot where the hash of the transaction ID points to */
	struct tcmi_slot *slot;
	struct tcmi_transaction *tran = NULL;
	/* temporary storage for the slot elements, needed by the iterator */
	tcmi_slot_node_t *nd; 

	hash = tcmi_transaction_hash(trans_id, tcmi_slotvec_hashmask(transactions));

	tcmi_slotvec_lock(transactions);
	/* get the slot, the hash is pointing too */
	if (!(slot = tcmi_slotvec_at(transactions, hash))) {
		mdbg(ERR3, "Can't locate slot");
		goto exit0;
	}
		
	/* Search the slot for matching transaction */
	tcmi_slot_for_each_entry(tran, nd, slot, node) {
		if (tcmi_transaction_id(tran) == trans_id) {
			minfo(INFO4,"Found matching transaction ID %x", trans_id);			
			break;
		}

	}
	/* succesfully found a transaction, adjust its reference counter */
	if (tran)
		tcmi_transaction_get(tran);

	tcmi_slotvec_unlock(transactions);

	return tran;

/* error handling */
 exit0:
	tcmi_slotvec_unlock(transactions);
	return NULL;
}


/** 
 * \<\<public\>\> Releases the instance. Decrements reference counter
 * and if it reaches 0, the transaction is removed from its slot and
 * freed.
 *
 * @param *self - pointer to this instance
 */
void tcmi_transaction_put(struct tcmi_transaction *self)
{
	struct tcmi_slotvec *transactions;
	
	if ( !self ) 
	    return;
	
	transactions = self->transactions;
	
	// Transactions slotvec lock used to guard against lookup/put races
	tcmi_slotvec_lock(transactions);
  
	if (self && atomic_dec_and_test(&self->ref_count)) {
		mdbg(INFO4, "Destroying transaction ID=%x, memory=%p", 
		     tcmi_transaction_id(self), self);
		tcmi_slot_remove(self->slot, &self->node);
		kfree(self);
	}
	
	tcmi_slotvec_unlock(transactions);
}

/** @addtogroup tcmi_transaction_class
 *
 * @{
 */

/** 
 * \<\<private\>\> Generates a transaction ID.
 *
 * @param *self - this transaction instance
 */
static void tcmi_transaction_gen_id(struct tcmi_transaction *self)
{
	u_int32_t id = TCMI_TRANSACTION_INVAL_ID;

	while (id == TCMI_TRANSACTION_INVAL_ID) {
		get_random_bytes(&id, sizeof(u_int32_t));
	}
	self->id = id;
	mdbg(INFO4, "Generated new transaction ID: %x", id);
}

/** 
 * \<\<private\>\> Callback for the kernel timer that handles
 * transaction timeouts.  This method is called upon timer expiration
 * and means that no response has arrived so far, the transaction
 * needs to be aborted.  Expired flag is set to indicate that the
 * timer has executed. This  means that the 'complete' or 'abort'
 * operations won't try to release the timer reference.
 * 
 * Also, the thread that handles the transaction is woken up. 
 *
 * Other transaction operations (complete, abort) are serialized with
 * this method by means of del_timer_sync() - they cannot proceed any
 * further until this method finishes.
 *
 * @param trans - pointer to the transaction instance
 */
static void tcmi_transaction_timer(unsigned long trans)
{
	struct tcmi_transaction *self = (struct tcmi_transaction*) trans;
	self->state = TCMI_TRANSACTION_ABORTED;
	mdbg(INFO3, "Transaction ID %x expired, aborting..", self->id);
	tcmi_transaction_set_expired(self);
	tcmi_transaction_notify_owner(self);

	tcmi_transaction_put(self);
}


/**
 * @}
 */
