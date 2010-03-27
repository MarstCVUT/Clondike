/**
 * @file tcmi_generic_user_msg.h - generic userspace message.
 */

#ifndef TCMI_GENERIC_USER_PROCMSG_H
#define TCMI_GENERIC_USER_PROCMSG_H

#include "tcmi_msg.h"

/** @defgroup tcmi_generic_user_class tcmi_generic_user_procmsg class
 *
 * @ingroup tcmi_msg_class
 *
 * This message can be send both by PEN or CCN. It can contain any data that are passed from user space to the director of peer node.
 * 
 * @{
 */

/** Generic user message structure */
struct tcmi_generic_user_procmsg {
	/** parent class instance. */
	struct tcmi_msg super;
	/** Id of the requestor PEN or CCN */
	u_int32_t node_id;

	/** User passed data. Opaque to the kernel part, this is purely user space responsibilithy to provide and parse those. */
	char *user_data;
	/** Size of user data */
	u_int32_t size;
};




/** \<\<public\>\> Message consutructor for receiving. */
extern struct tcmi_msg* tcmi_generic_user_procmsg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> Message constructor for transferring. */
extern struct tcmi_msg* tcmi_generic_user_procmsg_new_tx(u_int32_t node_id, char* user_data, int size);


/** \<\<public\>\> Message descriptor for the factory class, there is no error
 * handling as this is the starting request */
#define TCMI_GENERIC_USER_PROCMSG_DSC TCMI_MSG_DSC(TCMI_GENERIC_USER_PROCMSG_ID, tcmi_generic_user_procmsg_new_rx, NULL)

/** Casts to the tcmi_generic_user_msg instance. */
#define TCMI_GENERIC_USER_PROCMSG(m) ((struct tcmi_generic_user_procmsg*)m)

/**
 * \<\<public\>\> Node id getter
 * 
 * @param *self - this message instance
 * @return Node id
 */
static inline u_int32_t tcmi_generic_user_procmsg_node_id(struct tcmi_generic_user_procmsg *self)
{
	return self->node_id;
}

/**
 * \<\<public\>\> User data length getter
 * 
 * @param *self - this message instance
 * @return data length
 */
static inline u_int32_t tcmi_generic_user_procmsg_user_data_size(struct tcmi_generic_user_procmsg *self)
{
	return self->size;
}

/**
 * \<\<public\>\> User data data getter
 * 
 * @param *self - this message instance
 * @return User data
 */
static inline char* tcmi_generic_user_procmsg_user_data(struct tcmi_generic_user_procmsg *self)
{
	return self->user_data;
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_GENERIC_USER_MSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_generic_user_procmsg_recv(struct tcmi_msg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_generic_user_procmsg_send(struct tcmi_msg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_msg_ops generic_user_procmsg_ops;

#endif


/**
 * @}
 */


#endif

