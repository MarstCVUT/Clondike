/**
 * @file tcmi_msg.c - TCMI communication message.
 *       
 *                      
 * 
 *
 *
 * Date: 04/10/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_msg.c,v 1.3 2008/01/17 14:36:44 stavam2 Exp $
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

#include <linux/kernel.h>

#define TCMI_MSG_PRIVATE
#include "tcmi_msg.h"


#include <dbg.h>

/** Shared initialization fuction of both rx and tx messages */
static int tcmi_msg_init_shared(struct tcmi_msg *self, u_int32_t msg_id, struct tcmi_msg_ops *msg_ops) {
	mdbg(INFO4, "Initialized message ID=%u, %p", msg_id, self);
	self->msg_id = msg_id;
	/* initially no transaction associated with the message */
	self->trans_id.req = TCMI_TRANSACTION_INVAL_ID;
	self->trans_id.resp = TCMI_TRANSACTION_INVAL_ID;
	self->transaction = NULL;

	self->msg_ops = msg_ops;
	atomic_set(&self->ref_count, 1);

	/* initialize list head for message queueing */
	INIT_LIST_HEAD(&self->node);
	return 0;
}  

/** 
 * \<\<public\>\> Initializes the message for receiving.
 *
 * @param *self - pointer to this message instance
 * @param msg_id - ID of a particular message type
 * @param *msg_ops - pointer to the message operations
 * @return 0 upon success
 */
int tcmi_msg_init_rx(struct tcmi_msg *self, u_int32_t msg_id,
		     struct tcmi_msg_ops *msg_ops)
{
  return tcmi_msg_init_shared(self, msg_id, msg_ops);
}

/** 
 * \<\<public\>\> Initializes the message for transferring.  For
 * oneway messages sets only message ID and message operations and
 * associated transaction ID (for responses).  If there is a response
 * expected, sets up a new transaction.
 *
 * @param *self - pointer to this message instance
 * @param msg_id - ID of a particular message type
 * @param *msg_ops - pointer to the message specific operations
 * @param *transactions - storage for the new transaction(NULL for one-way messages)
 * @param resp_msg_id - response message ID - required for the transaction
 * if the transaction expires(NULL for one-way messages)
 * @param timeout - transaction time out (0 for one-way messages)
 * @param resp_trans_id - response transaction ID - used when
 * this message is a response to a particular request. The user must set
 * this ID based on the value ofr req_trans_id of the previously received message.
 * @return 0 upon success
 */
int tcmi_msg_init_tx(struct tcmi_msg *self, u_int32_t msg_id, 
		     struct tcmi_msg_ops *msg_ops,
		     struct tcmi_slotvec *transactions, u_int32_t resp_msg_id,
		     u_int timeout,
		     u_int32_t resp_trans_id)
{
	int error = 0;
	/* basic init is the same as when receiving a message */
	tcmi_msg_init_shared(self, msg_id, msg_ops);
	/* transaction ID, needed if this message is a response to a previously sent message */
	self->trans_id.resp = resp_trans_id;
	/* setup transaction when needed */
	if (transactions) {
		if (!(self->transaction = 
		      tcmi_transaction_new(transactions, 
					   TCMI_MSG_TYPE(resp_msg_id), timeout))) {
			mdbg(ERR3, "Can't create a new transaction for message(ID=%x)", msg_id);
			error = -1;
		}
		/* when a transaction was requested, set proper
		 * request ID, so that the receiving is able to reply
		 */
		else
			self->trans_id.req = tcmi_transaction_id(self->transaction);
	}
	return error;
}


/** 
 * \<\<public\>\> Sends a message via a specified connection.  Sending
 * an anonymous message means, that if there is a transaction
 * associated with the message, the response message will be delivered
 * into regular message queue. This is typically used when the sending
 * thread doesn't want to process the response message itself, but
 * still wants the response to be checked against the transaction.
 *
 * The actual work is delegated to tcmi_msg_send() specifying the
 * anonymous flag.  Then the transaction instance is released as the
 * sending thread doesn't needed it. The status of the send operation is
 * communicated to the user.
 *
 * This method is also to be used for sending one-way request messages.
 * 
 * @param *self - pointer to this message instance
 * @param *sock - KKC socket used for sending message data
 * be set in the transaction.
 * @return 0 upon success
 */
