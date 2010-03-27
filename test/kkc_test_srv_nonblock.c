/**
 * @file kkc_test_srv_nonblock.c - Basic test case 3 for KKC library. Non-blocking version.
 *                              - initializes KKC
 *                              - sets up a separate thread, listening on "tcp:192.168.0.3:5678"
 *                              - the thread accepts a connection and terminates the listening
 *                              Then it will sit in the loop waiting for any incoming data.
 *                              Every 10 received bytes, it echoes back what it has received
 *                              - upon module unload, the thread is sent SIGTERM - the thread
 *                              blocks all signals by default, this signal is forced(force_sig()).
 *                              The thread terminates and the module can be unloaded.
 *
 *
 * Date: 03/28/2005
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



/** 
 * Listening thread, this code should be split up in to separate
 * functions handling individual steps. For demostration
 * purposes it is kept all in one piece.  It performs following steps:
 * - starts listening on 'tcp:192.168.0.3:54321'
 * - use non blocking mode to check for incoming connections - see details on
 * why schedule_timeout is used, so that an incoming connections doesn't wait indefinitely.
 * - once the incoming connections is accepted, terminates the listening socket
 * - receives data into a 10byte buffer in non-blocking mode, the approach is
 * the same is an case of accept using schedule timeout. 
 * - keeps receiving and echoing data, until the module doesn't get unloaded 
 * - the thread always waits on proper termination from outsided (kthread_should_stop() check)
 * - returns the KKC library instance 
 */
int listen_thread(void *data)
{
	int err = 0;
	int result;
	/* kkc library instance */
/*	struct kkc_struct *k = (struct kkc_struct*) data; */
	struct kkc_sock *sock1 = NULL, *new_sock = NULL;
	struct kkc_sock_sleeper *sleeper1;
	/* buffer to test receiving data */
	char recv_data[10];

	minfo(INFO1, "KKC listen thread runing");
	if ((err = kkc_listen(&sock1, "tcp:192.168.0.3:54321"))) {
		minfo(ERR1, "Failed listening on TCP, shouldn't fail.. sock=%p", sock1);
		goto exit0;
	}
	sleeper1 = kkc_sock_sleeper_add(sock1, current);
	/* This is the only correct way, how to watch the socket and
	 * MUST be used when watching one or more sockets at the same
	 * time. If the schedule_timeout() wouldn't be used, there
	 * might be incoming connections waiting to be accepted
	 * indefinitely!! This example takes only first incoming
	 * connection and proceeds further. */
	while ((err = kkc_sock_accept(sock1, &new_sock, KKC_SOCK_NONBLOCK)) < 0) {
		if (err == -EAGAIN) {
			minfo(INFO1, "Accept would block, going to sleep to check in 5sec again..");
			schedule();
			/* schedule_timeout(5*HZ); */
			set_current_state(TASK_INTERRUPTIBLE);
		}

		if (signal_pending(current)) {
			minfo(INFO1, "Woken up by signal error code %d, sigpending=%d, terminating..", 
			      err, signal_pending(current));
			goto exit1;
		}
	}

	kkc_sock_sleeper_remove(sleeper1);
	minfo(INFO1, "Accepted incoming connection, closing everything..local: '%s', remote: '%s'", 
	      kkc_sock_getsockname2(new_sock), kkc_sock_getpeername2(new_sock));
	/* destroy the listening socket*/
	kkc_sock_put(sock1);	
	
	/* Now handle the incoming connection */
	sleeper1 = kkc_sock_sleeper_add(new_sock, current);
	/* wait until the thread is signalled to quit, checking for incoming data */
	set_current_state(TASK_INTERRUPTIBLE);
	while (!(kthread_should_stop() || signal_pending(current))) {
		if ((err = kkc_sock_recv(new_sock, recv_data, sizeof(recv_data), KKC_SOCK_NONBLOCK)) < 0) {

			if (err == -EAGAIN) {
				minfo(INFO1, "Receive would block, going to sleep to check in 5sec again..");
				/* Same case as for accept, we check again for the same reasons */
				schedule();
				set_current_state(TASK_INTERRUPTIBLE);
			}
			if (signal_pending(current)) {
				minfo(INFO1, "Woken up by signal error code %d, sigpending=%d, terminating..", 
				      err, signal_pending(current));
			}
			continue; /* next round to check */
		}
		recv_data[err - 1] = 0;
		minfo(INFO1, "Received result: %d bytes, '%s'", err,  recv_data); 
		/* send some random echo back */
		result = kkc_sock_send(new_sock, "Ahoj", 4, KKC_SOCK_BLOCK);
		minfo(INFO1, "Send result: %d", result);
	}

	kkc_sock_sleeper_remove(sleeper1);
	kkc_sock_put(new_sock);	
	/* very important to sync with the the thread that is terminating us */
	minfo(INFO1, "Listening thread terminating, waiting for shutdown");	
	while (!kthread_should_stop());

	minfo(INFO1, "Listening thread terminating");	
	return 0;

	/* error handling */
 exit1:
	kkc_sock_sleeper_remove(sleeper1);
	kkc_sock_put(sock1);	

 exit0:
	/* very important to sync with the the thread that is terminating us */
	while (!kthread_should_stop());
	return err;
}

static struct task_struct *thread;

/**
 * Module initialization
 *
 * @return - 
 */
static int __init kkc_test_srv_nonblock_init(void)
{


	minfo(INFO1, "Starting KKC test");
	thread = kthread_run(listen_thread, NULL, "kkc_listend");
	if (IS_ERR(thread)) {
		mdbg(ERR2, "Failed to create a transaction handling thread!");
		goto exit0;
	}

 exit0:
	return 0;
}

/**
 * module cleanup
 */
static void __exit kkc_test_srv_nonblock_exit(void)
{
	minfo(INFO1, "Finishing KKC test1");
	/* Signal the thread to terminate if it hasn't done so yet.*/
	force_sig(SIGTERM, thread);
	kthread_stop(thread);

}




module_init(kkc_test_srv_nonblock_init);
module_exit(kkc_test_srv_nonblock_exit);



