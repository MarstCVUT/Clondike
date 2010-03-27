/**
 * @file tcmi_msg_factory.c - TCMI message factory message.
 *       
 *                      
 * 
 *
 *
 * Date: 04/09/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_msg_factory.c,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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

#define TCMI_MSG_FACTORY_PRIVATE
#include "tcmi_msg_factory.h"
#include "tcmi_messages_dsc.h"

#include <dbg.h>

/** 
 * \<\<public\>\> Materializes the message.
 * Initially, a verification is performed whether the message ID belongs to the
 * requested message group. This way, unexpected messages from a wrong message
 * group won't get materialized.
 *
 * Once a proper message descriptor is retrieved, the message creation
 * is delegated to the \link tcmi_msg_class protocol message super class \endlink
 * 
 * @param msg_id - Full identifier of the message including flags.
 * @param msg_group - this is for verification, when the factory user wants to make sure
 * the message comes from a particular message group
 * @return message instance or NULL
 */
extern struct tcmi_msg* tcmi_msg_factory(u_int32_t msg_id, u_int32_t msg_group)
{
	struct tcmi_msg *msg = NULL;
	u_int32_t group_id = TCMI_MSG_GROUP(msg_id);
	u_int32_t index = TCMI_MSG_INDEX(msg_id);
	
	/* check if the requested group matches the message */
	if ((group_id != msg_group) && (msg_group != TCMI_MSG_GROUP_ANY)) {
		mdbg(ERR3, "Requested message ID (%x), doesn't match the requested group(%x)",
		     TCMI_MSG_GROUP(msg_id), msg_group);
		goto exit0;

	}
	/* check if the group id is valid and the index identifies a
	 * message within a group */
	if (!((group_id < TCMI_MSG_GROUP_LAST) &&
	     (index < msg_groups[group_id].msg_count))) {
		mdbg(ERR3, "Invalid message group(%d) or index(%d)",
		     group_id, index);
		goto exit0;
	}
	mdbg(INFO3, "Found a valid message group(%d) and index(%d), msg_count=%d",
	     group_id, index, msg_groups[group_id].msg_count);
	if (!(msg = tcmi_msg_new_rx(msg_id, &msg_groups[group_id].dsc[index]))) {
		mdbg(ERR3, "Failed to allocate a new message ID %x", msg_id);
	}
		
	return msg;

	/* error handling */
 exit0:
	return NULL;
}
