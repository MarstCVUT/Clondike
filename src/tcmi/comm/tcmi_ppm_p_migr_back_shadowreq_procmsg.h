/**
 * @file tcmi_ppm_p_migr_back_shadowreq_procmsg.h - migrate back request initiated by shadow
 */
#ifndef _TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_H
#define _TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_H

#include "tcmi_procmsg.h"
#include "tcmi_err_procmsg.h"

/** @defgroup tcmi_ppm_p_migr_back_shadowreq_procmsg_class tcmi_ppm_p_migr_back_shadowreq_procmsg class
 *
 * @ingroup tcmi_procmsg_class
 *
 * This class represents a message that is sent by a shadow task to the
 * guest task as a request to migrate the process back to CCN
 *
 * @{
 */

/** Compound structure, inherits from tcmi_procmsg_class */
struct tcmi_ppm_p_migr_back_shadowreq_procmsg {
	/** parent class instance */
	struct tcmi_procmsg super;
};

/** \<\<public\>\> Shadow request for migration back rx constructor. */
extern struct tcmi_msg* tcmi_ppm_p_migr_back_shadowreq_procmsg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> Shadow request for migration back tx constructor. */
extern struct tcmi_msg* tcmi_ppm_p_migr_back_shadowreq_procmsg_new_tx(pid_t dst_pid);

/** Message descriptor for the factory class, for error handling we used tcmi_errmsg_class */
#define TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_DSC \
TCMI_MSG_DSC(TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_ID, tcmi_ppm_p_migr_back_shadowreq_procmsg_new_rx, tcmi_err_procmsg_new_rx)

/** Casts to the tcmi_ppm_p_migr_back_shadowreq_procmsg instance. */
#define TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG(m) ((struct tcmi_ppm_p_migr_back_shadowreq_procmsg*)m)

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_ppm_p_migr_back_shadowreq_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_ppm_p_migr_back_shadowreq_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops ppm_p_migr_back_shadowreq_procmsg_ops;

#endif /* TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_PRIVATE */


/**
 * @}
 */

#endif


