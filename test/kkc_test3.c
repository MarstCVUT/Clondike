/**
 * @file kkc_test2.c - Basic test case 3 for KKC library. Non-blocking version.
 *                      - initializes KKC
 *                      - sets up a separate thread, listening on "tcp:192.168.0.3:5678"
 *                      - the thread accepts a connection and terminates the listening
 *                        Then it will sit in the loop waiting for any incoming data.
 *                        Every 10 received bytes, it echoes back what it has received
 *                      - upon module unload, the thread is sent SIGTERM - the thread
 *                      blocks all signals by default, this signal is forced(force_sig()).
 *                      The thread terminates and the module can be unloaded.
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



static struct kkc_sock *sock2 = NULL;

static int thread_finished = 0;

/** 
 * Listening thread, starts listening and waits for the first incoming
 * connection. Accepts it and stops listening and disconnects the
 * connection too.
 */
struct kkc_sock *new_sock = NULL;
int listen_thread(void *data)
{
	int err = 0;
	int result;
	/* kkc library instance */
	struct kkc_struct *k = (struct kkc_struct*) data;
	struct kkc_sock *sock1; /*, *new_sock;*/
	/* buffer to test receiving data */
	char recv_data[10];
	/* waitqueue used when waiting on data from the TCP connection */
	DECLARE_WAITQUEUE(wait, current);

	minfo(INFO1, "KKC listen thread runing");
	if (!k) {
		minfo(ERR1, "Can't access KKC library");
		err = -EINVAL;
		goto exit0;
	}
	if ((err = k->listen(&sock1, "tcp:192.168.0.3:54321"))) {
		minfo(ERR1, "Failed listening on TCP, shouldn't fail.. sock=%p", sock1);
		goto exit1;
	}
	/* Accept the first incoming connection */
	if ((err = k->accept(sock1, &new_sock, KKC_SOCK_BLOCK)) < 0) {
		minfo(ERR1, "Accept terminated by %s", 
		      (err == -EAGAIN ? "EAGAIN" : "other"));
		goto exit2;
	}
	minfo(INFO1, "Accepted incoming connection, closing everything..local: '%s', remote: '%s'", 
	      kkc_sock_getsockname2(new_sock), kkc_sock_getpeername2(new_sock));
	/* destroy the listening socket*/
	kkc_sock_put(sock1);	

/*	k->add_wait_queue(new_sock, &wait);*/
	/* wait until the thread is signaled to quit, checking for incoming data */
	while (!kthread_should_stop()) {
		if ((result = k->recv(new_sock, recv_data, 10, KKC_SOCK_BLOCK)) > 0) {
			recv_data[sizeof(recv_data) - 1] = 0;
			minfo(INFO1, "Received result: %d, '%s', '%s'", result, recv_data+7, recv_data); 
			/* send some random echo back */
			result = k->send(new_sock, "Ahoj", 4, KKC_SOCK_BLOCK);
			minfo(INFO1, "Send result: %d", result);
		}
/*		set_current_state(TASK_INTERRUPTIBLE);
		schedule(); */
	}
	minfo(INFO3, "Listening thread terminating");
/*	k->remove_wait_queue(new_sock, &wait);*/
	kkc_sock_put(new_sock);	
	/* release the KKC library instance */
	kkc_put();
	return 0;

	/* error handling */
 exit2:
	kkc_sock_put(sock1);	
 exit1:
	kkc_put();
 exit0:
	while (!kthread_should_stop());
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
	k = kkc_get();
	if (!k) {
		minfo(ERR1, "Can't access KKC library");
		goto exit0;
	}
	thread = kthread_run(listen_thread, kkc_get(), "kkc_listend");
	if (IS_ERR(thread)) {
		mdbg(ERR2, "Failed to create a transaction handling thread!");
		goto exit0;
	}

	if (k->connect(&sock2, "tcp:192.168.0.1:9"))
		minfo(ERR1, "Failed connecting to TCP, shouldn't fail..sock = %p", sock2);
	else
		minfo(INFO1, "Connected, socket details: '%s', remote: '%s'", 
		      kkc_sock_getsockname2(sock2), kkc_sock_getpeername2(sock2));

 exit0:
	return 0;
}

/**
 * module cleanup
 */
static void __exit kkc_test2_exit(void)
{
	minfo(INFO1, "Finishing KKC test1");
	/* Signal the thread to terminate if it hasn't done so yet.*/
	force_sig(SIGTERM, thread);
	kthread_stop(thread);

	kkc_sock_put(sock2);
	kkc_put();
}




module_init(kkc_test2_init);
module_exit(kkc_test2_exit);



