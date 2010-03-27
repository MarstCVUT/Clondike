/**
 * @file tcmi_authenticate_msg.c - TCMI authentication request
 */
#include <linux/slab.h>

#include "tcmi_transaction.h"

#include "tcmi_authenticate_resp_msg.h"
#define TCMI_AUTHENTICATE_MSG_PRIVATE
#include "tcmi_authenticate_msg.h"


#include <dbg.h>



/** 
 * \<\<public\>\> Message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID that will be used for this message
 * instance.
 * @return a new message or NULL.
 */
struct tcmi_msg* tcmi_authenticate_msg_new_rx(u_int32_t msg_id)
{
	struct tcmi_authenticate_msg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_AUTHENTICATE_MSG_ID) {
		mdbg(ERR3, "Factory specified message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_AUTHENTICATE_MSG_ID);
		goto exit0;
	}
	/* Allocate the instance */
	if (!(msg = TCMI_AUTHENTICATE_MSG(kmalloc(sizeof(struct tcmi_authenticate_msg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate auth request message");
		goto exit0;
	}
	
	/* Initialized the message for receiving. */
	if (tcmi_msg_init_rx(TCMI_MSG(msg), TCMI_AUTHENTICATE_MSG_ID, &authenticate_msg_ops)) {
		mdbg(ERR3, "Error initializing authenticate request message %x", msg_id);
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
 * \<\<public\>\> Auth message tx constructor.
 *
 *
 * @param *transactions - storage for the new transaction
 * @param pen_id Id of the requesting PEN
 * @param pen_arch Architecture of PEN
 * @return a new message ready for the transfer or NULL.
 */
struct tcmi_msg* tcmi_authenticate_msg_new_tx(struct tcmi_slotvec *transactions, u_int32_t pen_id, enum arch_ids pen_arch, char* auth_data, int size)
{
	struct tcmi_authenticate_msg *msg;

	if (!(msg = TCMI_AUTHENTICATE_MSG(kmalloc(sizeof(struct tcmi_authenticate_msg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate authenticate request message");
		goto exit0;
	}
	
	msg->pen_id = pen_id;
	msg->pen_arch = pen_arch;	
	msg->size = size;

	if ( size > 0 ) {
		if (!(msg->auth_data = (char*)kmalloc(size, GFP_KERNEL))) {
			mdbg(ERR3, "Can't allocate memory for checkpoint name");
			goto exit1;
		}
		memcpy(msg->auth_data, auth_data, size);
	} else {
		msg->auth_data = NULL;
	}

	/* Initialize the message for transfer */
	if (tcmi_msg_init_tx(TCMI_MSG(msg), TCMI_AUTHENTICATE_MSG_ID, &authenticate_msg_ops, 
			     transactions, TCMI_AUTHENTICATE_RESP_MSG_ID,
			     TCMI_DEFAULT_MSG_TIMEOUT, TCMI_TRANSACTION_INVAL_ID)) {
		mdbg(ERR3, "Error initializing authenticate request message message");
		goto exit2;
	}
	return TCMI_MSG(msg);

	/* error handling */
 exit2:
	kfree(msg->auth_data);
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}


/** @addtogroup tcmi_authenticate_msg_class
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
static int tcmi_authenticate_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock)
{
	int err = -EINVAL;
	struct tcmi_authenticate_msg *self_msg = TCMI_AUTHENTICATE_MSG(self);

	mdbg(INFO2, "Authenticate request message received");
	
	if ((err = kkc_sock_recv(sock, &self_msg->pen_id, 
				 sizeof(self_msg->pen_id), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive pen id");
		goto exit0;
	}

	if ((err = kkc_sock_recv(sock, &self_msg->pen_arch, 
				 sizeof(self_msg->pen_arch), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive pen arch");
		goto exit0;
	}

	if ((err = kkc_sock_recv(sock, &self_msg->size, 
				 sizeof(self_msg->size), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive size of auth data");
		goto exit0;
	}

	if ( self_msg->size > 0 ) {
		if (!(self_msg->auth_data = (char*)kmalloc(self_msg->size, GFP_KERNEL))) {
			mdbg(ERR3, "Can't allocate memory for auth data.");
			goto exit0;
		}
	
		/* Receive the authentication data */	
		if ((err = kkc_sock_recv(sock, self_msg->auth_data, 
					self_msg->size, KKC_SOCK_BLOCK)) < 0) {
			mdbg(ERR3, "Failed to receive checkpoint name");
			goto exit0;
		}	
	} else {
		self_msg->auth_data = NULL;
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
static int tcmi_authenticate_msg_send(struct tcmi_msg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_authenticate_msg *self_msg = TCMI_AUTHENTICATE_MSG(self);

	/* Receive the remote PID and checkpoint name size */	
	if ((err = kkc_sock_send(sock, &self_msg->pen_id, 
				 sizeof(self_msg->pen_id), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send pen id");
		goto exit0;
	}

	if ((err = kkc_sock_send(sock, &self_msg->pen_arch, 
				 sizeof(self_msg->pen_arch), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send pen arch");
		goto exit0;
	}

	if ((err = kkc_sock_send(sock, &self_msg->size, 
				 sizeof(self_msg->size), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send auth data size");
		goto exit0;
	}

	if ( self_msg->size > 0 ) {
		/* Send the authentication data */	
		if ((err = kkc_sock_send(sock, self_msg->auth_data, 
					self_msg->size, KKC_SOCK_BLOCK)) < 0) {
			mdbg(ERR3, "Failed to send authentication data");
			goto exit0;
		}
	}

	mdbg(INFO2, "Authenticate request message sent. Pen ID=%d Arch=%d Auth data length=%d", self_msg->pen_id, self_msg->pen_arch, self_msg->size);

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
static void tcmi_authenticate_msg_send_free(struct tcmi_msg *self)
{
	struct tcmi_authenticate_msg *self_msg = TCMI_AUTHENTICATE_MSG(self);	

	mdbg(INFO3, "Freeing auth message");
	kfree(self_msg->auth_data);
}


/** Message operations that support polymorphism. */
static struct tcmi_msg_ops authenticate_msg_ops = {
	.recv = tcmi_authenticate_msg_recv,
	.send = tcmi_authenticate_msg_send,
	.free = tcmi_authenticate_msg_send_free
};


/**
 * @}
 */

