/**
 * @file tcmi_authenticate_resp_msg.h - authentication response message
 */

#ifndef TCMI_AUTHENTICATE_RESP_MSG_H
#define TCMI_AUTHENTICATE_RESP_MSG_H

#include "tcmi_msg.h"
#include <arch/arch_ids.h>
#include <tcmi/migration/fs/fs_mount_params.h>

/** @defgroup tcmi_authenticate_resp_msg_class tcmi_authenticate_resp_msg class
 *
 * @ingroup tcmi_msg_class
 *
 * This message is send as a response to authentication request by the CCN to PEN
 * 
 * @{
 */

/** Authentication response message structure */
struct tcmi_authenticate_resp_msg {
	/** parent class instance. */
	struct tcmi_msg super;
	/** Id of the ccn that was asked for the authentication */
	u_int32_t ccn_id;
	/** Architecture of ccn */
	enum arch_ids ccn_arch;
	/** Parameters of the mount operation to be performed on task execve on PEN side */
	struct fs_mount_params mount_params;
	/** 0 if authentication succeeded, negative otherwise */
	int8_t result_code;
};




/** \<\<public\>\> Authentication response message constructor for receiving. */
extern struct tcmi_msg* tcmi_authenticate_resp_msg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> Authentication response message constructor for transferring. */
extern struct tcmi_msg* tcmi_authenticate_resp_msg_new_tx(u_int32_t trans_id, 
						       u_int32_t ccn_id, enum arch_ids ccn_arch, int8_t result_code, struct fs_mount_params* mount_params);


/** \<\<public\>\> Message descriptor for the factory class */
#define TCMI_AUTHENTICATE_RESP_MSG_DSC TCMI_MSG_DSC(TCMI_AUTHENTICATE_RESP_MSG_ID, tcmi_authenticate_resp_msg_new_rx, tcmi_err_msg_new_rx)

/** Casts to the tcmi_authenticate_resp_msg instance. */
#define TCMI_AUTHENTICATE_RESP_MSG(m) ((struct tcmi_authenticate_resp_msg*)m)

/**
 * \<\<public\>\> CCN id getter
 * 
 * @param *self - this message instance
 * @return CCN id
 */
static inline u_int32_t tcmi_authenticate_resp_msg_ccn_id(struct tcmi_authenticate_resp_msg *self)
{
	return self->ccn_id;
}

/**
 * \<\<public\>\> CCN architecture getter
 * 
 * @param *self - this message instance
 * @return CCN architecture
 */
static inline enum arch_ids tcmi_authenticate_resp_msg_arch(struct tcmi_authenticate_resp_msg *self)
{
	return self->ccn_arch;
}

/**
 * \<\<public\>\> Result code getter
 * 
 * @param *self - this message instance
 * @return result code
 */
static inline int8_t tcmi_authenticate_resp_msg_result_code(struct tcmi_authenticate_resp_msg *self)
{
	return self->result_code;
}

/**
 * \<\<public\>\> Mount params getter
 * 
 * @param *self - this message instance
 * @return Mount params
 */
static inline struct fs_mount_params* tcmi_authenticate_resp_msg_mount_params(struct tcmi_authenticate_resp_msg *self) {
	return &self->mount_params;
}


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_AUTHENTICATE_RESP_MSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_authenticate_resp_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_authenticate_resp_msg_send(struct tcmi_msg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_msg_ops authenticate_resp_msg_ops;

#endif


/**
 * @}
 */


#endif

