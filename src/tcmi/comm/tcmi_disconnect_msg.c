/**
 * @file tcmi_disconnect_msg.c - TCMI disconnection request
 */
#include <linux/slab.h>

#include "tcmi_transaction.h"

#define TCMI_DISCONNECT_MSG_PRIVATE
#include "tcmi_disconnect_msg.h"


#include <dbg.h>


/** 
 * \<\<public\>\> Message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID that will be used for this message
 * instance.
 * @return a new message or NULL.
 */
struct tcmi_msg* tcmi_disconnect_msg_new_rx(u_int32_t msg_id)
{
	struct tcmi_disconnect_msg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_DISCONNECT_MSG_ID) {
		mdbg(ERR3, "Factory specified message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_DISCONNECT_MSG_ID);
		goto exit0;
	}
	/* Allocate the instance */
	if (!(msg = TCMI_DISCONNECT_MSG(kmalloc(sizeof(struct tcmi_disconnect_msg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate disconnect request message");
		goto exit0;
	}
	
	/* Initialized the message for receiving. */
	if (tcmi_msg_init_rx(TCMI_MSG(msg), TCMI_DISCONNECT_MSG_ID, &disconnect_msg_ops)) {
		mdbg(ERR3, "Error initializing disconnect request message %x", msg_id);
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
 * \<\<public\>\> Disconnect message tx constructor.
 *
 *
 * @param *transactions - storage for the new transaction
 * @return a new message ready for the transfer or NULL.
 */
struct tcmi_msg* tcmi_disconnect_msg_new_tx(void)
{
	struct tcmi_disconnect_msg *msg;

	if (!(msg = TCMI_DISCONNECT_MSG(kmalloc(sizeof(struct tcmi_disconnect_msg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate disconnect request message");
		goto exit0;
	}
	
	/* Initialize the message for transfer */
	if (tcmi_msg_init_tx(TCMI_MSG(msg), TCMI_DISCONNECT_MSG_ID, &disconnect_msg_ops, 
			     NULL, 0,
			     0, TCMI_TRANSACTION_INVAL_ID)) {
		mdbg(ERR3, "Error initializing disconnect request message message");
		goto exit1;
	}
	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}


/** @addtogroup tcmi_disconnect_msg_class
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
static int tcmi_disconnect_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock)
{
	mdbg(INFO2, "Disconnect message received");

	return 0;
}

/**
 * \<\<private\>\> Sends the message via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for sending message data
 * @return 0 when successfully sent.
 */
static int tcmi_disconnect_msg_send(struct tcmi_msg *self, struct kkc_sock *sock)
{
	mdbg(INFO2, "Disconnect request message sent.");

	return 0;
}

/** Message operations that support polymorphism. */
static struct tcmi_msg_ops disconnect_msg_ops = {
	.recv = tcmi_disconnect_msg_recv,
	.send = tcmi_disconnect_msg_send,
	.free = NULL,
};


/**
 * @}
 */

