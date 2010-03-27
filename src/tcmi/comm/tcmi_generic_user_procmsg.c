/**
 * @file tcmi_generic_user_procmsg.c - Generic user message
 */
#include <linux/slab.h>

#include "tcmi_transaction.h"

#define TCMI_GENERIC_USER_MSG_PRIVATE
#include "tcmi_generic_user_procmsg.h"


#include <dbg.h>



/** 
 * \<\<public\>\> Message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID that will be used for this message
 * instance.
 * @return a new message or NULL.
 */
struct tcmi_msg* tcmi_generic_user_procmsg_new_rx(u_int32_t msg_id)
{
	struct tcmi_generic_user_procmsg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_GENERIC_USER_PROCMSG_ID) {
		mdbg(ERR3, "Factory specified message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_GENERIC_USER_PROCMSG_ID);
		goto exit0;
	}
	/* Allocate the instance */
	if (!(msg = TCMI_GENERIC_USER_PROCMSG(kmalloc(sizeof(struct tcmi_generic_user_procmsg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate generic user message");
		goto exit0;
	}
	
	/* Initialized the message for receiving. */
	if (tcmi_msg_init_rx(TCMI_MSG(msg), TCMI_GENERIC_USER_PROCMSG_ID, &generic_user_procmsg_ops)) {
		mdbg(ERR3, "Error initializing generic user message %x", msg_id);
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
 * \<\<public\>\> Message tx constructor.
 *
 *
 * @param node_id Id of the requesting node
 * @return a new message ready for the transfer or NULL.
 */
struct tcmi_msg* tcmi_generic_user_procmsg_new_tx(u_int32_t node_id, char* user_data, int size)
{
	struct tcmi_generic_user_procmsg *msg;

	if (!(msg = TCMI_GENERIC_USER_PROCMSG(kmalloc(sizeof(struct tcmi_generic_user_procmsg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate generic user message");
		goto exit0;
	}
	
	msg->node_id = node_id;
	msg->size = size;

	if ( size > 0 ) {
		if (!(msg->user_data = (char*)kmalloc(size, GFP_KERNEL))) {
			mdbg(ERR3, "Can't allocate memory for user data message");
			goto exit1;
		}
		memcpy(msg->user_data, user_data, size);
	} else {
		msg->user_data = NULL;
	}

	/* Initialize the message for transfer */
	if (tcmi_msg_init_tx(TCMI_MSG(msg), TCMI_GENERIC_USER_PROCMSG_ID, &generic_user_procmsg_ops, 
			     NULL, 0,
			     0, TCMI_TRANSACTION_INVAL_ID)) {
		mdbg(ERR3, "Error initializing generic user message");
		goto exit2;
	}
	return TCMI_MSG(msg);

	/* error handling */
 exit2:
	kfree(msg->user_data);
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}


/** @addtogroup tcmi_generic_user_msg_class
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
static int tcmi_generic_user_procmsg_recv(struct tcmi_msg *self, struct kkc_sock *sock)
{
	int err = -EINVAL;
	struct tcmi_generic_user_procmsg *self_msg = TCMI_GENERIC_USER_PROCMSG(self);

	mdbg(INFO2, "Generic user message received");
	
	if ((err = kkc_sock_recv(sock, &self_msg->node_id, 
				 sizeof(self_msg->node_id), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive node id");
		goto exit0;
	}

	if ((err = kkc_sock_recv(sock, &self_msg->size, 
				 sizeof(self_msg->size), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive size of user data");
		goto exit0;
	}

	if ( self_msg->size > 0 ) {
		if (!(self_msg->user_data = (char*)kmalloc(self_msg->size, GFP_KERNEL))) {
			mdbg(ERR3, "Can't allocate memory for user data.");
			goto exit0;
		}
	
		/* Receive the user data */	
		if ((err = kkc_sock_recv(sock, self_msg->user_data, 
					self_msg->size, KKC_SOCK_BLOCK)) < 0) {
			mdbg(ERR3, "Failed to receive user data");
			goto exit0;
		}	
	} else {
		self_msg->user_data = NULL;
	}

	return 0;

	/* error handling*/
 exit0:
	return err;
}

/**
 * \<\<private\>\> Sends the message via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for sending message data
 * @return 0 when successfully sent.
 */
static int tcmi_generic_user_procmsg_send(struct tcmi_msg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_generic_user_procmsg *self_msg = TCMI_GENERIC_USER_PROCMSG(self);

	if ((err = kkc_sock_send(sock, &self_msg->node_id, 
				 sizeof(self_msg->node_id), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send node id");
		goto exit0;
	}

	if ((err = kkc_sock_send(sock, &self_msg->size, 
				 sizeof(self_msg->size), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send user data size");
		goto exit0;
	}

	if ( self_msg->size > 0 ) {
		/* Send the user data */	
		if ((err = kkc_sock_send(sock, self_msg->user_data, 
					self_msg->size, KKC_SOCK_BLOCK)) < 0) {
			mdbg(ERR3, "Failed to send user data");
			goto exit0;
		}
	}

	mdbg(INFO2, "User data message sent. Pen ID=%d Auth data length=%d", self_msg->node_id, self_msg->size);

	return 0;
	/* error handling */
 exit0:
	return err;
}

/**
 * \<\<private\>\> Frees custom message resources.
 * The checkpoint image name string is released from memory.
 *
 * @param *self - this message instance
 */
static void tcmi_generic_user_procmsg_send_free(struct tcmi_msg *self)
{
	struct tcmi_generic_user_procmsg *self_msg = TCMI_GENERIC_USER_PROCMSG(self);	

	mdbg(INFO3, "Freeing user message");
	kfree(self_msg->user_data);
}


/** Message operations that support polymorphism. */
static struct tcmi_msg_ops generic_user_procmsg_ops = {
	.recv = tcmi_generic_user_procmsg_recv,
	.send = tcmi_generic_user_procmsg_send,
	.free = tcmi_generic_user_procmsg_send_free
};


/**
 * @}
 */

