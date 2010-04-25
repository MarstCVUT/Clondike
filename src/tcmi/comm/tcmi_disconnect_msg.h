/**
 * @file tcmi_disconnect_msg.h - disconnect message
 */

#ifndef TCMI_DISCONNECT_MSG_H
#define TCMI_DISCONNECT_MSG_H

#include "tcmi_msg.h"
#include <arch/arch_ids.h>

/** @defgroup tcmi_disconnect_msg_class tcmi_disconnect_msg class
 *
 * @ingroup tcmi_msg_class
 *
 * This message is send by PEN or CCN when it is going to disconnect from its peer. It serves as a notification, to tell the peer it shall as well terminate the connection.
 * In addition, if send from CCN to PEN, the PEN should as a resonpse first try to migrate back all tasks from a peer core node.
 *
 * @{
 */

/** Disconnect message structure */
struct tcmi_disconnect_msg {
	/** parent class instance. */
	struct tcmi_msg super;
};




/** \<\<public\>\> Disconnect message consutructor for receiving. */
extern struct tcmi_msg* tcmi_disconnect_msg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> Disconnect message constructor for transferring. */
extern struct tcmi_msg* tcmi_disconnect_msg_new_tx(void);


/** \<\<public\>\> Message descriptor for the factory class, there is no error
 * handling as this is the starting request */
#define TCMI_DISCONNECT_MSG_DSC TCMI_MSG_DSC(TCMI_DISCONNECT_MSG_ID, tcmi_disconnect_msg_new_rx, NULL)

/** Casts to the tcmi_disconnect_msg instance. */
#define TCMI_DISCONNECT_MSG(m) ((struct tcmi_disconnect_msg*)m)

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_DISCONNECT_MSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_disconnect_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_disconnect_msg_send(struct tcmi_msg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_msg_ops disconnect_msg_ops;

#endif


/**
 * @}
 */


#endif

