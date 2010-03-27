/**
 * @file kkc_send1M.c - Basic test case 4 for KKC library.
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
static int __init kkc_send1M_init(void)
{
	int err = 0;
	struct kkc_sock *sock1 = NULL;
	void *send_data;
	minfo(INFO1, "Starting KKC 1M receive test");

	if (!(send_data = (void *)vmalloc(DATA_SIZE))) {
		minfo(ERR1, "Error allocating %uKB of data", DATA_SIZE >> 10);
		goto exit0;
		
	}

	if (kkc_connect(&sock1, "tcp:192.168.0.3:5678")) {
		minfo(ERR1, "Failed connecting on TCP, shouldn't fail.. sock=%p", sock);
		goto exit1;
	}
	
	if ((err = kkc_sock_send(sock1, send_data, DATA_SIZE, KKC_SOCK_BLOCK)) < 0) {
		minfo(INFO1, "Signal forced, error code: %d", err);
		goto exit2;
	}
	minfo(INFO1, "Sent %u bytes of data", err);

	/* error handling */
 exit2:
	kkc_sock_put(sock1);	
 exit1:
	vfree(send_data);
 exit0:
	return err;
}

/**
 * module cleanup
 */
static void __exit kkc_send1M_exit(void)
{
	minfo(INFO1, "Finishing KKC recv1M");
}




module_init(kkc_send1M_init);
module_exit(kkc_send1M_exit);



