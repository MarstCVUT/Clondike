/** @file kkc_sock.c - Generic Kernel to Kernelin Communcation abstraction - socket
 *
 * Date: 04/13/2005
 *
 * Author: Jan Capek
 *
 * $Id: kkc_sock.c,v 1.3 2007/10/07 15:53:59 stavam2 Exp $
 *
 * This file is part of Kernel-to-Kernel Communication Library(KKC)
 * Copyleft (C) 2005  Jan Capek
 * 
 * KKC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * KKC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <linux/errno.h>

#define KKC_SOCK_PRIVATE
#include "kkc_sock.h"

#include <dbg.h>


/** 
 * \<\<public\>\> Initializes generic socket.  This method should be
 * called back from the architecture specific constructor to
 * initialize common socket fields.  Architecture is set.
 *
 * @param *self - pointer to this socket instance
 * @param *sock_ops - operations vector of the architecture specific socket
 * @param *arch - socket specific architecture. This allows the user
 * to read architecture name.
 * 
 * @return 0 upon successful connection
 */
int kkc_sock_init(struct kkc_sock *self, struct kkc_arch *arch, 
		  struct kkc_sock_ops *sock_ops)
{
	if (!arch) {
		mdbg(ERR4, "No architecture specified, can't init socket");
		return -EINVAL;
	}
	self->arch = arch;
	self->sock_ops = sock_ops;
	atomic_set(&self->ref_count, 1);
	init_MUTEX(&self->sock_send_sem);
	init_MUTEX(&self->sock_recv_sem);
	INIT_LIST_HEAD(&self->pub_list);

	mdbg(INFO4, "Initialized KKC socket: %p", self); 

	return 0;
}

/** 
 * \<\<public\>\> Connects to the given address.  Delegates all work
 * to the architecture specific socket operation.
 *
 * @param *self - pointer to this socket instance
 * @param *addr - address where to connect to.
 * @return 0 upon successful connection
 */
int kkc_sock_connect(struct kkc_sock *self, char *addr)
{
	int error = 0;
	mdbg(INFO4, "Connecting to '%s' ..", addr);
	if (self->sock_ops && self->sock_ops->connect)
		error = self->sock_ops->connect(self, addr);

	return error;
}


/** 
 * \<\<public\>\> Starts listening on a specified address. Delegates
 * all work to the architecture specific socket operation.
 *
 * @param *self - pointer to this socket instance
 * @param *addr - address where to connect to.
 * @return 0 upon successfully establishing the listening
 */
int kkc_sock_listen(struct kkc_sock *self, char *addr)
{
	int error = 0;
	mdbg(INFO4, "Listening on '%s' ..", addr);
	if (self->sock_ops && self->sock_ops->listen)
		error = self->sock_ops->listen(self, addr);

	return error;
}

/** 
 * \<\<public\>\> Creates a new socket by accepting the incoming
 * connection.  Delegates work to the architecture specific class.
 *
 * @param *self - pointer to this socket instance
 * @param **new_kkc_sock - storage for the new socket created for the incoming 
 * connection.
 * @param flags - specify blocking/nonblocking mode
 * @return 0 upon successful connection
 */
int kkc_sock_accept(struct kkc_sock *self, struct kkc_sock **new_kkc_sock, 
		    kkc_sock_flags_t flags)
{
	int error = 0;
	mdbg(INFO4, "Accepting connection..");
	if (self->sock_ops && self->sock_ops->accept)
		error = self->sock_ops->accept(self, new_kkc_sock, flags);
	return error;
}

/** 
 * \<\<public\>\> Sends out specified data.  Sends the entire content
 * of the specified buffer. This might require multiple iterations -
 * the method will try until all data is sent or an error occurs.
 *
 * @param *self - pointer to this socket instance
 * @param *buf - buffer with the data to be sent
 * @param buflen - length of the buffer to be sent
 * @param *buflen - number of bytes actually sent
 * @param flags - specify blocking or non-blocking mode. 
 * @return number of bytes actually sent or error when < 0
 */
int kkc_sock_send(struct kkc_sock *self, void *buf, int buflen,
		  kkc_sock_flags_t flags)
{
	int result = 0;
	mdbg(INFO4, "Sending buffer %p length: %d bytes", buf, buflen);
	if (self->sock_ops && self->sock_ops->send) {
		result = kkc_sock_send_recv(self, buf, buflen, flags, 
					    self->sock_ops->send);
	}

	return result;
}


/** 
 * \<\<public\>\> Receives requested number of bytes of data.  The
 * behavior of the function depends on the mode. In blocking mode, it
 * will try to receive all buflen bytes via kkc_sock_send_recv(). In
 * non-blocking mode it will perform only 1 attempt to receive data,
 * using the receive operation of the socket instance directly.
 *
 * @param *self - pointer to this socket instance
 * @param *buf - buffer where to store received data.
 * @param buflen - length of the buffer to be sent
 * @param flags - specify blocking or non-blocking mode.
 * @return number of bytes actually received or error when < 0
 */
