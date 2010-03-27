/**
 * @file tcmi_queue_test.c - Basic test case for the queue datastructure.
 *                      - creates a dynamic queue
 *                      - appends 3 objects to the queue.
 *                      - removes all 3 objects from the queue.
 *                      - declares & initializes a static queue
 *                      - repeats previous test.
 *
 * Date: 04/10/2005
 *
 * Author: Jan Capek
 *
 *
 * License....
 */
#include <linux/kernel.h>
#include <linux/module.h>

#include <tcmi/lib/tcmi_queue.h>

#include <dbg.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

struct object {
	/** used for linking objects within the queue */
	struct list_head node;
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

/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_queue_test_init(void)
{
	int i;
	struct tcmi_queue *dyn_q;

	minfo(INFO1, "Initializing the dynamic queue");
	if (!(dyn_q = tcmi_queue_new())) {
		minfo(ERR1, "Failed to create a new queue");
		goto exit0;
	}
	minfo(INFO1,"------------------ INSERTION TESTING -----------------");
	/* The last one will fail for last 2 objects as there are only 3 slots available */
	for (i = 0; i < 3; i++) {
		tcmi_queue_add(dyn_q, &objects[i].node);
	}
	struct object *obj;
	while(!tcmi_queue_empty(dyn_q)) {
		tcmi_queue_remove_entry(dyn_q, obj, node);
		minfo(INFO1, "Removed an object %p, %s", obj, obj->text);
	}
	tcmi_queue_remove_entry(dyn_q, obj, node);
	if (!obj) 
		minfo(INFO1, "No more objects available, queue is empty");


	dyn_q = tcmi_queue_get(dyn_q);
	tcmi_queue_put(dyn_q);
	tcmi_queue_put(dyn_q);

 exit0:
	return 0;
}

/**
 * Module cleanup
 */
static void __exit tcmi_queue_test_exit(void)
{
	minfo(INFO1, "Exiting..");
}


module_init(tcmi_queue_test_init);
module_exit(tcmi_queue_test_exit);



