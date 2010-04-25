/**
 * @file tcmi_messages_id.h - This is a list of all identifiers of  migration messages.
 *
 *       
 *                      
 * 
 *
 *
 * Date: 04/12/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_messages_id.h,v 1.7 2008/05/02 19:59:20 stavam2 Exp $
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
#ifndef _TCMI_MIG_MESSAGES_ID_H
#define _TCMI_MIG_MESSAGES_ID_H

/** @defgroup tcmi_messages_id message ID's
 *
 * @ingroup tcmi_comm_group
 *
 * Message ID has 3 components:
 *
 * - bits 31-30 - message flags
 *              - 0 - regular message
 *              - 1 - error message
 *              - 2 - reserved
 *              - 3 - reserved
 * - bits 29-16 - message group ID. Initially, there are 2 groups:
 *              - 0 - migration control connection messages
 *              - 1 - process control messages
 *
 * - bits 15-0  - a unique message index within a particular
 *              group.
 \verbatim
   31   30 29       16 15         0
   +------+-----------+-----------+
   | flgs |   group   |   index   |
   +------+-----------+-----------+
           \                     /
            \                   /
             \                 /
              \               /
               \             / 
                 message type - used when associating
   a response message with a transaction
 \endverbatim
 * @{
 */
/** There are 2^index_bits messages */
#define TCMI_MSG_INDEX_BITS     16
/** There are 2^group_bits message groups */
#define TCMI_MSG_GROUP_BITS     14
/** There are 2^flg_bits flags available */
#define TCMI_MSG_FLG_BITS       2
/** Total number of bits reserved for message identification. */
#define TCMI_MSG_TYPE_BITS      (TCMI_MSG_GROUP_BITS + TCMI_MSG_INDEX_BITS) 

/** Mask for the message index */
#define TCMI_MSG_INDEX_MSK      ((1 << TCMI_MSG_INDEX_BITS) - 1)
/** Extracts message index */
#define TCMI_MSG_INDEX(msg_id) (msg_id & TCMI_MSG_INDEX_MSK)
/** Mask for the group */
#define TCMI_MSG_GROUP_MSK      ((1 << TCMI_MSG_GROUP_BITS) - 1)
/** Extracts group number */
#define TCMI_MSG_GROUP(msg_id) ((msg_id >> TCMI_MSG_INDEX_BITS) & TCMI_MSG_GROUP_MSK)
/** Mask for the message type */
#define TCMI_MSG_TYPE_MSK       ((1 << TCMI_MSG_TYPE_BITS) - 1)
/** Extracts the message type */
#define TCMI_MSG_TYPE(msg_id)  (msg_id & TCMI_MSG_TYPE_MSK)
/** Mask for the flag bits */
#define TCMI_MSG_FLG_MSK        ((1 << TCMI_MSG_FLG_BITS) - 1)
/** Extracts the message flags */
#define TCMI_MSG_FLG(msg_id)   ((msg_id >> TCMI_MSG_TYPE_BITS) & TCMI_MSG_FLG_MSK)
/** Flag shift */
#define TCMI_MSG_FLG_SHIFT      TCMI_MSG_TYPE_BITS
/** Sets a specified group */
#define TCMI_MSG_SET_GRP(grp)  (grp << TCMI_MSG_INDEX_BITS)

/** Regular message flag */
#define TCMI_MSG_REGFLGS        0
/** Error message flag */
#define TCMI_MSG_ERRFLGS        1
/** Sets an error flag */
#define TCMI_MSG_FLG_SET_ERR(id)    (id | (TCMI_MSG_ERRFLGS << TCMI_MSG_FLG_SHIFT))


/** Message groups - each group has a unique ID. */
enum {
	TCMI_MSG_GROUP_MIG  = 0, /* TCMI migration messages */
	TCMI_MSG_GROUP_PROC,     /* TCMI process control messages */
	TCMI_MSG_GROUP_LAST,
	TCMI_MSG_GROUP_ANY  = 0xffffffff
};

/********** Following enums define message types within each group *********/

/** TCMI Migration messages */
enum {
	TCMI_SKEL_MSG_ID = TCMI_MSG_SET_GRP(TCMI_MSG_GROUP_MIG), /* TCMI test message */
	TCMI_SKELRESP_MSG_ID,                                    /* TCMI test response message */
	TCMI_AUTHENTICATE_MSG_ID,						 /* TCMI authentication request */
	TCMI_AUTHENTICATE_RESP_MSG_ID,                                    /* TCMI authentication response */
	TCMI_DISCONNECT_MSG_ID,						 /* TCMI disconnect request */
	TCMI_P_EMIGRATE_MSG_ID,                              	 /* TCMI phys. emigration message */
	TCMI_SIGNAL_MSG_ID,                                      /* TCMI signal message */
        TCMI_GENERIC_USER_MSG_ID,                            /* Generic user message */
	TCMI_LAST_MSG_ID                                         /* Last ID */
};

/** TCMI Process control messages */
enum {
	TCMI_SKEL_PROCMSG_ID = TCMI_MSG_SET_GRP(TCMI_MSG_GROUP_PROC), /* TCMI test process message */
	TCMI_SKELRESP_PROCMSG_ID,                                     /* TCMI test response process message */
	TCMI_EXIT_PROCMSG_ID,                                         /* TCMI guest task exit mesage */
	TCMI_VFORK_DONE_PROCMSG_ID,                                   /* TCMI guest task execved -> parent vfork is completed */
	TCMI_GUEST_STARTED_PROCMSG_ID,                                /* TCMI guest task has been started */
	TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_ID,                     /* TCMI migrate back guest request*/
	TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_ID,                     /* TCMI migrate back request from shadow task */
	TCMI_RPC_PROCMSG_ID,                                          /* TCMI RPC mesage */
	TCMI_RPCRESP_PROCMSG_ID,                                      /* TCMI RPC response mesage */

	TCMI_LAST_PROCMSG_ID                                          /* Last ID */
};

/**
 * @}
 */

#endif /* _TCMI_MIG_MESSAGES_ID_H */

