/**
 * @file tcmi_msg.h - TCMI communication message.
 *       
 *                      
 * 
 *
 *
 * Date: 04/09/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_msg.h,v 1.4 2007/09/02 10:54:25 stavam2 Exp $
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
#ifndef _TCMI_MSG_H
#define _TCMI_MSG_H

#include <linux/list.h>
#include <asm/atomic.h>
#include <linux/types.h>
#include <linux/sched.h>

#include "tcmi_transaction.h"
#include "tcmi_messages_id.h"

#include <tcmi/lib/tcmi_slotvec.h>
#include <tcmi/lib/tcmi_queue.h>

#include <kkc/kkc_sock.h>

/** @defgroup tcmi_msg_class tcmi_msg class
 *
 * @ingroup tcmi_comm_group
 *
 * This class represents a generic message in TCMI communication
 * protocol. A message can be part of a \link tcmi_transaction_class
 * transaction\endlink. For that reason it carries two extra ID's 
 * -# a transaction request ID - identifies a transaction started
 * by the sending party. The sending party expects a message with response
 * transaction ID set to this value. This ID is assigned automatically
 * upon message creation depending on a particular messagere type.
 * -# a transaction response ID - this value comes from the
 * transaction request ID of the original request message that the new
 * message is replying to.  This ID has to be specified explicitely
 * when building the response message.
 *
 * A message is instantiated always twice during its lifetime:
 *
 * - the sender builds the message based on parameters specified by
 * the message instantiator.
 * - the receiver performs message materialization from the data stream.
 *
 * These two cases are substantially different. In the first case the
 * instantiator knows what message is to be built. While the receiver
 * is left to the very last moment upon discovery of the message ID to build
 * the message. A suitable solution to this problem is provided by a
 * a \<\<factory\>\> design pattern. The message instantiation on the receiver
 * side is carried out by \link tcmi_msg_factory_class message factory \endlink.
 *
 * A generic message consists of following parts:
 * - message id - unique identifier that describes a particular message type.
 * This is specified by particular subclasses.
 * - transaction request ID - explained above
 * - transaction response ID - explained above
 *
 * Generic message methods: 
 * - init_rx - initializes a new message so that it is ready to be
 * received.
 * - init_tx - initializes a new message for sending based on
 * parameters specified by the user.
 * - recv - receives a message via a communication channel
 * - send - sends out a message via a communication channel
 * - deliver - delivers a request message to a queue or delivers a response
 * message to a transaction and completes the transaction.
 *
 * A particular message of the protocol then provides following interface:
 * - new_rx - this method is registered with the factory class, so that
 * the build process is fully automated.
 * - new_rx_err - also registered with the factory class, provides an
 * alternate(error) version of the message.
 * - new_tx - builds a message based on parameters specified by the user
 *
 * @{
 */

/** A coumpound structure that carries a generic TCMI message. */
struct tcmi_msg {
	/** message id */
	u_int32_t msg_id;
	/** groups transactions ID's, so that they are sent at once */
	struct {
		/** request transaction ID */
		u_int32_t req;
		/** response transaction ID */
		u_int32_t resp;
	} trans_id  __attribute__((__packed__));
	/** transaction associated with the message */
	struct tcmi_transaction *transaction;

	/** message operations */
	struct tcmi_msg_ops *msg_ops;

	/** reference counter */
	atomic_t ref_count;
	/** list entry for the message queue */
	struct list_head node;

};

/** Message operations that support polymorphism. */
struct tcmi_msg_ops {
	/** Receives the message via a specified connection. */
	int (*recv)(struct tcmi_msg*, struct kkc_sock*);
	/** Sends the message via a specified connection. */
	int (*send)(struct tcmi_msg*, struct kkc_sock*);
	/** Frees custom message resources. The destruction of the
	 * actual message instance is handled internally by this
	 * class */
	void (*free)(struct tcmi_msg*);
};

/** This descriptor is to be provided by every individual message
 * class for the \link tcmi_msg_factory_class factory \endlink. */
struct tcmi_msg_dsc {
	/** message id */
	u_int32_t msg_id;
	/** Builds the regular version of the message */
	struct tcmi_msg* (*new_rx)(u_int32_t msg_id);
	/** Builds the error version of the message */
	struct tcmi_msg* (*new_rx_err)(u_int32_t msg_id);
};

