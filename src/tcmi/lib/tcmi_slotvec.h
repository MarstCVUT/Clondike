/**
 * @file tcmi_slotvec.h - slot vector container used through out TCMI
 *                      
 * 
 *
 *
 * Date: 04/04/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_slotvec.h,v 1.3 2007/07/29 17:32:51 stavam2 Exp $
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
#ifndef _TCMI_SLOTVEC_H
#define _TCMI_SLOTVEC_H

#include <linux/list.h>
#include <asm/atomic.h>
#include <linux/spinlock.h>

#include "tcmi_slot.h"

/** @defgroup tcmi_slotvec_class tcmi_slotvec class 
 *
 * @ingroup tcmi_lib_group
 * 
 * This class represents a data structure widely used in
 * implementation of TCMI. It is a vector of slots where each slot is
 * head of a doubly linked list of the respective objects.
 \verbatim
    +-------+
   0| slot  | -> object1 -> object 2 -> object 3...
    +-------+
   1| slot  | empty
    +-------+
   2| slot  | -> object 5 -> object 6...
    +-------+
   3| slot  | empty 
    +-------+
    .       .
    .       .
    .       .
    +-------+
    | slot  | -> objectX
    +-------+
   n| slot  | empty
    +-------+
 \endverbatim
 *
 * - The vector has limited size specified during initialization by
 * the user. The benefit is that it provides O(1) access to all its
 * slots based on index.  
 * - Allows iterating through non-empty slots 
 * - Allows allocating an empty slot with O(1) time complexity
 * 
 * The data structure internally keeps track of used and unused slots.
 * The user can exclusively insert objects into empty slots
 * only. This way, it can be used as a data structure describing some
 * limited resource.  On the other hand(as seen in the figure), the
 * user is able to insert new elements into any slots, thus yielding a
 * hashtable. The user needs to select only a suitable hashing
 * function. The general requirement on the hash function is to yield
 * the index 0 to n-1 with an approximately uniform  distribution.
 *
 * Since the datastructure is expected to be accessed from multiple
 * threads of execution a reference counting is implemented. The user
 * should always access the slot vector by means of tcmi_slotvec_get()
 * method and release the instance by means of tcmi_slotvec_put().
 * The last accessing thread causes the slot vector to be freed from 
 * memory.
 * 
 *@{
 */

/** 
 * Iterates through all used slots in the slot vector.  This is
 * intended for traversals only, no removals should take place.  We
 * still need to ensure the slot vector protection against concurrent
 * manipulation from other threads.
 * 
 * @param slot - every iteration points to a new object
 * @param slotvec - slot vector we are iterating through
 */
#define tcmi_slotvec_for_each_used_slot(slot, slotvec) \
 list_for_each_entry(slot, &slotvec->used_slots, slot_list)

/** 
 * \<\<public\>\> Iterates through all used slots in the slot vector -
 * safe version.  This is a version safe against removals. If the
 * current slot, will be removed during the iteration, no issues will
 * arise. We still need to ensure the slot vector protection against
 * concurrent manipulation from other threads.
 * 
 * @param slot - every iteration points to a new object
 * @param slot2 - temporary storage for the current slot
 * @param slotvec - slot vector we are iterating through
 */
#define tcmi_slotvec_for_each_used_slot_safe(slot, slot2, slotvec) \
 list_for_each_entry_safe(slot, slot2, &slotvec->used_slots, slot_list)

/** Extracts the actual slot from the list */
#define tcmi_slotvec_entry(ptr, type, member) list_entry(ptr, type, member)


/** 
 * \<\<public\>\> Removes one entry from the slotvector and returns
 * it.  Takes first used slot and obtains first element from it.
 * Removes the element from the slot.
 *
 * @param slotvec - slotvector where the removal is to be done
 * @param obj - a pointer where to store the object retrieved from
 * @param member - name of the object member used for linking
 * into the slot.
 */
#define tcmi_slotvec_remove_one_entry(slotvec, obj, member)	\
do {								\
	struct tcmi_slot *__slot;				\
	obj = NULL;						\
	tcmi_slotvec_for_each_used_slot(__slot, slotvec) {	\
		tcmi_slot_remove_first(obj, __slot, member);	\
		break;						\
	}							\
} while(0)

