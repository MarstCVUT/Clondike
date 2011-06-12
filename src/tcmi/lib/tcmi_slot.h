/**
 * @file tcmi_slot.h - slot used as an SMP safe doubly link list
 *                      
 * 
 *
 *
 * Date: 04/04/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_slot.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#ifndef TCMI_SLOT_H
#define TCMI_SLOT_H

#include <linux/list.h>
#include <linux/types.h>
#include <linux/spinlock.h>

#include <dbg.h>

/** @defgroup tcmi_slot_class tcmi_slot class 
 *
 * @ingroup tcmi_lib_group
 * 
 * This class represents a slot that is part of a slot vector.  The
 * slot contains a head of a doubly linked list. This list is used for
 * storing objects. Objects within a slot are stored(linked together by
 * means of tcmi_slot_node_t. Each slot is identified by its number. That way,
 * the user can locate it in the \link tcmi_slotvec_class slot vector \endlink.
 *
 * Also the slots are internally linked together by the slot_list. The
 * slot is inserted into used_ or unused_slots list. This allows fast
 * allocation of empty slots. The unused_/used_slots list management
 * is done by each slot on its own. The references to
 * unused_/used_slots list are always shared by all slots within a
 * particular slot vector.
 *
 * An typical object that wants to be stored in a slot will look like
 * this:
 * \verbatim
 * struct object {
 *	tcmi_slot_node_t node;
 *       .
 *       .
 *      (other object data items)
 * };
 * 
 * \endverbatim
 * 
 * @{
 */
/** \<\<public\>\> Extracts the actual entry from a slot node */
#define tcmi_slot_entry(ptr, type, member) hlist_entry(ptr, type, member)

/** 
 * \<\<public\>\> Iterates through all nodes in the slot.
 *
 * @param node - every iteration points to a new node
 * @param slot - slot that we are iterating through
 */
#define tcmi_slot_for_each(node, slot) hlist_for_each(node, &slot->head)

/** 
 * \<\<public\>\> Iterates through all nodes in the slot.  This is
 * intended for traversals only, no removals should take place.  We
 * still need to ensure the slot protection against concurrent
 * manipulation from other threads.
 * 
 * @param object - every iteration points to a new object
 * @param node - temporary storage for the current tcmi_slot_node_t 
 * @param slot - slot that we are iterating through
 * @param member - name of the list node member in the object
 */
#define tcmi_slot_for_each_entry(object, node, slot, member) \
 hlist_for_each_entry(object, node, &slot->head, member)

/** 
 * \<\<public\>\> Iterates through all nodes in the slot - safe
 * version.  This is a version safe against removals. If the current
 * object, will be removed during the iteration, no issues will
 * arise. We still need to ensure the slot protection against
 * concurrent manipulation from other threads.
 *
 * @param object - every iteration points to a new object
 * @param node - temporary storage for the current tcmi_slot_node_t 
 * @param node2 - another temporary storage for the copy of the 
 * previous node tcmi_slot_node_t 
 * @param slot - slot that we are iterating through
 * @param member - name of the list node member in the object
 */
#define tcmi_slot_for_each_entry_safe(object, node, node2, slot, member) \
 hlist_for_each_entry_safe(object, node, node2, &slot->head, member)


/** 
 * \<\<public\>\> Returns the first object in the slot.
 *
 * @param object - will store the pointer to the object in the slot
 * @param slot - from which to take the first object
 * @param member - name of the list node member in the object
 */
#define tcmi_slot_find_first(object, slot, member)		\
do {								\
	tcmi_slot_node_t *__node; 				\
	tcmi_slot_for_each_entry(object, __node, slot, member) 	\
		break;						\
} while(0)

/** 
 * \<\<public\>\> Removes the first object in the slot.  If the slot
 * is empty, does nothing
 *
 * @param object - will store the pointer to the object in the slot
 * @param slot - from which to take the first object
 * @param member - name of the list node member in the object
 */
