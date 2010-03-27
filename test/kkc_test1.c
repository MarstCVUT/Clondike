/**
 * @file kkc_test1.c - Basic test case for KKC library.
 *                      - initializes KKC
 *                      - tries connecting to "udp:192.168.0.3:5678"
 *                      this will fail as there is no such architecture
 *                      - tries connecting to "tcp:192.168.0.3:5678"
 *                      this will succeed
 *                      - sets a listening on "tcp:192.168.0.3:0" - port chosen
 *                      by the network layer
 *                      - issue a blocking accept - terminate with signal from user 
 *                      space
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
#include <linux/pagemap.h>

#include <kkc/kkc.h>

#include <dbg.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

/*static struct kkc_struct *k;*/


static struct kkc_sock *sock = NULL;
/**
 * Module initialization
 *
 * @return - 
 */
static int __init kkc_test1_init(void)
{
	int err = 0;

	minfo(INFO1, "Starting KKC test");
	struct kkc_sock *new_sock = NULL;

	if (kkc_connect(&sock, "udp:192.168.0.3:5678"))
		minfo(ERR1, "Failed connecting to UDP, as expected, sock=%p", sock);

	if (kkc_connect(&sock, "tcp:192.168.0.3:5678"))
		minfo(ERR1, "Failed connecting to TCP, shouldn't fail..sock = %p", sock);
	else
		kkc_sock_put(sock);

	if (kkc_listen(&sock, "tcp:192.168.0.3:0")) {
		minfo(ERR1, "Failed listening on TCP, shouldn't fail.. sock=%p", sock);
		goto exit0;
	}
	minfo(INFO1, "Socket names-local: '%s', remote: '%s'", 
	      kkc_sock_getsockname2(sock), kkc_sock_getpeername2(sock));

	/* Accept the first incoming connection */
	if ((err = kkc_sock_accept(sock, &new_sock, KKC_SOCK_BLOCK)) < 0) {
		minfo(ERR1, "Accept terminated by with error code:%d",err); 
		goto exit0;
	}
	minfo(INFO1, "Accepted incoming connection, terminating listening on: '%s'",
	      kkc_sock_getsockname2(new_sock));
	/* destroy the listening socket*/
	kkc_sock_put(new_sock);	
 exit0:
	return 0;
}

/**
 * module cleanup
 */
static void __exit kkc_test1_exit(void)
{
	minfo(INFO1, "Finishing KKC test1");
	/* no checking for sock needed, NULL is handled properly */
	kkc_sock_put(sock);
}




module_init(kkc_test1_init);
module_exit(kkc_test1_exit);



