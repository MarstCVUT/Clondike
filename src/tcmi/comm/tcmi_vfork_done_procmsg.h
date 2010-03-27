/**
 * @file tcmi_vfork_done_procmsg.h - TCMI vfork done, send to notify about vfork completition
 */
#ifndef _TCMI_VFORK_DONE_PROCMSG_H
#define _TCMI_VFORK_DONE_PROCMSG_H

#include "tcmi_procmsg.h"

/** @defgroup tcmi_vfork_done_procmsg_class tcmi_vfork_done_procmsg class
 *
 * @ingroup tcmi_procmsg_class
 *
 * This class represents an vfork done notification message.
 *
 * @{
 */

/** Compound structure, inherits from tcmi_procmsg_class */
struct tcmi_vfork_done_procmsg {
	/** parent class instance. */
	struct tcmi_procmsg super;
};




/** \<\<public\>\> Vfork done process message constructor for receiving. */
extern struct tcmi_msg* tcmi_vfork_done_procmsg_new_rx(u_int32_t msg_id);

/** \<\<public\>\>  Vfork done process message constructor for transferring. */
extern struct tcmi_msg* tcmi_vfork_done_procmsg_new_tx(pid_t dst_pid);


/** Message descriptor for the factory class, there is no error
 * handling as this is the starting request */
#define TCMI_VFORK_DONE_PROCMSG_DSC TCMI_MSG_DSC(TCMI_VFORK_DONE_PROCMSG_ID, tcmi_vfork_done_procmsg_new_rx, NULL)

/** Casts to the tcmi_vfork_done_procmsg instance. */
#define TCMI_VFORK_DONE_PROCMSG(m) ((struct tcmi_vfork_done_procmsg*)m)


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_VFORK_DONE_PROCMSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_vfork_done_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_vfork_done_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops vfork_done_procmsg_ops;

#endif /* TCMI_VFORK_DONE_PROCMSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_VFORK_DONE_PROCMSG_H */

