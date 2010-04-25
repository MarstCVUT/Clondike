/**
 * @file tcmi_messages_dsc.h - A list of descriptors of all TCMI messages and
 * their groups.
 *       
 *                      
 * 
 *
 *
 * Date: 04/12/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_messages_dsc.h,v 1.7 2008/05/02 19:59:20 stavam2 Exp $
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
#ifndef _TCMI_MESSAGES_DSC_H
#define _TCMI_MESSAGES_DSC_H

#include "tcmi_msg.h"

/* migration messages */
#include "tcmi_skel_msg.h"
#include "tcmi_skelresp_msg.h"
#include "tcmi_signal_msg.h"
#include "tcmi_p_emigrate_msg.h"
#include "tcmi_authenticate_msg.h"
#include "tcmi_authenticate_resp_msg.h"
#include "tcmi_disconnect_msg.h"

/* process messages */
#include "tcmi_skel_procmsg.h"
#include "tcmi_skelresp_procmsg.h"
#include "tcmi_exit_procmsg.h"
#include "tcmi_rpc_procmsg.h"
#include "tcmi_rpcresp_procmsg.h"
#include "tcmi_guest_started_procmsg.h"
#include "tcmi_ppm_p_migr_back_guestreq_procmsg.h"
#include "tcmi_ppm_p_migr_back_shadowreq_procmsg.h"
#include "tcmi_vfork_done_procmsg.h"
#include "tcmi_generic_user_msg.h"

/** @defgroup tcmi_messages_dsc message descriptors
 *
 * @ingroup tcmi_comm_group
 *
 * Message descriptors are internally used by tcmi_msg_factory_class
 * when materializing messages that are being received. In future, this
 * file will be automatically generated based on all tcmi message
 * declarations.
 *
 * @{
 */

/** Describes a group of messages. */
struct tcmi_msg_group {
	/** array of message descriptors within a group */
	struct tcmi_msg_dsc *dsc;
	/** message count in the group */
	u_int32_t msg_count;
};


#ifdef TCMI_MSG_FACTORY_PRIVATE

/******* M I G R A T I O N  C O N T R O L  M E S S A G E S *********/

/** Individual descriptors of migration messages(sent via migration
 * control control connection */
static struct tcmi_msg_dsc mig_messages[] = {
	TCMI_SKEL_MSG_DSC,
	TCMI_SKELRESP_MSG_DSC,
	TCMI_AUTHENTICATE_MSG_DSC,
	TCMI_AUTHENTICATE_RESP_MSG_DSC,
	TCMI_DISCONNECT_MSG_DSC,
	TCMI_P_EMIGRATE_MSG_DSC,
	TCMI_SIGNAL_MSG_DSC,
	TCMI_GENERIC_USER_MSG_DSC,
};


/********* P R O C E S S  C O N T R O L  M E S S A G E S ***********/
/** Individual descriptors of process messages(sent via process
 * control connection */
static struct tcmi_msg_dsc proc_messages[] = {
	TCMI_SKEL_PROCMSG_DSC,
	TCMI_SKELRESP_PROCMSG_DSC,
	TCMI_EXIT_PROCMSG_DSC,
	TCMI_VFORK_DONE_PROCMSG_DSC,
	TCMI_GUEST_STARTED_PROCMSG_DSC,
	TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_DSC,
	TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_DSC,
	TCMI_RPC_PROCMSG_DSC,
	TCMI_RPCRESP_PROCMSG_DSC,	
};



/****************** M E S S A G E  G R O U P S *********************/
/** List of supported groups, new message groups are to be added here */
static struct tcmi_msg_group msg_groups[] = {
	{
	 .dsc = mig_messages,
	 .msg_count = TCMI_MSG_INDEX(TCMI_LAST_MSG_ID)
	},
	{
	 .dsc = proc_messages,
	 .msg_count = TCMI_MSG_INDEX(TCMI_LAST_PROCMSG_ID)
	}
};



#endif /* TCMI_MSG_FACTORY_PRIVATE */

/**
 * @}
 */

#endif /* _TCMI_MESSAGES_DSC_H */


