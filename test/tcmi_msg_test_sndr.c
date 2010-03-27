/**
 * @file tcmi_msg_test_sndr.c - Basic test case for the TCMI communication message framework.
 *                      This is the sender part. The module upon loading connects
 *                      to a message receving party at tcp:192.168.0.3:54321.
 *                      Spawns a message receiving and delivering thread.
 *                      The main thread sends a message and waits for a response or timeout.
 *                      Upon a valid message arrival, the main thread processes it. Currently
 *                      it is able to handle only 1 type of message TCMI_SKELRESP_MSG_ID with
 *                      optional error flags.
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
#ifdef APP_NAME
#undef APP_NAME
#endif
#define APP_NAME SNDR

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
#include <tcmi/comm/tcmi_msg_factory.h>
#include <tcmi/comm/tcmi_messages_dsc.h>

#include <kkc/kkc.h>

#include <dbg.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

static void handle_tran(struct tcmi_transaction *tran) 
{
	switch (tcmi_transaction_state(tran)) {
	case TCMI_TRANSACTION_INIT:
		minfo(INFO1, "Transaction (%x) is in INIT state..", 
		      tcmi_transaction_id(tran));
		break;
	case TCMI_TRANSACTION_RUNNING:
		minfo(INFO1, "Transaction (%x) is in RUNNING state..", 
		      tcmi_transaction_id(tran));
		break;
	case TCMI_TRANSACTION_ABORTED:
		minfo(INFO1, "Transaction (%x) is in ABORTED state..", 
		      tcmi_transaction_id(tran));
		break;
	case TCMI_TRANSACTION_COMPLETE:
		minfo(INFO1, "Transaction (%x) is in COMPLETE state..", 
		      tcmi_transaction_id(tran));
		break;
	}
}

static void check_transactions(struct tcmi_slotvec *transactions)
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

#define wait_on_should_stop()	\
do {						\
  schedule();					\
  set_current_state(TASK_INTERRUPTIBLE);	\
} while(!kthread_should_stop())
/** 
 * Messages receiving thread.
 * - receives any messages on the connection passed in as a parameter.
 * - as long as no invalid ID or other error occurs
 * or the main thread signals to quit it will be delivering messages to the
 * main thread.
 */
int msg_test_sndr_recv_thread(void *data)
{
	struct tcmi_msg *m;
	u_int32_t msg_id;
	int result;
	int err = 0;
	struct kkc_sock *sock = KKC_SOCK(data);
	minfo(INFO1, "Messages receiving thread started");

	minfo(INFO1, "Receiving on socket local: '%s', remote: '%s'",
	      kkc_sock_getsockname2(sock), kkc_sock_getpeername2(sock));

	while (!(kthread_should_stop() || signal_pending(current))) {
		if ((result = kkc_sock_recv(sock, &msg_id, sizeof(msg_id), KKC_SOCK_BLOCK)) < 0) {
			minfo(INFO1, "Error receiving message ID, error code: %d, terminating", result);
			break;
		}
		if (!(m = tcmi_msg_factory(msg_id, TCMI_MSG_GROUP_MIG))) {
			minfo(ERR1, "Error building message %x, terminating", msg_id);
			break;
		}
		if ((err = tcmi_msg_recv(m, sock)) < 0) {
			minfo(ERR1, "Error receiving message %x, error %d, terminating", msg_id, err);
			break;
		}
		/* try to deliver if failed, destroy the message */
		if (tcmi_msg_deliver(m, &queue, transactions) < 0)
			tcmi_msg_put(m);
	}
	kkc_sock_put(sock);	
	/* very important to sync with the the thread that is terminating us */
	wait_on_should_stop();
	mdbg(INFO1, "Message receiving thread terminating");
	return 0;
}


