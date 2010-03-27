/**
 * @file tcmi_slotvec_test.c - Basic test case for the slot vector datastructure.
 *                      - creates a new vector with 3slots
 *                      - tries inserting 5 objects into empty slots (only 3 will 
 *                        succeed)
 *                      - inserts the 4th object into the slot at position 0(the slot
 *                      will now contain 2 objects
 *                      - tests iterating through slot 0
 *                      - removes objects 0 and 3 from their slots -> since they reside
 *                        in the same slot, the slot will be marked empty. The safe 
 *                        version of removal is used that verifies that an object belongs 
 *                        to the slot from which it is being removed
 *                      - tests iterating through all used slots in the vector and then
 *                        through all objects within each used slot
 *                      - removes all objects from the slot vector.
 *
 * Date: 04/05/2005
 *
 * Author: Jan Capek
 *
 *
 * License....
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/list.h>

#include <tcmi/lib/tcmi_slotvec.h>

#include <dbg.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

struct object {
	/** used for linking objects within a slot, managed by
	 * tcmi_slot class, including the initialization */
	tcmi_slot_node_t node;

	/** This is just an example implentation. It stores the
	 * reference to the slot where the object has been stored. The
	 * user has to keep slot reference whereever he/she wants
	 * to.*/
	struct tcmi_slot *slot;
	char *text;
};
static struct object objects[] = {
	{
		.text = "object 0"
	},
	{
		.text = "object 1"
	},
	{
		.text = "object 2"
	},
	{
		.text = "object 3"
	},
	{
		.text = "object 4"
	},
};

#define OBJ_COUNT (sizeof(objects)/sizeof(struct object))

static struct tcmi_slotvec *vec = NULL;
/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_slotvec_test_init(void)
{
	int i;
	minfo(INFO1, "Initializing the slot vector");
	if ((vec = tcmi_slotvec_new(3)))
		minfo(INFO1, "Initialized slot vector %p", vec);

	minfo(INFO1,"------------------ INSERTION TESTING -----------------");
	/* The last one will fail for last 2 objects as there are only 3 slots available */
	for (i = 0; i < 5; i++) {
		if (!(objects[i].slot = tcmi_slotvec_insert_at_empty(vec, &objects[i].node)))
			minfo(ERR1, "Can't insert object %d (text=%s), no empty slot available!",
			      i, objects[i].text);
		else
			minfo(INFO1, "Inserted object %d (text=%s), into slot %d",
			      i, objects[i].text, tcmi_slot_index(objects[i].slot));
	}

	/* after this 2 objects will be in slot 0 */
	if (!(objects[3].slot = tcmi_slotvec_insert_at(vec, 0, &objects[3].node)))
		minfo(ERR4, "Can't insert, invalid slot specified");

	minfo(INFO1,"------------------------------------------------------");
	minfo(INFO1,"--------------- SLOT %d ITERATION TESTING ---------------", 
	      tcmi_slot_index(objects[3].slot));
	/* Iterate through all objects stored in slot where object 3 is */
	tcmi_slot_lock(objects[3].slot);

	tcmi_slot_node_t *nd, *nd2;
	struct object *object;
	/* Accesing by nodes and extracting the objects by tcmi_slot_entry */
	tcmi_slot_for_each(nd, objects[3].slot) {
		/* extract the original object from the slot node */
		object = tcmi_slot_entry(nd, struct object, node);
		minfo(INFO1,"Object text='%s'", object->text);
	}
	/* Accesing the slot node entrie's directly */
	tcmi_slot_for_each_entry(object, nd, objects[3].slot, node) {
		minfo(INFO1,"Object text='%s'", object->text);
	}

	/* Accesing the slot node entrie's directly, using a safe way
	 * - each node is copied */
	tcmi_slot_for_each_entry_safe(object, nd, nd2, objects[3].slot, node) {
		/* extract the original object from the slot node */
		minfo(INFO1,"Object text='%s'", object->text);
	}
	tcmi_slot_unlock(objects[3].slot);
	minfo(INFO1,"------------------------------------------------------");
	minfo(INFO1,"--------------- NODE REMOVAL IN SLOT 0  ---------------");
	/* remove both objects from slot 0, the second removal, will
	 * the locking is handled internally by the slot upon removal */
	tcmi_slot_remove_safe(objects[0].slot, &objects[0].node);
	tcmi_slot_remove_safe(objects[3].slot, &objects[3].node);
	minfo(INFO1,"------------------------------------------------------");

	minfo(INFO1,"----------- SLOT VECTOR ITERATION TESTING ------------");
	/* Iterates through all used slots. Display all objects in
	 * each slot - no removals, we can use 'non-safe' version */
	struct tcmi_slot *sl1, *sl2;
	tcmi_slotvec_lock(vec);
	tcmi_slotvec_for_each_used_slot(sl1, vec) {
		minfo(INFO1,"Searching slot %d", tcmi_slot_index(sl1));
		/* iterate through all objects in the slot. 'node'
		 * denotes the member name in struct object */
		tcmi_slot_for_each_entry(object, nd, sl1, node) {
			/* extract the original object from the slot node */
			minfo(INFO1,"Object text='%s'", object->text);
		}
		minfo(INFO1,"--------");
	}
	tcmi_slotvec_unlock(vec);
	minfo(INFO1,"------------------------------------------------------");
	minfo(INFO1,"------------------- REMOVAL TESTING ------------------");
	/* we will lock the entire slot vector and for the actual
	 * removal we use the __tcmi_slot_remove version that assumes
	 * the lock is already held There are removals, so we have to
	 * use the safe version for traversing */
	tcmi_slotvec_lock(vec);
	while (!tcmi_slotvec_empty(vec)) {
		tcmi_slotvec_remove_one_entry(vec, object, node);
		if (object) {
			minfo(INFO1,"Removed object text='%s'", object->text);
		}

	}
	tcmi_slotvec_unlock(vec);

	minfo(INFO1,"------------------------------------------------------");

	minfo(INFO1,"------------------- REMOVAL TESTING - 2 ------------------");
	/* we will lock the entire slot vector and for the actual
	 * removal we use the __tcmi_slot_remove version that assumes
	 * the lock is already held There are removals, so we have to
	 * use the safe version for traversing */
	tcmi_slotvec_lock(vec);
	tcmi_slotvec_for_each_used_slot_safe(sl1, sl2, vec) {
		minfo(INFO1,"Searching slot %d", tcmi_slot_index(sl1));
		tcmi_slot_for_each_entry_safe(object, nd, nd2, sl1, node) {
			/* extract the original object from the slot node */
			minfo(INFO1,"Removing object text='%s'", object->text);
			/* light removal, no checking, no locks held */
			__tcmi_slot_remove(sl1, &object->node);
		}
		minfo(INFO1,"--------");
	}
	tcmi_slotvec_unlock(vec);
	minfo(INFO1,"------------------------------------------------------");
	return 0;
}

/**
 * Module cleanup
 */
static void __exit tcmi_slotvec_test_exit(void)
{
	minfo(INFO1, "Destroying the slot vector %p", vec);
	tcmi_slotvec_put(vec);
}


module_init(tcmi_slotvec_test_init);
module_exit(tcmi_slotvec_test_exit);



