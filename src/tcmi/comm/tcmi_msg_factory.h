/**
 * @file tcmi_msg_factory.h - TCMI message factory message.
 *       
 *                      
 * 
 *
 *
 * Date: 04/09/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_msg_factory.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#ifndef _TCMI_MSG_FACTORY_H
#define _TCMI_MSG_FACTORY_H

#include "tcmi_msg.h"

/** @defgroup tcmi_msg_factory_class tcmi_msg_factory class
 *
 * @ingroup tcmi_comm_group
 *
 * This \<\<singleton\>\> class is responsible for message
 * materialization based on message ID. In addition it provides an
 * optional verification whether the specified message ID belongs
 * to the requested message group.
 *
 * @{
 */


/** \<\<public\>\> Materializes the message. */
extern struct tcmi_msg* tcmi_msg_factory(u_int32_t msg_id, u_int32_t msg_group);

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_MSG_FACTORY_PRIVATE

#endif /* TCMI_MSG_FACTORY_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_MSG_FACTORY_H */

