/**
 * @file tcmi_ccnmigman_test.c - Basic test case for the TCMI CCN migration manager
 *                      - creates a listening on 'tcp:192.168.03.:54321'
 *                      - instantiates the manager
 *                      - migration manager is disposed upon module unloading
 *
 *
 * Date: 04/19/2005
 *
 * Author: Jan Capek
 *
 *
 * License....
 */
#ifdef APP_NAME
#undef APP_NAME
#endif
#define APP_NAME CCNMIGMAN

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/errno.h>

#include <tcmi/manager/tcmi_ccnmigman.h>
#include <tcmi/ctlfs/tcmi_ctlfs.h>

#include <dbg.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

static struct tcmi_migman *migman = NULL;
/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_ccnmigman_test_init(void)
{
	int err = 0;
	struct tcmi_ctlfs_entry *root = tcmi_ctlfs_get_root();

	struct kkc_sock *sock1 = NULL, *new_sock = NULL;

	if ((err = kkc_listen(&sock1, "tcp:192.168.0.3:54321"))) {
		minfo(ERR1, "Failed listening on TCP, shouldn't fail.. sock=%p", sock1);
		goto exit0;
	}
	/* Accept the first incoming connection */
	if ((err = kkc_sock_accept(sock1, &new_sock, KKC_SOCK_BLOCK)) < 0) {
		minfo(ERR1, "Accept terminated with error code: %d",err); 
		goto exit1;
	}
	minfo(INFO1, "Accepted incoming connection, terminating listening on: '%s'",
	      kkc_sock_getsockname2(sock1));

	/* Accept the first incoming connection */
	if (!(migman = tcmi_ccnmigman_new(new_sock, 0xdeadbeef, 0, root, root, "ccnmigman"))) {
		minfo(ERR1, "Failed to instantiate the CCN migration manager"); 
		goto exit2;
	}
	if ((tcmi_ccnmigman_auth_pen(TCMI_CCNMIGMAN(migman))) < 0) {
		minfo(ERR1, "Failed to authenticate the PEN! %d", err); 
	}
	/* error handling */
 exit2:
	kkc_sock_put(new_sock);
 exit1:
	kkc_sock_put(sock1);
 exit0:
	return err;
}

/**
 * Module cleanup, stops the message receiving
 * thread, clenas transactions and cleans all messages
 * in the queue.
 */
static void __exit tcmi_ccnmigman_test_exit(void)
{
	minfo(INFO1, "Destroying TCMI CCN migration manager");
	tcmi_migman_put(migman);
	tcmi_ctlfs_put_root();
}


module_init(tcmi_ccnmigman_test_init);
module_exit(tcmi_ccnmigman_test_exit);



