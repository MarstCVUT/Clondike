/**
 * @file tcmi_transaction_test.c - Basic test case for the TCMI transactions.
 *                      - creates a new hash for transaction with 2^3 buckets
 *                      - creates a thread that will handle both transactions
 *                      - creates 3 new transactions all with a 2 second time out.
 *                      - starts all transaction transactions.
 *                      - completes the first transaction setting a matching
 *                        response ID.
 *                      - completes the second transaction, sets a wrong response
 *                        ID - the transaction  will be aborted
 *                      - leaves the last transaction to timeout.
 * 
 *
 *
 * Date: 04/11/2005
 *
 * Author: Jan Capek
 *
 *
 * License....
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/list.h>
#include <linux/kthread.h>

#include <tcmi/lib/tcmi_slotvec.h>
#include <tcmi/comm/tcmi_transaction.h>

#include <dbg.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

/* This object type is stored into the transaction as its context upon
 * completion */
struct object {
	char *text;
};
/* a few objects for testing */
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

static struct tcmi_slotvec *transactions = NULL;

/** 
 * Prints out details about a transaction, for completed transactions
 * a object contained in it is displayed
 */
static void handle_tran(struct tcmi_transaction *tran) 
{
	struct object *obj;

	switch (tcmi_transaction_state(tran)) {
	case TCMI_TRANSACTION_INIT:
		minfo(INFO1, "Transaction (%x) is in INIT state deleting..", 
		      tcmi_transaction_id(tran));
		break;
	case TCMI_TRANSACTION_RUNNING:
		minfo(INFO1, "Transaction (%x) is in RUNNING state deleting..", 
		      tcmi_transaction_id(tran));
		break;
	case TCMI_TRANSACTION_ABORTED:
		minfo(INFO1, "Transaction (%x) is in ABORTED state deleting..", 
		      tcmi_transaction_id(tran));
		break;
	case TCMI_TRANSACTION_COMPLETE:
		minfo(INFO1, "Transaction (%x) is in COMPLETE state deleting..", 
		      tcmi_transaction_id(tran));
		obj = tcmi_transaction_context(tran);
		minfo(INFO1, "Context object is '%s'", obj->text);
		break;
	}

}


/** 
 * Transaction handling thread, as a parameter it receives the transactions
 * slot vector.
 * It wakes up periodically scanning the list of transactions. and displaying
 * their details.
 * @param *data - the transactions slotvector is passed from the main thread.
 */
int tcmi_trans_thread(void *data)
{
	struct tcmi_slotvec *transactions =
		(struct tcmi_slotvec*) data;
	struct tcmi_slot *sl1, *sl2;
	tcmi_slot_node_t *nd1, *nd2;
	struct tcmi_transaction *tran;

	minfo(INFO1, "transaction thread started");
	while (!kthread_should_stop()) {
		tcmi_slotvec_lock(transactions);
		/* search all used slots, find transactions to be handled */
		tcmi_slotvec_for_each_used_slot_safe(sl1, sl2, transactions) {
			minfo(INFO1,"Searching slot %d", tcmi_slot_index(sl1));
			/* iterate through transactions in  the slot. */
			tcmi_slot_for_each_entry_safe(tran, nd1, nd2, sl1, node) {
				handle_tran(tran);
			}
		} /* */
		tcmi_slotvec_unlock(transactions);
		
		mdbg(INFO3, "Transaction thread going to sleep");
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(5*HZ);
		mdbg(INFO3, "Transaction thread woken up");
	}

	tcmi_slotvec_put(transactions);
	return 0;
}

/* response ID's associated with each transactions */
u_int32_t resp_id[] = {
	0x12345,
	0x56789,
	0xabcde
};

/* response ID's associated with each transactions */
static u_int32_t tran_id[3];
/* transaction handling thread reference */
static struct task_struct *thread;
static struct tcmi_transaction *t[3] = {NULL, NULL, NULL};

/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_transaction_test_init(void)
{
	int i;
	struct tcmi_transaction *tran = NULL;
	minfo(INFO1, "Initializing the slot vector hashtable");
	if (!(transactions = tcmi_slotvec_hnew(10))) {
		minfo(ERR1, "Failed to create hashtable");
		goto exit0;
	}

	thread = kthread_run(tcmi_trans_thread, tcmi_slotvec_get(transactions), "tcmi_tran_testd");
	if (IS_ERR(thread)) {
		mdbg(ERR2, "Failed to create a transaction handling thread!");
		goto exit0;
	}
	for (i = 0; i < 3; i++) {
		if (!(t[i] = tcmi_transaction_new(transactions, resp_id[i], 5*HZ))) {
			minfo(ERR1, "Failed to create transaction number %d", i);
			goto exit0;
		}
		minfo(INFO1, "Created new transaction ID = %x",
		      (tran_id[i] = tcmi_transaction_id(t[i])));
		tcmi_transaction_start(t[i], 0);
	}
	/* this will fail in 99.999% cases,  just trying wrong transaction ID */
	if (!(tran = tcmi_transaction_lookup(transactions, 0x111111)))
		minfo(ERR1, "Can't find as expected transaction with ID %x", 0x111111);
	else {
		minfo(ERR1, "Found?????? trans with ID %x", 0x111111);
		tcmi_transaction_put(tran);
	}
	/* find a valid transaction */
	if (!(tran = tcmi_transaction_lookup(transactions, tran_id[0])))
		minfo(ERR1, "Can't find - suprisingly, shouldn't fail, transaction with ID %x", 
		      tran_id[0]);
	/* completing it should also work */
	else {
		tcmi_transaction_complete(tran, &objects[0], resp_id[0]);
		tcmi_transaction_put(tran);
	}
	
	/* find a valid transaction, but try completing it with a wrong response ID */
	if (!(tran = tcmi_transaction_lookup(transactions, tran_id[1])))
		minfo(ERR1, "Can't find - suprisingly, shouldn't fail, transaction with ID %x", 
		      tran_id[1]);
	/* completing will fail - wrong response ID */
	else {
		tcmi_transaction_complete(tran, &objects[1], 0x99); /* response ID doesn't match */
		tcmi_transaction_put(tran);
	}


        /* error handling*/
 exit0:
	return 0;
}

/**
 * Module cleanup
 */
static void __exit tcmi_transaction_test_exit(void)
{
	int i;
	minfo(INFO1, "Destroying the transactions vector");
	/* stop the transaction thread */
	kthread_stop(thread);
	for (i = 0; i < 3; i++) 
		tcmi_transaction_put(t[i]);
	tcmi_slotvec_put(transactions);
}


module_init(tcmi_transaction_test_init);
module_exit(tcmi_transaction_test_exit);



