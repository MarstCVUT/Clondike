/**
 * @file tcmi_penmigman_test.c - Basic test case for the TCMI CCN migration manager
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
#define APP_NAME PENMIGMAN

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/errno.h>

#include <tcmi/manager/tcmi_penmigman.h>
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
static int __init tcmi_penmigman_test_init(void)
{
	int err = 0;
	struct tcmi_ctlfs_entry *root = tcmi_ctlfs_get_root();

	struct kkc_sock *sock = NULL;

	if (kkc_connect(&sock, "tcp:192.168.0.3:54321")) {
		minfo(ERR1, "Failed connecting on TCP, shouldn't fail.. ");
		goto exit0;
	}
	minfo(INFO1, "Connected local: '%s', remote: '%s'",
	      kkc_sock_getsockname2(sock), kkc_sock_getpeername2(sock));

	/* Accept the first incoming connection */
	if (!(migman = tcmi_penmigman_new(sock, 0xaaaabbbb, 0, root, root, "penmigman"))) {
		minfo(ERR1, "Failed to instantiate the PEN migration manager"); 
		goto exit1;
	}
	if ((tcmi_penmigman_auth_ccn(TCMI_PENMIGMAN(migman), 0, NULL)) < 0) {
		minfo(ERR1, "Failed to authenticate at CCN! %d", err); 
	}
	/* error handling */
 exit1:
	kkc_sock_put(sock);
 exit0:
	return err;
}

/**
 * Module cleanup, stops the message receiving
 * thread, clenas transactions and cleans all messages
 * in the queue.
 */
static void __exit tcmi_penmigman_test_exit(void)
{
	minfo(INFO1, "Destroying TCMI PEN migration manager");
	tcmi_migman_put(migman);
	tcmi_ctlfs_put_root();
}


module_init(tcmi_penmigman_test_init);
module_exit(tcmi_penmigman_test_exit);



