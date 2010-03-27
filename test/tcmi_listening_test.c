/**
 * @file tcmi_listening_test.c - Basic test case for tcmi_listening
 *                      - creates a new slot vector with 3slots
 *                      - tries creating 4 listenings - 1 should fail as there is not enough
 *                      free slots.
 *                      - removes the second listening.
 *                      - tries inserting the 4th listening again.
 *
 * Date: 04/08/2005
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

#include <tcmi/lib/tcmi_listening.h>
#include <tcmi/lib/tcmi_slotvec.h>
#include <tcmi/ctlfs/tcmi_ctlfs.h>

#include <dbg.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

static struct tcmi_listening *listenings[] = {
	NULL,
	NULL,
	NULL,
	NULL
};
static char *ifaces[] = {
	"tcp:192.168.0.1",
	"tcp:192.168.0.2",
	"tcp:192.168.0.3",
	"tcp:192.168.0.4",
};

#define OBJ_COUNT (sizeof(listenings)/sizeof(struct tcmi_listening*))

static struct tcmi_slotvec *vec = NULL;
/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_listening_test_init(void)
{
	int i;
	struct tcmi_ctlfs_dir *root = tcmi_ctlfs_get_root();
	minfo(INFO1, "Initializing the slot vector");
	if ((vec = tcmi_slotvec_new(3)))
		minfo(INFO1, "Initialized slot vector %p", vec);
	
	minfo(INFO1,"------------------ INSERTION TESTING -----------------");
	/* The last one will fail for last 2 objects as there are only 3 slots available */
	for (i = 0; i < OBJ_COUNT; i++) {
		listenings[i] = tcmi_listening_new(root, vec, NULL, ifaces[i]);
		if (!listenings[i])
			minfo(INFO1, "Failed to create a listening on interface '%s'", 
			      ifaces[i]);
	}

	minfo(INFO1,"------------------- REPLACING LISTENING IN SLOT 1 ------------------");
	struct tcmi_slot *sl;
	tcmi_slotvec_lock(vec);
	/* get the slot 1 */
	sl = tcmi_slotvec_at(vec, 1);
	if (sl) {
		struct tcmi_listening *listening = NULL;
		/* retrieve the first object in the slot */
		tcmi_slot_find_first(listening, sl, node);
		if (listening) {
			minfo(INFO1, "Found listening in slot 1, releasing..");
			/* instance release, locks on the slot are held already */
			tcmi_listening_put(listening);
		}
	}
	tcmi_slotvec_unlock(vec);
	/* Try instantiating listening 3 again */
	listenings[3] = tcmi_listening_new(root, vec, NULL, ifaces[3]);
	minfo(INFO1,"------------------------------------------------------");

	return 0;
}

/**
 * Module cleanup
 */
static void __exit tcmi_listening_test_exit(void)
{
	minfo(INFO1,"------------------- REMOVAL TESTING ------------------");
	struct tcmi_slot *sl1, *sl2;
	/* we will lock the entire slot vector and for the actual
	 * removal we use the __tcmi_slot_remove version that assumes
	 * the lock is already held There are removals, so we have to
	 * use the safe version for traversing */
	tcmi_slotvec_lock(vec);
	tcmi_slotvec_for_each_used_slot_safe(sl1, sl2, vec) {
		/* slot nodes needed for safe traversal */
		tcmi_slot_node_t *nd, *nd2; 
		struct tcmi_listening *listening;
		minfo(INFO1,"Searching slot %d", tcmi_slot_index(sl1));
		tcmi_slot_for_each_entry_safe(listening, nd, nd2, sl1, node) {
			minfo(INFO1,"Removing listening..");
			/* instance release, locks on the slot are held already */
			tcmi_listening_put(listening);
		}
		minfo(INFO1,"--------");
	}
	tcmi_slotvec_unlock(vec);
	minfo(INFO1,"------------------------------------------------------");

	minfo(INFO1, "Destroying the slot vector %p", vec);
	tcmi_slotvec_put(vec);
	tcmi_ctlfs_put_root();
}


module_init(tcmi_listening_test_init);
module_exit(tcmi_listening_test_exit);
