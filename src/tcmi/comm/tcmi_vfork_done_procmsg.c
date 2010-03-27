/**
 * @file tcmi_vfork_done_procmsg.c - TCMI vfork completition notification
 *       
 */
#include <linux/slab.h>

#include "tcmi_transaction.h"

#define TCMI_VFORK_DONE_PROCMSG_PRIVATE
#include "tcmi_vfork_done_procmsg.h"


#include <dbg.h>



/** 
 * \<\public\>\> vfork_done message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID - used for verification
 * instance.
 * @return a new message or NULL.
 */
struct tcmi_msg* tcmi_vfork_done_procmsg_new_rx(u_int32_t msg_id)
{
	struct tcmi_vfork_done_procmsg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_VFORK_DONE_PROCMSG_ID) {
		mdbg(ERR3, "Factory specified message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_VFORK_DONE_PROCMSG_ID);
		goto exit0;
	}
	/* Allocate the instance */
	if (!(msg = TCMI_VFORK_DONE_PROCMSG(kmalloc(sizeof(struct tcmi_vfork_done_procmsg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate vfork_done request message");
		goto exit0;
	}
	/* Initialized the message for receiving. */
	if (tcmi_procmsg_init_rx(TCMI_PROCMSG(msg), TCMI_VFORK_DONE_PROCMSG_ID, &vfork_done_procmsg_ops)) {
		mdbg(ERR3, "Error initializing test request message %x", msg_id);
		goto exit1;
	}

	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
}

/** 
 * \<\public\>\> vfork_done message tx constructor.
 *
 * The vfork_done notification has no transaction associated.
 *
 *
 * @param dst_pid - destination process PID
 * @return a new message ready for the transfer or NULL.
 */
struct tcmi_msg* tcmi_vfork_done_procmsg_new_tx(pid_t dst_pid)
{
	struct tcmi_vfork_done_procmsg *msg;

	if (!(msg = TCMI_VFORK_DONE_PROCMSG(kmalloc(sizeof(struct tcmi_vfork_done_procmsg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate vfork_done notification message");
		goto exit0;
	}

	/* Initialize the message for transfer, no transaction to be
	 * created, no response expected, no timeout for response, not a reply to any transaction */
	if (tcmi_procmsg_init_tx(TCMI_PROCMSG(msg), TCMI_VFORK_DONE_PROCMSG_ID, &vfork_done_procmsg_ops, 
				 dst_pid, 0,
				 NULL, 0, 
				 0, TCMI_TRANSACTION_INVAL_ID)) {
		mdbg(ERR3, "Error initializing vfork_done notification message message");
		goto exit1;
	}
	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}


/** @addtogroup tcmi_vfork_done_procmsg_class
 *
 * @{
 */

/**
 * \<\<private\>\> Receives the message via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully received.
 */
static int tcmi_vfork_done_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	return 0;
}

/**
 * \<\<private\>\> Sends the vfork_done notification via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully sent.
 */
static int tcmi_vfork_done_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	return 0;
}



/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops vfork_done_procmsg_ops = {
	.recv = tcmi_vfork_done_procmsg_recv,
	.send = tcmi_vfork_done_procmsg_send
};


/**
 * @}
 */