#define tcmi_slot_remove_first(object, slot, member)			\
do {									\
	tcmi_slot_node_t *__node; 					\
	tcmi_slot_for_each_entry(object, __node, slot, member) {	\
		__tcmi_slot_remove(slot, &object->member);		\
		break;							\
	}								\
} while(0)

/** A slot node is in fact a hlist node */
typedef struct hlist_node tcmi_slot_node_t;

/** Check for node being removed from the hlist */
#define tcmi_slot_node_unhashed(n) hlist_unhashed(n)
/**
 * A slot compound structure. Contains the list of items stored
 * items. Also there is another list head provided. This is for internal use
 * by the tcmi_slotvec class.
 */
struct tcmi_slot {
	/** The slot number */
	u_int index;

	/** used when linking slots in used or unused lists */
	struct list_head slot_list;


	/** head of the list of items stored in the slot */
	struct hlist_head head;
	/** Protects the slotvector from concurrent modification */
	/* spinlock_t slot_lock; */

	/* Following items come from the associated slot vector as we
	 * want the slot to do the management of unused/used lists on its own */
	/** list of unused slots */
	struct list_head *unused_slots;
	/** list of used slots */
	struct list_head *used_slots;
	/** Protects the unused/used_slots lists from concurrent
	 * modification. Also, this lock is used when
	 * modifying the slot itself. Maybe, it would be a good idea
	 * to have a separate lock for each slot in addition to this
	 * one. The drawback would be the increased size of the
	 * slot. I will keep it like this for now */
	spinlock_t *s_lock;
	/** Technical field used for irq safe spinlocks */
	unsigned long lock_flags;
};

/**
 * Initializes a slot. Assigns it an index and appends it to the tail
 * of the unused_slots list. The slot keeps track whether it is empty
 * or not on its own. That's why we store the unused_slots and
 * used_slots heads inside the slot. The spinlock protects those lists
 * from concurrent modification. 
 * 
 * @param *self - pointer to this slot instance
 * @param index - slot index
 * @param unused_slots - head of the list of unused slots
 * @param used_slots - head of the list of used slots
 * @param *lock - pointer to the lock that will be used when
 * synchronizing access to the slotvector where the slot resides.
 */
static inline void tcmi_slot_init(struct tcmi_slot *self, u_int index, 
				  struct list_head *unused_slots,
				  struct list_head *used_slots, spinlock_t *lock)
{
	self->index = index;
	INIT_LIST_HEAD(&self->slot_list);
	INIT_HLIST_HEAD(&self->head);	
	self->unused_slots = unused_slots;
	self->used_slots = used_slots;
	self->s_lock = lock;
	list_add_tail(&self->slot_list, unused_slots);
}

/** 
 * \<\<public\>\> Checks if the slot is empty.
 *
 * @param *self - pointer to this slot instance
 * @return 1 - when the slot is empty
 */
static inline int tcmi_slot_empty(struct tcmi_slot *self)
{
	return hlist_empty(&self->head);
}

/** 
 * \<\<public\>\> Slot index accessor.
 *
 * @param *self - pointer to this slot instance
 * @return slot index
 */
static inline u_int tcmi_slot_index(struct tcmi_slot *self)
{
	return self->index;
}

/**
 * \<\<public\>\> Locks the slot data. Spin lock used - no sleeping
 * allowed!
 *
 * @param *self - pointer to this slot instance
 */
static inline void tcmi_slot_lock(struct tcmi_slot *self)
{
	// Have to be irqsave to avoid possible safe->unsafe conflict in forks
	spin_lock_irqsave(self->s_lock, self->lock_flags);
}

/**
 * \<\<public\>\> Unlocks the instance data
 *
 * @param *self - pointer to this slot instance
 */
static inline void tcmi_slot_unlock(struct tcmi_slot *self)
{
	spin_unlock_irqrestore(self->s_lock, self->lock_flags);
}

/**
 * \<\<private\>\> Moves a slot into the specified list.
 *
 * @param *self - pointer to this slot instance
 * @param head - list where the slot should be moved
 */
