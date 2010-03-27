/**
 * @file tcmi_err_msg.h - TCMI error message - carries an error status code.
 *       
 *                      
 * 
 *
 *
 * Date: 04/11/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_err_msg.h,v 1.3 2007/09/02 10:54:25 stavam2 Exp $
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
#ifndef _TCMI_ERR_MSG_H
#define _TCMI_ERR_MSG_H

#include "tcmi_msg.h"

/** @defgroup tcmi_err_msg_class tcmi_err_msg class
 *
 * @ingroup tcmi_msg_class
 *
 * This class represents a generic error message. It carries an error
 * code and can be used as response to any regular message that has
 * registered its rx constructor as the error rx constructor with the
 * message factory. Unlike other messages, it doesn't define a fixed
 * message ID. Instead the transfer message constructor
 * (tcmi_err_msg_new_tx()) allows specifying a message ID that would
 * carry the regular response message.
 * 
 * The constructor only sets error flags in the message ID.  The
 * receiving party identifies the message based on the regular
 * response ID. The message flags indicating an error make \link
 * tcmi_msg_factory_class the message factory \endlink use the error rx
 * constructor.  After delivery, the error message can be processed
 * and the error code extracted.
 *
 * @{
 */

/** Compound structure, inherits from tcmi_msg_class */
struct tcmi_err_msg {
	/** parent class instance. */
	struct tcmi_msg super;
	/** error code that the message is carrying. */
	int32_t err_code;
};


/** \<\<public\>\> Error message, constructor for receiving. */
extern struct tcmi_msg* tcmi_err_msg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> Error message, constructor for transferring. */
extern struct tcmi_msg* tcmi_err_msg_new_tx(u_int32_t msg_id, u_int32_t trans_id, int32_t err_code);

/** Casts to the tcmi_err_msg instance. */
#define TCMI_ERR_MSG(m) ((struct tcmi_err_msg*)m)

/** 
 * \<\<public\>\> Error code accessor.
 *
 * @param *self - pointer to this message instance
 * @return error code that the message is carrying
 */
static inline int32_t tcmi_err_msg_code(struct tcmi_msg *self)
{
	return TCMI_ERR_MSG(self)->err_code;
}


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_ERR_MSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_err_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_err_msg_send(struct tcmi_msg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_msg_ops err_msg_ops;

#endif /* TCMI_ERR_MSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_ERR_MSG_H */