/** Macro that fills the descriptor */
#define TCMI_MSG_DSC(id, new_rx_m, new_rx_err_m)			\
{.msg_id = id, .new_rx = new_rx_m, .new_rx_err = new_rx_err_m}
/*{.msg_id = id, .new_rx = new_rx_m, .new_rx_err = new_rx_err_m}*/


/** Message transaction default timeout set to 60 seconds*/
#define TCMI_DEFAULT_MSG_TIMEOUT 60*HZ

/** Casts to the tcmi_msg instance. */
#define TCMI_MSG(m) ((struct tcmi_msg*)m)

/** \<\<public\>\> Initializes the message for receiving. */
extern int tcmi_msg_init_rx(struct tcmi_msg *self, u_int32_t msg_id,
			    struct tcmi_msg_ops *msg_ops);

/** \<\<public\>\> Initializes the message for transferring. */
extern int tcmi_msg_init_tx(struct tcmi_msg *self, u_int32_t msg_id, 
			    struct tcmi_msg_ops *msg_ops,
			    struct tcmi_slotvec *transactions, u_int32_t resp_msg_id,
			    u_int timeout,
			    u_int32_t resp_trans_id);
/** \<\<public\>\> Receives the message via a specified connection. */
extern int tcmi_msg_recv(struct tcmi_msg *, struct kkc_sock *sock);

/** \<\<public\>\> Sends a message via a specified connection. */
extern int tcmi_msg_send_anonymous(struct tcmi_msg *self, struct kkc_sock *sock);

/** \<\<public\>\> Sends a message via a specified connection and waits for the response. */
extern int tcmi_msg_send_and_receive(struct tcmi_msg *self, struct kkc_sock *sock, 
				     struct tcmi_msg **resp);


/**\<\<public\>\>  Message delivers itself.*/
extern int tcmi_msg_deliver(struct tcmi_msg *self, struct tcmi_queue *requests, 
			    struct tcmi_slotvec *transactions);

/** \<\<public\>\> Class method - creates a message based on a descriptor. */
extern struct tcmi_msg* tcmi_msg_new_rx(u_int32_t msg_id, struct tcmi_msg_dsc *dsc);

/** 
 * \<\<public\>\> Instance accessor, increments the reference count.
 *
 * @param *self - pointer to this message instance
 */
static inline struct tcmi_msg* tcmi_msg_get(struct tcmi_msg *self)
{
	if (self) {
		atomic_inc(&self->ref_count);
	}
	return self;
}

/** 
 * \<\<public\>\> Decrements reference counter, if it reaches 0 the
 * custom free method is called if defined. Eventually, the instance
 * is destroyed.
 *
 * The user is responsible for taking the message out of the message
 * queue or detaching it from the transaction when needed.
 *
 * @param *self - pointer to this message instance
 */
static inline void tcmi_msg_put(struct tcmi_msg *self)
{
	if (!self)
		return;
	if (atomic_dec_and_test(&self->ref_count)) {
		mdbg(INFO4, "Destroying message ID %x, %p", self->msg_id, self);
		if(self->msg_ops && self->msg_ops->free) 
			self->msg_ops->free(self);
		tcmi_transaction_put(self->transaction);
		kfree(self);
	}
}

/** 
 * \<\<public\>\> Message ID accessor.
 *
 * @param *self - pointer to this message instance
 * @return message ID
 */
static inline u_int32_t tcmi_msg_id(struct tcmi_msg *self)
{
	return self->msg_id;
}

/** 
 * \<\<public\>\> Message error ID accessor.
 *
 * @param *self - pointer to this message instance
 * @return message ID extended with error flags
 */
static inline u_int32_t tcmi_msg_errid(struct tcmi_msg *self)
{
	return TCMI_MSG_FLG_SET_ERR(self->msg_id);
}

/** 
 * \<\<public\>\> Request transaction ID accessor.  This is needed
 * when constructing reply messages to set a proper response
 * transaction ID.
 *
 * @param *self - pointer to this message instance
 * @return message ID extended with error flags
 */
static inline u_int32_t tcmi_msg_req_id(struct tcmi_msg *self)
{
	return self->trans_id.req;
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_MSG_PRIVATE

/** Sends the message via a specified connection. */
static int tcmi_msg_send(struct tcmi_msg *self, struct kkc_sock *sock, int flags);

#endif /* TCMI_MSG_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_MSG_H */