int kkc_sock_recv(struct kkc_sock *self, void *buf, int buflen,
		  kkc_sock_flags_t flags)
{
	int result = 0;
	mdbg(INFO4, "Receiving into buffer %p length: %d bytes", buf, buflen);
	if (self->sock_ops && self->sock_ops->recv) {
		if (flags == KKC_SOCK_BLOCK)
			result = kkc_sock_send_recv(self, buf, buflen, flags,
						    self->sock_ops->recv); 
		else
			result = self->sock_ops->recv(self, buf, buflen, flags);
			
	}

	return result;
}


/** 
 * \<\<public\>\> Disconnects the socket.  Delegates work to the
 * specific socket class.
 *
 * @param *self - pointer to this socket instance
 * @return 0 upon success
 */
int kkc_sock_shutdown(struct kkc_sock *self)
{
	int error = 0;
	if (self->sock_ops && self->sock_ops->shutdown) {
		error = self->sock_ops->shutdown(self);

	}

	return error;
}

/** 
 * \<\<public\>\> Adds a task to the queue of processes sleeping on
 * the socket.  Delegates work to the specific socket class.
 *
 * @param *self - pointer to this socket instance
 * @param *wait - pointer to the waitqueue element with the task that
 * wants to sleep on this socket.
 * @return 0 upon success
 */
int kkc_sock_add_wait_queue(struct kkc_sock *self, wait_queue_t *wait)
{
	int error = 0;
	if (self->sock_ops && self->sock_ops->add_wait_queue)
		error = self->sock_ops->add_wait_queue(self, wait);

	return error;
}

/** 
 * \<\<public\>\> Removes a task from the queue of processes sleeping
 * on the socket.  Delegates work to the specific socket class.
 *
 * @param *self - pointer to this socket instance
 * @param *wait - pointer to the waitqueue element with the task that
 * wants to sleep on this socket.
 * @return 0 upon success
 */
int kkc_sock_remove_wait_queue(struct kkc_sock *self, wait_queue_t *wait)
{
	int error = 0;
	if (self->sock_ops && self->sock_ops->remove_wait_queue)
		error = self->sock_ops->remove_wait_queue(self, wait);

	return error;
}

/** \<\<public\>\> Registers data_ready callback function, that should be called when there are some data 
 * read to be read on the socket
 */
int kkc_sock_register_read_callback(struct kkc_sock* self, kkc_data_ready callback, void* callback_data) {
	int error = 0;
	if (self->sock_ops && self->sock_ops->register_read_callback)
		error = self->sock_ops->register_read_callback(self, callback, callback_data);

	return error;
};


/** @addtogroup kkc_sock_class
 *
 * @{
 */

/**
 * \<\<private\>\> Transfers a required number of bytes. This method
 * is intended for receiving and sending in blocking mode.  The caller
 * specifies only which method should be used. The method tries to
 * transfer all the requested bytes until an error occurs.
 *
 * @param *self - pointer to this socket instance
 * @param *buf - buffer for transferred data (received or sent)
 * @param buflen - length of the buffer to be sent/received
 * @param flags - specify blocking or non-blocking mode.
 * @param *kkc_sock_method - send or receive method to be executed
 * @return number of bytes actually sent or received or error when < 0
 */
static int kkc_sock_send_recv(struct kkc_sock *self, void *buf, int buflen, 
			      kkc_sock_flags_t flags,
			      int (*kkc_sock_method)(struct kkc_sock *, void *, 
						     int, kkc_sock_flags_t))
{
	int error = 0;
	int total = 0;
	while ((buflen > 0) && (error >= 0)) {
		error = kkc_sock_method(self, buf, buflen, flags);
		mdbg(INFO4, "Buffer starting at %p, xferred %d bytes", buf, error);
		buf += error;
		buflen -= error;
		total += error;
	}
	/* When error occured, return error code, otherwise the total */
	return  ((error >= 0) ? total : error);
}

/**
 * @}
 */
/* connect and listen are not exported as sockets should be created via
 * kkc_connect/kkc_listen that allow specifying a proper architecture
 * in the address string */
EXPORT_SYMBOL_GPL(kkc_sock_accept);
EXPORT_SYMBOL_GPL(kkc_sock_send);
EXPORT_SYMBOL_GPL(kkc_sock_recv);
EXPORT_SYMBOL_GPL(kkc_sock_shutdown);
EXPORT_SYMBOL_GPL(kkc_sock_add_wait_queue);
EXPORT_SYMBOL_GPL(kkc_sock_remove_wait_queue);
EXPORT_SYMBOL_GPL(kkc_sock_register_read_callback);