void sndr_handle_msg(struct tcmi_msg *m)
{
	if (!m)
		return;
	
	switch(tcmi_msg_id(m)) {
	case TCMI_SKELRESP_MSG_ID:
		minfo(INFO1, "Skeleton response arrived as expected..");
		break;
	case TCMI_MSG_FLG_SET_ERR(TCMI_SKELRESP_MSG_ID):
		minfo(INFO1, "Skeleton error response arrived as expected..");
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
static struct task_struct *thread = NULL;

/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_msg_test_sndr_init(void)
{
	struct tcmi_msg *m, *response;
	struct kkc_sock *sock;
	minfo(INFO1, "Initializing the transaction hashtable");
	if (!(transactions = tcmi_slotvec_hnew(10))) {
		minfo(ERR1, "Failed to create hashtable");
		goto exit0;
	}
	minfo(INFO1, "Initializing the message queue");
	tcmi_queue_init(&queue);

	if (kkc_connect(&sock, "tcp:192.168.0.3:54321")) {
		minfo(ERR1, "Failed connecting on TCP, shouldn't fail.. ");
		goto exit0;
	}
	thread = kthread_run(msg_test_sndr_recv_thread, kkc_sock_get(sock), 
			     "tcmi_msg_test_sndr_recvd");
	if (IS_ERR(thread)) {
		minfo(ERR2, "Failed to create a message receiving thread!");
		goto exit1;
	}

/*	if (!(m = tcmi_skelresp_msg_new_tx(TCMI_TRANSACTION_INVAL_ID))) {*/
	if (!(m = tcmi_skel_msg_new_tx(transactions))) {
		minfo(ERR1, "Error creating a new test message");
		goto exit1;
	}
	/* Skeleton message expects a response, so perform send and receive */
/*	if (tcmi_msg_send_anonymous(m, sock) < 0) {
		minfo(ERR1, "Failed to send message!!");
		goto exit2;
	} 
*/
	/* sleep on the message queue, until a message arrives */
/*	if ((ret = tcmi_queue_wait_on_empty_interruptible(&queue)) < 0) {
		minfo(INFO1, "Signal arrived %d", ret);
	}
	else {
		tcmi_queue_remove_entry(&queue, response, node);
		if (response) {
			minfo(INFO1, "Anonymous message is now to be handled");
			sndr_handle_msg(response);
		}
		else {
			minfo(ERR1, "Woken up, but the queue is still empty - nothing arrived");
		}
	}
*/
	/** wait for a transaction to complete */
	if (tcmi_msg_send_and_receive(m, sock, &response) < 0) {
		minfo(ERR1, "Failed to send message!!");
		goto exit2;
	}
	if (response) {
		minfo(INFO1, "Transaction Response is now to be handled");
		sndr_handle_msg(response);
	}
	else {
		minfo(ERR1, "Transaction Response - nothing arrived");
	}
	/* release the message */
	tcmi_msg_put(m);
	/* drop the socket */
	kkc_sock_put(sock);
	return 0;

	/* error handling */
 exit2:
	tcmi_msg_put(m);
 exit1:
	kkc_sock_put(sock);
 exit0:
	return 0;
}

/**
 * Module cleanup, stops the message receiving
 * thread, cleans transactions and cleans all messages
 * in the queue.
 */
static void __exit tcmi_msg_test_sndr_exit(void)
{
	struct tcmi_msg *m;
	minfo(INFO1, "Message Sender terminating");
	if (thread) {
		/* stop the transaction thread */
		force_sig(SIGTERM, thread);
		kthread_stop(thread);
	}
	if (transactions) {
		/* check transactions - see if there are any stale */
		check_transactions(transactions);
	}

	/* handle remaining messages in the queue. */
	while(!tcmi_queue_empty(&queue)) {
		tcmi_queue_remove_entry(&queue, m, node);
		sndr_handle_msg(m);
	}
	minfo(INFO1, "Last check for stale transactions");
	if (transactions) {
		/* check transactions - see if there are any stale */
		check_transactions(transactions);
		tcmi_slotvec_put(transactions);
	}
}


module_init(tcmi_msg_test_sndr_init);
module_exit(tcmi_msg_test_sndr_exit);



