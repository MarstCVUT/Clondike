/**
 * @file tcmi_ppm_p_migr_back_guestreq_procmsg.h - migrate back request initiated by guest
 *       
 *                      
 * 
 *
 *
 * Date: 05/04/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ppm_p_migr_back_guestreq_procmsg.h,v 1.3 2007/09/02 10:54:25 stavam2 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2005  Jan Capek
 * 
 * TCMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * TCMI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef _TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_H
#define _TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_H

#include "tcmi_procmsg.h"
#include "tcmi_err_procmsg.h"

/** @defgroup tcmi_ppm_p_migr_back_guestreq_procmsg_class tcmi_ppm_p_migr_back_guestreq_procmsg class
 *
 * @ingroup tcmi_procmsg_class
 *
 * This class represents a message that is sent by guest task to the
 * shadow task as a request(announcement) to migrate the process back to CCN
 *
 * @{
 */

/** Compound structure, inherits from tcmi_procmsg_class */
struct tcmi_ppm_p_migr_back_guestreq_procmsg {
	/** parent class instance */
	struct tcmi_procmsg super;
	/** size of the checkpoint name in bytes (including trailing
	    zero) */
	int32_t size;
	/** name of the checkpoint file. */
	char *ckpt_name;
};




/** \<\<public\>\> Guest request for migration back rx constructor. */
extern struct tcmi_msg* tcmi_ppm_p_migr_back_guestreq_procmsg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> Guest request for migration back tx constructor. */
extern struct tcmi_msg* tcmi_ppm_p_migr_back_guestreq_procmsg_new_tx(pid_t dst_pid, char *ckpt_name);



/** Message descriptor for the factory class, for error handling we used tcmi_errmsg_class */
#define TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_DSC \
TCMI_MSG_DSC(TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_ID, tcmi_ppm_p_migr_back_guestreq_procmsg_new_rx, tcmi_err_procmsg_new_rx)

/** Casts to the tcmi_ppm_p_migr_back_guestreq_procmsg instance. */
#define TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG(m) ((struct tcmi_ppm_p_migr_back_guestreq_procmsg*)m)

/**
 * Checkpoint name accessor
 * 
 * @param *self - this message instance
 * @return checkpoint name string
 */
static inline char* tcmi_ppm_p_migr_back_guestreq_procmsg_ckpt_name(struct tcmi_ppm_p_migr_back_guestreq_procmsg *self)
{
	return self->ckpt_name;
}


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_ppm_p_migr_back_guestreq_procmsg_recv(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_ppm_p_migr_back_guestreq_procmsg_send(struct tcmi_procmsg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_procmsg_ops ppm_p_migr_back_guestreq_procmsg_ops;

#endif /* TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_H */