static inline void tcmi_slot_move(struct tcmi_slot *self, struct list_head *head) 
{
	list_move(&self->slot_list, head);
}

/**
 * \<\<private\>\> Moves a slot to the tail of the specified list.
 *
 * @param *self - pointer to this slot instance
 * @param head - list where the slot should be moved
 */
static inline void tcmi_slot_move_tail(struct tcmi_slot *self, struct list_head *head) 
{
	list_move_tail(&self->slot_list, head);
}

/** 
 * \<\<public\>\> Inserts a new node into the slot. It is necessary to
 * acquire a lock to prevent races with other threads. The slot is
 * also moved into the used_slots list. This might be considered as
 * overhead operation if the slot is already in that list, but it
 * simplifies the design. Otherwise we would need an extra flag
 * indicating which list it is currently in. Also the operation is
 * O(1), so the penalty is rather small.
 *
 * @param *self - pointer to this slot instance
 * @param *node - pointer to the node to be added
 *
 */
static inline void tcmi_slot_insert(struct tcmi_slot *self, tcmi_slot_node_t *node)
{
	tcmi_slot_lock(self);
	mdbg(INFO4, "Inserted node %p into slot %d", node, self->index);
	INIT_HLIST_NODE(node);
	hlist_add_head(node, &self->head);
	tcmi_slot_move(self, self->used_slots);
	tcmi_slot_unlock(self);
}


/** 
 * \<\<public\>\> Moves a slot among unused_slots.
 *
 * @param *self - pointer to this slot instance
 */
static inline void tcmi_slot_move_unused(struct tcmi_slot *self)
{
	tcmi_slot_move(self, self->unused_slots);
}
/** 
 * \<\<public\>\> Removes a given node from a slot. No check is
 * performed, whether the node belongs to this slot. If the slot
 * becomes empty by removeing the node, it will be moved to the unused
 * slot list.
 *
 * \note This method assumes the s_lock is held.
 *
 * @param *self - pointer to this slot instance
 * @param *node - pointer to the node to be removed
 */
static inline void __tcmi_slot_remove(struct tcmi_slot *self, tcmi_slot_node_t *node)
{
	hlist_del_init(node);
	if (tcmi_slot_empty(self)) {
		tcmi_slot_move_unused(self);

		mdbg(INFO4, "Slot %d is now empty, moving to unused slots", self->index);
	}
}

/** 
 * \<\<public\>\> Removes a given node from a slot. No check is
 * performed, whether the node belongs to this slot. After locking the
 * slot, the work is delegated to __tcmi_slot_remove().
 *
 * @param *self - pointer to this slot instance
 * @param *node - pointer to the node to be removed
 */
static inline void tcmi_slot_remove(struct tcmi_slot *self, tcmi_slot_node_t *node)
{
	if (self) {
		if ( spin_is_locked(self->s_lock) ) {
		    __tcmi_slot_remove(self, node);
		} else {
		    tcmi_slot_lock(self);
		    __tcmi_slot_remove(self, node);
		    tcmi_slot_unlock(self);		  
		}	  
	}
}


/** 
 * \<\<public\>\> Removes a given node from a slot. A check is
 * performed whether the node belongs to this slot. The work is then
 * delegated to tcmi_slot_remove().
 *
 * @param *self - pointer to this slot instance
 * @param *node - pointer to the node to be removed
 */
static inline void tcmi_slot_remove_safe(struct tcmi_slot *self, tcmi_slot_node_t *node)
{
	struct hlist_node *slot_node;
	int found = 0;
	if (self) {
		tcmi_slot_lock(self);
		tcmi_slot_for_each(slot_node, self) {
			if (slot_node == node) {
				found = 1;
				break;
			}
		}
		if (found) {
			mdbg(INFO4, "Found the node %p in slot %d", node, self->index);
			__tcmi_slot_remove(self, node);
		}
		tcmi_slot_unlock(self);
	}
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SLOT_PRIVATE

#endif /* TCMI_SLOT_PRIVATE */


/**
 * @}
 */
#endif /* TCMI_SLOT_H */
