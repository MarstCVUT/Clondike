/**
 * @file tcmi_msg_client.c - Basic test case for the TCMI communication message framework.
 *                      This is the client part, the module upon loading, spawns a message
 *                      receiving thread, that will receive and deliver messages to the
 *                      main thread.
 *                      The main thread is able to handle only 1 type of message with
 *                      id TCMI_SKEL_MSG_ID
 * 
 *
 *
 * Date: 04/15/2005
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
#include <linux/wait.h>
#include <linux/errno.h>

#include <tcmi/lib/tcmi_queue.h>
#include <tcmi/lib/tcmi_slotvec.h>
#include <tcmi/comm/tcmi_transaction.h>
#include <tcmi/comm/tcmi_msg_groups.h>
#include <tcmi/comm/tcmi_msg_factory.h>
#include <kkc/kkc.h>

#include <dbg.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

static void handle_tran(struct tcmi_transaction *tran) 
{
	/* to prevent deleting an already deleted transaction */
	int deleted = 0;
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
		break;
	case TCMI_TRANSACTION_DELETED:
		minfo(INFO1, "Transaction (%x) is in DELETED state", 
		      tcmi_transaction_id(tran));
		deleted = 1;
		break;
	}
	if (!deleted)
		tcmi_transaction_delete(tran);
}

static void clean_transactions(struct tcmi_slotvec *transactions)
{
	struct tcmi_slot *sl1, *sl2;
	tcmi_slot_node_t *nd1, *nd2;
	struct tcmi_transaction *tran;
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

}
/* transactions and message queue are shared among the main and receiving thread */
static struct tcmi_slotvec *transactions;
static struct tcmi_queue queue;

/** 
 * Messages receiving thread.
 * - sets up listening on tcp:192.168.0.3:54321
 * - accepts one connection 
 * - receives any messages as long as no invalid ID or other error occurs
 * or the main thread signals to quit
 * - delivers each message to the main thread.
 */
int recv_thread(void *data)
{
	struct tcmi_msg *m;
	u_int32_t msg_id;
	int result;
	int err = 0;
	struct kkc_sock *sock1 = NULL, *new_sock = NULL;
	minfo(INFO1, "Messages receiving thread started");

	if ((err = kkc_listen(&sock1, "tcp:192.168.0.3:54321"))) {
		minfo(ERR1, "Failed listening on TCP, shouldn't fail.. sock=%p", sock1);
		goto exit0;
	}
	/* Accept the first incoming connection */
	if ((err = kkc_sock_accept(sock1, &new_sock, KKC_SOCK_BLOCK)) < 0) {
		minfo(ERR1, "Accept terminated with error code:%d",err); 
		goto exit1;
	}
	minfo(INFO1, "Accepted incoming connection, terminating listening on: '%s'",
	      kkc_sock_getsockname2(sock1));
	/* destroy the listening socket*/
	kkc_sock_put(sock1);	

	while (!kthread_should_stop()) {
		if ((result = kkc_sock_recv(new_sock, &msg_id, sizeof(msg_id), KKC_SOCK_BLOCK)) < 0) {
			minfo(INFO1, "Error receiving message ID, error code: %d, terminating", result);
			break;
		}
		if (!(m = tcmi_msg_factory(msg_id, TCMI_MSG_GROUP_MIG))) {
			minfo(ERR1, "Error building message %x, terminating", msg_id);
			break;
		}
		if ((err = tcmi_msg_recv(m, new_sock)) < 0) {
			minfo(ERR1, "Error receiving message %x, error %d, terminating", msg_id, err);
			break;
		}
		tcmi_msg_deliver(m, &queue, transactions);
	}
	kkc_sock_put(new_sock);	
	/* very important to sync with the the thread that is terminating us */
	while (!kthread_should_stop());
	mdbg(INFO1, "Message receiving thread terminating");
	return 0;

	/* error handling */
 exit1:
	kkc_sock_put(sock1);	
 exit0:
	/* very important to sync with the the thread that is terminating us */
	while (!kthread_should_stop());
	return err;
}


void handle_msg(struct tcmi_msg *m)
{
	if (!m)
		return;
	
	switch(tcmi_msg_id(m)) {
	case TCMI_SKEL_MSG_ID:
		minfo(INFO1, "Skeleton message arrived as expected..");
		break;
	default:
		minfo(INFO1, "Unexpected message ID %x, no handler available.", 
		      tcmi_msg_id(m));
		break;
	}
	/* release the message */
	tcmi_msg_put(m);
}

/* message receiving thread reference */
static struct task_struct *thread;

/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_msg_client_init(void)
{
	struct tcmi_msg *m;
	int ret;
	minfo(INFO1, "Initializing the transaction hashtable");
	if (!(transactions = tcmi_slotvec_hnew(10))) {
		minfo(ERR1, "Failed to create hashtable");
		goto exit0;
	}
	minfo(INFO1, "Initializing the message queue");
	tcmi_queue_init(&queue);

	thread = kthread_run(recv_thread, NULL, "tcmi_msg_client_recvd");
	if (IS_ERR(thread)) {
		minfo(ERR2, "Failed to create a message receiving thread!");
		goto exit0;
	}
	
	while (!signal_pending(current)) {
		/* sleep on the message queue, until a message arrives */
		if ((ret = tcmi_queue_wait_on_empty_interruptible(&queue)) < 0) {
			minfo(INFO1, "Signal arrived %d", ret);
		}
		tcmi_queue_remove_entry(&queue, m, node);
		handle_msg(m);
	}


 exit0:
	return 0;
}

/**
 * Module cleanup, stops the message receiving
 * thread, clenas transactions and cleans all messages
 * in the queue.
 */
static void __exit tcmi_msg_client_exit(void)
{
	struct tcmi_msg *m;
	minfo(INFO1, "Message Client terminating");
	/* stop the transaction thread */
	force_sig(SIGTERM, thread);
	kthread_stop(thread);

	/* clean all transactions */
	clean_transactions(transactions);
	tcmi_slotvec_put(transactions);

	/* handle remaining messages in the queue. */
	while(!tcmi_queue_empty(&queue)) {
		tcmi_queue_remove_entry(&queue, m, node);
		handle_msg(m);
	}
}


module_init(tcmi_msg_client_init);
module_exit(tcmi_msg_client_exit);



