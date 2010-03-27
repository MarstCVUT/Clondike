/**
 * @file tcmi_authenticate_resp_msg.c - TCMI authenticate response message
 */
#include <linux/slab.h>

#include "tcmi_transaction.h"

#define TCMI_AUTHENTICATE_RESP_MSG_PRIVATE
#include "tcmi_authenticate_resp_msg.h"


#include <dbg.h>



/** 
 * \<\<public\>\> Authenticate response message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID that will be used for this error message
 * instance.
 * @return a new message or NULL.
 */
struct tcmi_msg* tcmi_authenticate_resp_msg_new_rx(u_int32_t msg_id)
{
	struct tcmi_authenticate_resp_msg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_AUTHENTICATE_RESP_MSG_ID) {
		mdbg(ERR3, "Factory specified message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_AUTHENTICATE_RESP_MSG_ID);
		goto exit0;
	}
	if (!(msg = TCMI_AUTHENTICATE_RESP_MSG(kmalloc(sizeof(struct tcmi_authenticate_resp_msg), 
					    GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate auth resp message");
		goto exit0;
	}
	/* Initialized the message for receiving, message ID is extended with error flags */
	if (tcmi_msg_init_rx(TCMI_MSG(msg), TCMI_AUTHENTICATE_RESP_MSG_ID, &authenticate_resp_msg_ops)) {
		mdbg(ERR3, "Error initializing auth resp message %x", msg_id);
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
 * \<\<public\>\> Authenticate response message tx constructor.
 *
 *
 * @param trans_id - transaction ID that this message is replying to.
 * @param ccn_id - ID of this CCN
 * @param ccn_arch - Architecture of CCN
 * @param result_code Result of the authentication
 * @param mount_params Paremeters of the distributed FS mount to be performed on PEN side
 * @return a new test response message for the transfer or NULL.
 */
struct tcmi_msg* tcmi_authenticate_resp_msg_new_tx(u_int32_t trans_id, u_int32_t ccn_id, enum arch_ids ccn_arch, int8_t result_code, struct fs_mount_params* mount_params)
{
	struct tcmi_authenticate_resp_msg *msg;

	if (!(msg = TCMI_AUTHENTICATE_RESP_MSG(kmalloc(sizeof(struct tcmi_authenticate_resp_msg), 
					    GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate auth resp message");
		goto exit0;
	}

	msg->ccn_id = ccn_id;
	msg->ccn_arch = ccn_arch;
	msg->result_code = result_code;
	msg->mount_params = *mount_params;

	if (tcmi_msg_init_tx(TCMI_MSG(msg), TCMI_AUTHENTICATE_RESP_MSG_ID, &authenticate_resp_msg_ops, 
			     NULL, 0, 0, trans_id)) {
		mdbg(ERR3, "Error initializing authenticate response message");
		goto exit1;
	}
	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}


/** @addtogroup tcmi_authenticate_resp_msg_class
 *
 * @{
 */

/**
 * \<\<private\>\> Receives the message via a specified connection. 
 * Receiving the erro message requires reading the error code from the specified
 * connection 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully received.
 */
static int tcmi_authenticate_resp_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_authenticate_resp_msg *self_msg = TCMI_AUTHENTICATE_RESP_MSG(self);
	mdbg(INFO3, "Authenticate response message received");

	if ((err = kkc_sock_recv(sock, &self_msg->ccn_id, 
				 sizeof(self_msg->ccn_id), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive ccn id");
		goto exit0;
	}

	if ((err = kkc_sock_recv(sock, &self_msg->ccn_arch, 
				 sizeof(self_msg->ccn_arch), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive ccn arch");
		goto exit0;
	}

	if ((err = kkc_sock_recv(sock, &self_msg->result_code, 
				 sizeof(self_msg->result_code), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive result_code");
		goto exit0;
	}

	if ((err = kkc_sock_recv(sock, &self_msg->mount_params, 
				 sizeof(self_msg->mount_params), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive mount params");
		goto exit0;
	}
	
	return 0;
exit0:
	return err;
}

/**
 * \<\<private\>\> Receives the message via a specified connection. 
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for sending message data
 * @return 0 when successfully sent.
 */
static int tcmi_authenticate_resp_msg_send(struct tcmi_msg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_authenticate_resp_msg *self_msg = TCMI_AUTHENTICATE_RESP_MSG(self);
	mdbg(INFO3, "Authenticate response message sent");

	if ((err = kkc_sock_send(sock, &self_msg->ccn_id, 
				 sizeof(self_msg->ccn_id), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send ccn id");
		goto exit0;
	}

	if ((err = kkc_sock_send(sock, &self_msg->ccn_arch, 
				 sizeof(self_msg->ccn_arch), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send ccn arch");
		goto exit0;
	}

	if ((err = kkc_sock_send(sock, &self_msg->result_code, 
				 sizeof(self_msg->result_code), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send result code");
		goto exit0;
	}

	if ((err = kkc_sock_send(sock, &self_msg->mount_params, 
				 sizeof(self_msg->mount_params), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send mount_params");
		goto exit0;
	}

	return 0;
exit0:
	return err;
}



/** Message operations that support polymorphism. */
static struct tcmi_msg_ops authenticate_resp_msg_ops = {
	.recv = tcmi_authenticate_resp_msg_recv,
	.send = tcmi_authenticate_resp_msg_send
};


/**
 * @}
 */