/**
 * A slot vector compound structure. Contains the used/unused slot lists
 * and the actual vector.
 */
struct tcmi_slotvec {
	/** The actual slot vector */
	struct tcmi_slot *slots;
	/** Number of slots in the vector */
	u_int slot_count;
	/** Hash mask - valid only when slot_count is \$2^order\$ and
	 * the slot vector has been created by tcmi_slotvec_hnew(). */
	u_int hashmsk;

	/** list of unused slots */
	struct list_head unused_slots;
	/** list of used slots */
	struct list_head used_slots;

	/** reference counter */
	atomic_t ref_count;

	/** current instance reference - used when freeing */
	/* struct tcmi_slotvec *this;*/
	/** Protects the slotvector from concurrent modification */
	spinlock_t s_lock;
	/** Technical field used for irq safe spinlocks */
	unsigned long lock_flags;
};

/** Allocates a new slot vector. */
extern struct tcmi_slotvec* tcmi_slotvec_new(u_int size);

/** Allocates a new slot vector with 2^order buckets. */
extern struct tcmi_slotvec* tcmi_slotvec_hnew(u_int order);

/** 
 * Slot vector instance accessor. 
 * Checks whether the instance is not currently being freed - if so,
 * returns NULL. Otherwise increments the reference counter and
 * returns self.
 * 
 * @param *self - pointer to this vector instance
 * @return - self
 */
static inline struct tcmi_slotvec* tcmi_slotvec_get(struct tcmi_slotvec *self)
{
	if (!self)
		return NULL;
	atomic_inc(&self->ref_count);
	return self;
}

/** \<\<public\>\> Releases the instance. */ 
extern void tcmi_slotvec_put(struct tcmi_slotvec *self);

/** \<\<public\>\> Accesses a slot at a given index. */
extern struct tcmi_slot* tcmi_slotvec_at(struct tcmi_slotvec *self, u_int index);

/** \<\<public\>\> Finds the first empty slot. */
extern struct tcmi_slot* tcmi_slotvec_reserve_empty(struct tcmi_slotvec *self);

/** \<\<public\>\> Inserts an item in a slot at a given index. */
extern struct tcmi_slot* tcmi_slotvec_insert_at(struct tcmi_slotvec *self, u_int index,
						struct hlist_node *node);
/** \<\<public\>\> Inserts an item into an empty slot at a given index. */
extern struct tcmi_slot* tcmi_slotvec_insert_at_empty(struct tcmi_slotvec *self,
						      struct hlist_node *node);

/**
 * \<\<public\>\> Tester method for slotvec emptyness
 *
 * @param *self - pointer to this vector instance
 * @return 1 if the slotvector is empty
 */
static inline int tcmi_slotvec_empty(struct tcmi_slotvec *self)
{
	return list_empty(&self->used_slots);
}

/**
 * \<\<public\>\> Locks the instance data. Spin lock used - no
 * sleeping allowed!
 *
 * @param *self - pointer to this vector instance
 */
static inline void tcmi_slotvec_lock(struct tcmi_slotvec *self)
{
	// Have to be irqsave to avoid possible safe->unsafe conflict in forks
	spin_lock_irqsave(&self->s_lock, self->lock_flags);
}

/**
 * \<\<public\>\> Unlocks the instance data.
 *
 * @param *self - pointer to this vector instance
 */
static inline void tcmi_slotvec_unlock(struct tcmi_slotvec *self)
{
 	spin_unlock_irqrestore(&self->s_lock, self->lock_flags);
}

/** 
 * \<\<public\>\> Hashmask accessor. This method is to be used only if
 * the slot vector has been created by tcmi_slotvec_hnew() and is
 * intented to be used as a hashtable. Otherwise the hashmask is not
 * valid and defaults to 0.
 *
 * @return hashmask of the slot vector
 */
static inline u_int tcmi_slotvec_hashmask(struct tcmi_slotvec *self)
{
	return self->hashmsk;
}



/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SLOTVEC_PRIVATE

#endif /* TCMI_SLOTVEC_PRIVATE */


/**
 * @}
 */
#endif /* _TCMI_SLOTVEC_H */