int tcmi_msg_send_anonymous(struct tcmi_msg *self, struct kkc_sock *sock)
{
	int err;
	err = tcmi_msg_send(self, sock, TCMI_TRANS_FLAGS_ANONYMOUS);
	return err;
}


/** 
 * \<\<public\>\> Sends a message via a specified connection and waits
 * for the response.  This method is used when the message being sent
 * out has a transaction and a response is expected. The calling
 * thread blocks until a valid response arrives or the transaction
 * expires.
 *
 * The actual work is delegated to tcmi_msg_send() specifying no extra
 * flags for the transaction. The thread is then put to sleep waiting
 * for completion of the transaction. When its woken up again, the transaction
 * has:
 * - been aborted - the user gets NULL in the response message parameter 
 * OR
 * - been completed - the response is extracted from the transaction context
 * OR
 * - a signal has arrived, in that case the transaction will be aborted,
 * but there is still a chance that the response has arrived in the mean time
 * and completed the transaction. In that rare case, the abort fails
 * and returns a valid transaction context, which is our response message.
 * The transaction stays marked as complete.
 *
 * The transaction is eventually released.
 * 
 * The case when there is no transaction associated with the message
 * is handled correctly - method terminates right after sending out the message.
 *
 * @param *self - pointer to this message instance
 * @param *sock - KKC socket used for sending message data
 * @param **resp - contains response message or NULL if the transaction
 * is aborted
 * @return 0 upon success
 */
int tcmi_msg_send_and_receive(struct tcmi_msg *self, struct kkc_sock *sock, 
			      struct tcmi_msg **resp)
{
	int err = 0;
	int ret;
	/* increment the reference counter, does nothing when NULL */
	struct tcmi_transaction *trans = self->transaction;
	*resp = NULL;

	/* try sending the data first, quit on error or no transaction */
	err = tcmi_msg_send(self, sock, 0);
	if (!self->transaction || err < 0)
		goto exit0;

	/* wait till the transaction expires */
	ret = tcmi_transaction_wait_interruptible(trans);
	switch (ret) {
	case TCMI_TRANSACTION_ABORTED:
			mdbg(INFO3, "Transaction has been aborted, no response arrived");
		break;

	case TCMI_TRANSACTION_COMPLETE:
			mdbg(INFO3, "Transaction complete, picking up the response");
			*resp = TCMI_MSG(tcmi_transaction_context(trans));
		break;
		/* Aborting a transaction upon a signal arrival must
		 * be atomic, it will fail if the transaction has been
		 * completed in the meantime and thus the method returns a valid
		 * transaction context and transaction stays in 'COMPLETE' state */
	case -ERESTARTSYS:
	default:
		mdbg(INFO3, "Signal arrived, result %d. Check if we abort the transaction..", ret);
		*resp = TCMI_MSG(tcmi_transaction_abort(trans));
		/* communicate the -ERESTARTSYS to the caller */
		err = ret;
		break;
	}

 exit0:
	return err;
}


/** 
 * \<\<public\>\> Receives a message via a specified connection.
 * - reads the transaction ID from the connection
 * - calls the receive method of the child class
 *
 * The transaction ID is temporarily stored in the instance and will
 * be used upon delivery when searching for the corresponding
 * transaction.
 *
 * @param *self - pointer to this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return >=0 upon success.Unlike tcmi_msg_send() we don't convert a
 * >=0 result to 0 as this will never get communicated to the user.
 * The \link tcmi_comm.c::tcmi_comm_thread receiving thread \endlink
 * is the only component that should use this method.
 */
int tcmi_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock)

