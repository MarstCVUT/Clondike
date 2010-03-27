/**
 * @file tcmi_ppm_p_migr_back_shadowreq_procmsg.c - migrate back request initiated by shadow
 */       

#include <linux/slab.h>
#include <linux/string.h>

#include "tcmi_transaction.h"

#include "tcmi_skelresp_msg.h"
#define TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_PRIVATE
#include "tcmi_ppm_p_migr_back_shadowreq_procmsg.h"

#include <dbg.h>


/** 
 * \<\<public\>\> PPM_P message rx constructor.
 * This method is called by the factory class.
 *
 * @param msg_id - message ID that will be used for this error message
 * instance.
 * @return a new error message or NULL.
 */
struct tcmi_msg* tcmi_ppm_p_migr_back_shadowreq_procmsg_new_rx(u_int32_t msg_id)
{
	struct tcmi_ppm_p_migr_back_shadowreq_procmsg *msg;

	/* Check if the factory is building what it really thinks. */
	if (msg_id != TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_ID) {
		mdbg(ERR3, "Factory specified message ID(%x) doesn't match real ID(%x)",
		     msg_id, TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_ID);
		goto exit0;
	}
	/* Allocate the instance */
	if (!(msg = TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG(kmalloc(sizeof(struct tcmi_ppm_p_migr_back_shadowreq_procmsg), 
								 GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate migrate back shadow request message");
		goto exit0;
	}
	/* Initialize the message for receiving. */
	if (tcmi_procmsg_init_rx(TCMI_PROCMSG(msg), TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_ID, 
				 &ppm_p_migr_back_shadowreq_procmsg_ops)) {
		mdbg(ERR3, "Error initializing migrate back shadow request message %x", msg_id);
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
 * \<\<public\>\> Shadow request for migration back tx constructor.
 *
 * Generates a one-way request message
 *
 * @param dst_pid - PID of the target process that will receive this message
 * @return a new message ready for the transfer or NULL.
 */
struct tcmi_msg* tcmi_ppm_p_migr_back_shadowreq_procmsg_new_tx(pid_t dst_pid)
{
	struct tcmi_ppm_p_migr_back_shadowreq_procmsg *msg;

	if (!(msg = TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG(kmalloc(sizeof(struct tcmi_ppm_p_migr_back_shadowreq_procmsg), 
								 GFP_KERNEL)))) {
		mdbg(ERR3, "Can't create migrate back shadow request message");
		goto exit0;
	}

	/* Initialize the message for transfer, no transaction
	 * required, no timout, no response ID */
	if (tcmi_procmsg_init_tx(TCMI_PROCMSG(msg), TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_ID,
				 &ppm_p_migr_back_shadowreq_procmsg_ops,
				 dst_pid, 1,
				 NULL, 0, 0, TCMI_TRANSACTION_INVAL_ID)) {
		mdbg(ERR3, "Error initializing migrate back shadow request message");
		goto exit1;
	}

	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}

/** @addtogroup tcmi_ppm_p_migr_back_guestreq_procmsg_class
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
static int tcmi_ppm_p_migr_back_shadowreq_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	mdbg(INFO2, "PPM_P migrate back shadow request received guest PID=%d",
	     tcmi_procmsg_dst_pid(self));

	return 0;
}

/**
 * \<\<private\>\> Sends the PPM_P migrate back shadow request
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for sending message data
 * @return 0 when successfully sent.
 */
static int tcmi_ppm_p_migr_back_shadowreq_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock)
{
	mdbg(INFO2, "PPM_P migrate back shadow request sent guest PID=%d",
	     tcmi_procmsg_dst_pid(self));

	return 0;
}

/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops ppm_p_migr_back_shadowreq_procmsg_ops = {
	.recv = tcmi_ppm_p_migr_back_shadowreq_procmsg_recv,
	.send = tcmi_ppm_p_migr_back_shadowreq_procmsg_send,
};

/**
 * @}
 */
