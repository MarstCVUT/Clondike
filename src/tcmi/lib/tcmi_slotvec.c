/**
 * @file tcmi_slotvec.c - slot vector container used through out TCMI 
 *                      - implementation
 * 
 *
 *
 * Date: 04/05/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_slotvec.c,v 1.3 2007/09/02 10:54:43 stavam2 Exp $
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


#include <linux/slab.h>

#include "tcmi_slotvec.h"

#include <dbg.h>

/** 
 * \<\<public\>\> Creates new instance.
 * - Vector of slots is allocated and all slots are initialized.
 * Since we want the slot to do the management of unused/used lists
 * on their own, we pass the vector lock as well as both list heads
 * to the init method.
 * - reference counter is initialized to 1 - the instantiator
 * owns this slot vector now
 * 
 * @param size - number of slots to be allocated in the vector.
 * @return a new slot vector or NULL
 */
struct tcmi_slotvec* tcmi_slotvec_new(u_int size)
{
	struct tcmi_slotvec *vec;
	struct tcmi_slot *slots;
	int i;
	if (!(vec = kmalloc(sizeof(struct tcmi_slotvec), GFP_ATOMIC))) {
		mdbg(ERR3, "Failed to allocate a slotvector");
		goto exit0;
	}
	if (!(slots = kmalloc(sizeof(struct tcmi_slot) * size, GFP_ATOMIC))) {
		mdbg(ERR3, "Failed to allocate a %d slots", size);
		goto exit1;
	}
	vec->slots = slots;
	vec->slot_count = size;
	INIT_LIST_HEAD(&vec->unused_slots);
	INIT_LIST_HEAD(&vec->used_slots);
	atomic_set(&vec->ref_count, 1);
	spin_lock_init(&vec->s_lock);
	/* initialize all slots */
	for (i = 0; i < size; i++) {
		tcmi_slot_init(slots, i, &vec->unused_slots, 
			       &vec->used_slots, &vec->s_lock);
		slots++;
	}
	mdbg(INFO4, "Allocated %d slots (%lu bytes), memory=%p", size, 
	     (unsigned long)(sizeof(struct tcmi_slot) * size), vec);
	return vec;

	/* error handling */
 exit1:
	kfree(vec);
 exit0:
	return NULL;
}

/**
 * \<\<public\>\> This method creates a slot vector of the size
 * \$2^{order}\$.  It is intended when using the slot vector as a
 * hashtable with \$2^{order}\$ buckets. A hashmask is calculated as
 * the vector size - 1.
 *
 * @param order - \$log_2(buckets)\$ 
 * @return a new slot vector or NULL
 */
struct tcmi_slotvec* tcmi_slotvec_hnew(u_int order)
{
	struct tcmi_slotvec *vec;
	if (!(vec = tcmi_slotvec_new(1 << order))) {
		mdbg(ERR3, "Error creating a slot vector hashtable with 2^%d buckets", order);
		goto exit0;
	}
	/* precompute the hashmask */
	vec->hashmsk = vec->slot_count - 1;
	return vec;
	/* error handling */
 exit0:
	return NULL;
}

/**
 * \<\<public\>\> Decrements the reference counter, when the count
 * reaches 0, the instance is destroyed. This requires releasing all
 * slots and eventually releasing the instance itself.
 *
 * @param *self - pointer to this vector instance
 */
void tcmi_slotvec_put(struct tcmi_slotvec *self)
{
	struct tcmi_slot *slots;
	int i;
	if (!self)
		return;
	mdbg(INFO3, "Putting ref. count=%d (%p)", 
	     atomic_read(&self->ref_count), self);
	if (atomic_dec_and_test(&self->ref_count)) {
		slots = self->slots;
		for (i = 0; i < self->slot_count; i++, slots++) {
			if (!(tcmi_slot_empty(slots)))
			    mdbg(WARN3, "Warning - non empty slot %d", i);
		}
		mdbg(INFO4, "Released %d slots (%lu bytes), memory=%p", 
		     self->slot_count, 
		     (unsigned long)(sizeof(struct tcmi_slot) * self->slot_count), self);
		kfree(self->slots);
		kfree(self);
	}

}

/** 
 * \<\<public\>\> Accesses a slot at a given index. If the index is
 * not valid (exceeds the vector length), NULL is returned.
 *
 * @param *self - pointer to this vector instance
 * @param index - index of a slot to be accessed.
 * @return pointer to a slot at a given index or NULL
 */
struct tcmi_slot* tcmi_slotvec_at(struct tcmi_slotvec *self, u_int index)
{
	struct tcmi_slot *slot = NULL;
	if (index < self->slot_count)
		slot = &self->slots[index];

	return slot;
}

/** 
 * \<\<public\>\> Finds the first empty slot and reserves it.  The
 * slot is removed from the unused list, so that other threads won't
 * consider it when searching for empty slots. The user of this method
 * is responsible for either filling the slot with some data or
 * releasing the slot back to unused list by means of
 * tcmi_slot.h::tcmi_slot_move_unused()
 *
 * @param *self - pointer to this vector instance
 * @return pointer to an empty slot or NULL
 */
struct tcmi_slot* tcmi_slotvec_reserve_empty(struct tcmi_slotvec *self)
{
	struct tcmi_slot *slot = NULL;
	struct list_head *list;

	tcmi_slotvec_lock(self);
	if (!list_empty(&self->unused_slots)) {
		/* get the first slot */
		list_for_each(list, &self->unused_slots)
			break;
		slot = list_entry(list, struct tcmi_slot, slot_list);
		/* remove the slot, from the unused list */
		list_del_init(list);
		mdbg(INFO4, "Found empty slot index = %d", slot->index);
	}
	tcmi_slotvec_unlock(self);

	return slot;
}


/** 
 * \<\<public\>\> The index where the insertion takes place has to be
 * a valid slot.  The work is then delegated to the slot.
 *
 * @param *self - pointer to this vector instance
 * @param index - index of a slot to be accessed.
 * @param *node - item to be inserted into an empty slot
 * @return pointer to a slot at a given index where the node has been
 * inserted or NULL
 */
struct tcmi_slot* tcmi_slotvec_insert_at(struct tcmi_slotvec *self, u_int index,
					 struct hlist_node *node)
{
	struct tcmi_slot *slot = tcmi_slotvec_at(self, index);
	if (slot)
		tcmi_slot_insert(slot, node);
	return slot;
}
/**
 * \<\<public\>\> Takes the first slot from unused slot list and
 * removes it from the list. The slot is then asked to insert the new
 * node.
 *
 * @param *self - pointer to this vector instance
 * @param *node - item to be inserted into an empty slot
 * @return a pointer to the slot where the insertion took place
 * or NULL
 */
struct tcmi_slot* tcmi_slotvec_insert_at_empty(struct tcmi_slotvec *self,
					       struct hlist_node *node)
{
	struct tcmi_slot *slot = tcmi_slotvec_reserve_empty(self);
	if (slot)
		tcmi_slot_insert(slot, node);
	return slot;
}