{
	int err = 0;
	struct tcmi_msg_ops *ops = self->msg_ops;
	if ((err = kkc_sock_rcv_lock_interruptible(sock))) {
		mdbg(ERR3, "Socket lock - interrupted by a signal %d", err);
		goto exit0;
	}
	/* Receive the transaction ID part */	
	if ((err = kkc_sock_recv(sock, &self->trans_id, 
				 sizeof(self->trans_id), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to receive transaction ID's");
		goto exit1;
	}

	mdbg(INFO3, "Receiving message(ID=%x req=%x resp=%x)", 
	     self->msg_id, self->trans_id.req, self->trans_id.resp);
	/* */
	if (ops && ops->recv) 
		err = ops->recv(self, sock);
	kkc_sock_rcv_unlock(sock);
	return err;

	/* error handling */
 exit1:
	kkc_sock_rcv_unlock(sock);
 exit0:
	return err;
}

/** 
 * \<\<public\>\> A message always delivers itself. A delivery of a
 * message depends on the message type. If it is the request part of
 * the message or a one-way message, then the receiving party/thread
 * wants to have it in the associated message queue. If it is a
 * response to a previously sent message and the receiving
 * party/thread expects it, it will get associated with its
 * transaction.
 *
 * - a message is of request type if it has a request transaction
 * ID. In that case, it will be appended to the regular request queue
 * and the queue processing thread will pick it up.
 * - a message is of response type - has a response transaction ID. In
 * that case, it will find its transaction. On success, it will
 * associate itself with the transaction and complete it. Completing
 * the transaction is handled by \link
 * tcmi_transaction.c::tcmi_transaction_complete() the transaction
 * class \endlink internally.
 * - in addition, if the transaction is marked anonymous (nobody is
 * synchronously waiting for the response), the message will also
 * be delivered into the regular message queue.
 *
 * @param *self - pointer to this message instance
 * @param *requests - queue where the message is stored if it is not a
 * response to other message. 
 * @param *transactions - contains all active transactions, the message
 * tries to find its matching transaction.
 * @return 0 upon success
 */
int tcmi_msg_deliver(struct tcmi_msg *self, struct tcmi_queue *requests, 
		     struct tcmi_slotvec *transactions)
{
	int error = 0;
	/* storage for the potential transaction associated with the message */
	struct tcmi_transaction *tran;
	/* request message handling */
	if (self->trans_id.resp == TCMI_TRANSACTION_INVAL_ID) {
		mdbg(INFO3, "Adding message(ID=%x req=%x resp=%x) to the request queue", 
		     self->msg_id, self->trans_id.req, self->trans_id.resp);
		tcmi_queue_add(requests, &self->node);
	}
	/* response message handling */
	else {
		/* search for the transaction */
		if (!(tran = tcmi_transaction_lookup(transactions, self->trans_id.resp))) {
			mdbg(ERR3, "Message(ID=%x req=%x resp=%x) - "
			     "can't find response transaction ID among transactions",
			     self->msg_id, self->trans_id.req, self->trans_id.resp);
			error = -1;
			goto exit0;
		}
		/* store the transaction in the message, reference
		 * counter has been increased by the successful
		 * lookup */
		self->transaction = tran;
		/* try completing it - only message type from the
		* message ID is taken, to verify that the message
		* being delivered matches the message expected by the
		* transaction */
		if (tcmi_transaction_complete(tran, self, TCMI_MSG_TYPE(self->msg_id))) {
			mdbg(ERR3, "Error completing transaction %x", 
			     tcmi_transaction_id(tran));
			error = -EINVAL;
			goto exit0;
		}
		if (tcmi_transaction_is_anon(tran)) {
			mdbg(INFO3, "Detected anonymous transaction, queueing the message too..");
			tcmi_queue_add(requests, &self->node);
		}
		mdbg(INFO3, "Message (ID=%x req=%x resp=%x) delivered, transaction completed", 
		     self->msg_id, self->trans_id.req, self->trans_id.resp);
	}
	
	/* error handling */
 exit0:
	return error;
}

/** 
 * \<\<public\>\> Class method - creates a message based on a
 * descriptor.  Verifies, that the message type in the message ID
 * matches the one in the selected descriptor. Then, based on message
 * ID flags calles the regular rx constructor or error rx constructor.
 * This method is meant to be called by the message factory.
 *
 * @param msg_id - this message ID is for verification
 * @param *dsc - descriptor of a message that is to be built
 * @return new message instance or NULL
 */
struct tcmi_msg* tcmi_msg_new_rx(u_int32_t msg_id, struct tcmi_msg_dsc *dsc)
{
	struct tcmi_msg *msg = NULL;

	if (TCMI_MSG_TYPE(msg_id) != TCMI_MSG_TYPE(dsc->msg_id)) {
		mdbg(ERR3, "Requested message ID type(%x) doesn't match the descriptor(%x)",
		     TCMI_MSG_TYPE(msg_id), TCMI_MSG_TYPE(dsc->msg_id));
		goto exit0;
	}
	/* based on message flags call the regular or error rx constructor */
	switch (TCMI_MSG_FLG(msg_id)) {
	case TCMI_MSG_REGFLGS:
		if (!dsc->new_rx) {
			mdbg(ERR3, "Regular rx message constructor not present, bailing out..");
			goto exit0;
		}
		msg = dsc->new_rx(msg_id);
		break;
	case TCMI_MSG_ERRFLGS:
		if (!dsc->new_rx_err) {
			mdbg(ERR3, "Error rx message constructor not present, bailing out..");
			goto exit0;
		}
		msg = dsc->new_rx_err(msg_id);
		break;
	default:
		mdbg(ERR3, "Unexpected message flags value in message ID %x", msg_id);
	}
	return msg;
	/* error handling */
 exit0:
	return NULL;
}

/** @addtogroup tcmi_msg_class
 *
 * @{
 */

/** 
 * \<\<private\>\> Sends a message via a specified connection.
 * Sending a message requires following steps:
 * - activate the transaction(if any)
 * - send message ID,
 * - send transaction ID(or TCMI_TRANSACTION_INVAL_ID for oneway messages)
 * - call a specific message method
 *
 * It is necessary to lock the socket, so that the message won't get
 * intermixed with some other.
 * 
 * @param *self - pointer to this message instance
 * @param *sock - KKC socket used for sending message data
 * @param flags - if the message has a transaction, these are additional flags to 
 * be set in the transaction.
 * @return 0 upon success
 */
static int tcmi_msg_send(struct tcmi_msg *self, struct kkc_sock *sock, int flags)
{
	int err = 0;
	struct tcmi_msg_ops *ops = self->msg_ops;
	struct tcmi_transaction *trans = self->transaction;

	/** try to start the transaction */
	if (trans && (err = tcmi_transaction_start(trans, flags))) {
		mdbg(ERR3, "Failed to start the transaction %d", err);
		goto exit0;
	}

	/* Send message ID*/
	mdbg(INFO3, "Sending message(Msg=%p ID=%x req=%x resp=%x), transaction started: %d", 
	     self, self->msg_id, self->trans_id.req, self->trans_id.resp, (trans!=NULL));
	if ((err = kkc_sock_snd_lock_interruptible(sock))) {
		mdbg(ERR3, "Socket lock - interrupted by a signal %d", err);
		goto exit0;
	}
	if ((err = kkc_sock_send(sock, &self->msg_id, 
				 sizeof(self->msg_id), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send message ID: %d. Err: %d", self->msg_id, err);
		goto exit1;
	}
	/* Send transaction ID's */
	if ((err = kkc_sock_send(sock, &self->trans_id, 
				 sizeof(self->trans_id), KKC_SOCK_BLOCK)) < 0) {
		mdbg(ERR3, "Failed to send transaction ID's");
		goto exit1;
	}
	/* Send the actual message content */	
	if (ops && ops->send) {
		err = ops->send(self, sock);
	}

	/* convert the result to 0 if no error has occured this is
	 * because the number of bytes sent has no meaning for us anymore */
	err = min(err, 0);
	kkc_sock_snd_unlock(sock);

	return err;
	/* error handling */
 exit1:
	kkc_sock_snd_unlock(sock);
 exit0:
	return err;
}


/**
 * @}
 */
