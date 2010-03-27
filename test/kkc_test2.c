/**
 * @file kkc_test2.c - Basic test case 2 for KKC library. Blocking version.
 *                      - initializes KKC
 *                      - sets up a separate thread, listening on "tcp:192.168.0.3:5678"
 *                      - the thread accepts a connection and terminates the listening
 *                        Then it will sit in the loop waiting for any incoming data.
 *                        Every 10 received bytes, it echoes back what it has received
 *                      - upon module unload, the thread is sent SIGTERM - the thread
 *                      blocks all signals by default, this signal is forced(force_sig()).
 *                      The thread terminates and the module can be unloaded.
 *
 * Date: 04/14/2005
 *
 * Author: Jan Capek
 *
 *
 * License....
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/sched.h>

#include <kkc/kkc.h>

#include <dbg.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

/** This macro implements atomic check for thread stop request and
 * puts the task to sleep until such request arrives. Note the order
 * of operations, the change of the state must always be prior to
 * checking if it should stop. Any other order is incorrect and might
 * put the task to sleep indefinitely. */
#define wait_on_should_stop()				\
do {							\
	do {						\
minfo(INFO1, "Looping.."); \
		schedule();				\
		set_current_state(TASK_UNINTERRUPTIBLE);	\
	} while(!kthread_should_stop());		\
minfo(INFO1, "Finished looping.."); \
	set_current_state(TASK_RUNNING);		\
} while(0)

/** 
 * Listening thread, starts listening and waits for the first incoming
 * connection. Accepts it and stops listening waits for incoming data.
 */
int listen_thread(void *data)
{
	int err = 0;
	int result;
	struct kkc_sock *sock1 = NULL, *new_sock = NULL;
	/* buffer to test receiving data */
	char recv_data[10];

	minfo(INFO1, "KKC listen thread runing");

	if ((err = kkc_listen(&sock1, "tcp:192.168.0.3:54321"))) {
		minfo(ERR1, "Failed listening on TCP, shouldn't fail.. sock=%p", sock1);
		goto exit0;
	}
	/* Accept the first incoming connection */
	if ((err = kkc_sock_accept(sock1, &new_sock, KKC_SOCK_BLOCK)) < 0) {
		minfo(ERR1, "Accept terminated by with error code:%d",err); 
		goto exit1;
	}
	minfo(INFO1, "Accepted incoming connection, terminating listening on: '%s'",
	      kkc_sock_getsockname2(new_sock));
	/* destroy the listening socket*/
	kkc_sock_put(sock1);	

	/* wait until the thread is signaled to quit, checking for incoming data */
	while (!kthread_should_stop()) {
		if ((result = kkc_sock_recv(new_sock, recv_data, 10, KKC_SOCK_BLOCK)) < 0) {
			minfo(INFO1, "Signal forced, error code: %d", result);
			break;
		}
		recv_data[sizeof(recv_data) - 1] = 0;
		minfo(INFO1, "Received result: %d, '%s', '%s'", result, recv_data+7, recv_data); 
		/* make some change to the data */
		recv_data[0] = 'X';
		/* send some random echo back */
		result = kkc_sock_send(new_sock, recv_data, sizeof(recv_data), KKC_SOCK_BLOCK);
		minfo(INFO1, "Send result: %d", result);

	}
	kkc_sock_put(new_sock);	
	/* very important to sync with the the thread that is terminating us */
	minfo(INFO3, "Listening thread waiting on should stop");
	wait_on_should_stop();
	minfo(INFO3, "Listening thread terminating");
	return 0;

	/* error handling */
 exit1:
	kkc_sock_put(sock1);	
 exit0:
	/* very important to sync with the the thread that is terminating us */
	minfo(ERR3, "Listening thread waiting on should stop");
	wait_on_should_stop();
	minfo(ERR3, "Listening thread terminating with error %d", err);
	return err;
}

static struct task_struct *thread;

/**
 * Module initialization
 *
 * @return - 
 */
struct kkc_struct *k;
static int __init kkc_test2_init(void)
{


	minfo(INFO1, "Starting KKC test");

	thread = kthread_run(listen_thread, NULL, "kkc_listend");
	if (IS_ERR(thread)) {
		mdbg(ERR2, "Failed to create a listening thread!");
		goto exit0;
	}

 exit0:
	return 0;
}

/**
 * module cleanup
 */
static void __exit kkc_test2_exit(void)
{
	minfo(INFO1, "Finishing KKC test2");
	/* Signal the thread to terminate if it hasn't done so yet.*/
	force_sig(SIGTERM, thread);
	minfo(INFO1, "Exit sleeping");
	schedule_timeout(5*HZ);
	minfo(INFO1, "Exit wokenup");
	kthread_stop(thread);
}




module_init(kkc_test2_init);
module_exit(kkc_test2_exit);



