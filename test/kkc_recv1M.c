/**
 * @file kkc_recv1M.c - Basic test case 4 for KKC library.
 *                      - initializes KKC
 *                      - sets up listening on "tcp:192.168.0.3:5678"
 *                      - upon incoming connection, receives 1M of data
 *                      - reports when all data has been received and terminates
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
#include <linux/vmalloc.h>

#include <kkc/kkc.h>

#include <dbg.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");



static struct kkc_sock *sock = NULL;
/**
 * Module initialization
 *
 * @return - 
 */
#define DATA_SIZE (1 << 20)
static int __init kkc_recv1M_init(void)
{
	int err = 0;
	struct kkc_sock *sock1 = NULL, *new_sock = NULL;
	void *recv_data;

	minfo(INFO1, "Starting KKC 1M receive test");

	if (!(recv_data = (void *)vmalloc(DATA_SIZE))) {
		minfo(ERR1, "Error allocating %uKB of data", DATA_SIZE >> 10);
		goto exit0;
		
	}

	if (kkc_listen(&sock1, "tcp:192.168.0.3:5678")) {
		minfo(ERR1, "Failed listening on TCP, shouldn't fail.. sock=%p", sock);
		goto exit1;
	}
	/* Accept the first incoming connection */
	if ((err = kkc_sock_accept(sock1, &new_sock, KKC_SOCK_BLOCK)) < 0) {
		minfo(ERR1, "Accept terminated by with error code:%d",err); 
		goto exit2;
	}
	
	if ((err = kkc_sock_recv(new_sock, recv_data, DATA_SIZE, KKC_SOCK_BLOCK)) < 0) {
		minfo(INFO1, "Signal forced, error code: %d", err);
		goto exit3;
	}
	minfo(INFO1, "Received %u bytes of data", err);

	/* error handling */
 exit3:
	kkc_sock_put(new_sock);	
 exit2:
	kkc_sock_put(sock1);	
 exit1:
	vfree(recv_data);
 exit0:
	return err;
}

/**
 * module cleanup
 */
static void __exit kkc_recv1M_exit(void)
{
	minfo(INFO1, "Finishing KKC recv1M");
}




module_init(kkc_recv1M_init);
module_exit(kkc_recv1M_exit);



