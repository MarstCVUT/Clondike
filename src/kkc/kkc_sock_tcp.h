/** @file kkc_sock_tcp.h - Generic Kernel to Kernelin Communcation abstraction - socket
 *
 * Date: 04/13/2005
 *
 * Author: Jan Capek
 *
 * $Id: kkc_sock_tcp.h,v 1.4 2008/05/02 19:59:20 stavam2 Exp $
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

#ifndef _KKC_SOCK_TCP_H
#define _KKC_SOCK_TCP_H

#include <linux/inet.h>

#include "kkc_sock.h"

/** @defgroup kkc_sock_tcp_class kkc_sock_tcp class
 *
 * @ingroup kkc_sock_class
 *
 * This class is a TCP socket. The functionality is implemented via
 * BSD socket abstraction.
 * 
 * @{
 */

/** A coumpound structure that contains TCP socket specific information. */
struct kkc_sock_tcp {
	/** parent class instance */
	struct kkc_sock super;
	/** BSD socket abstraction. */
	struct socket *sock;
	/** Read callback function.. TODO: Perhaps move this to sock directly? */
	kkc_data_ready read_callback;
	/** Data provided to the callback function. The socket is NOT owner of these data! */
	void* callback_data;
};

/** Casts to the kkc_sock instance. */
#define KKC_SOCK_TCP(s) ((struct kkc_sock_tcp*)s)

/** Method used for disabling of nagels algorithm on a tcp connection */
int kkc_sock_tcp_disable_nagle(struct kkc_sock* self);
/** Method used for enabling quick-ack on a tcp connection */
int kkc_sock_tcp_enable_quickack(struct kkc_sock* self);
/** Method used for enabling of keep alive on socket */
int kkc_sock_enable_keepalive(struct kkc_sock* self);

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef KKC_SOCK_TCP_PRIVATE

/** Architecture specific constructor. */
static kkc_arch_obj_t kkc_sock_tcp_new(void);
/** Connects to the given address */
static int kkc_sock_tcp_connect(struct kkc_sock *self, char *addr);
/** Starts listening on a specified address */
static int kkc_sock_tcp_listen(struct kkc_sock *self, char *addr);
/** Creates a new socket by accepting the incoming socket. */
static int kkc_sock_tcp_accept(struct kkc_sock *self, struct kkc_sock **new_kkc_sock, 
			       kkc_sock_flags_t flags);
/** Sends out specified data. */
static int kkc_sock_tcp_send(struct kkc_sock *self, void *buf, int buflen,
			     kkc_sock_flags_t flags);
/** Receives requested number of bytes of data. */
static int kkc_sock_tcp_recv(struct kkc_sock *self, void *buf, int buflen,
			     kkc_sock_flags_t flags);
/** Disconnects the socket. */
static int kkc_sock_tcp_shutdown(struct kkc_sock *self);
/** Adds a task to the queue of processes waiting sleeping on the socket. */
static int kkc_sock_tcp_add_wait_queue(struct kkc_sock *self, wait_queue_t *wait);
/** Removes a task from the queue of processes waiting sleeping on the socket. */
static int kkc_sock_tcp_remove_wait_queue(struct kkc_sock *self, wait_queue_t *wait);
/** Free socket related resources */
static void kkc_sock_tcp_free(struct kkc_sock *self);
/** Socket name accessor. */
static int kkc_sock_tcp_getname(struct kkc_sock *self, char *name, int size, int local);
/** Address comparator */
static int kkc_sock_tcp_is_address_equal_to(struct kkc_sock *self, const char* address, int addr_length, int local);

/** Transfers a buffer using the specified method. */
/*static int kkc_sock_tcp_send_recv(struct kkc_sock *self, void *buf, int buflen, 
				  int (*kkc_sock_tcp_method)(struct kkc_sock_tcp *, void *, 
							     int, kkc_tcp_flags_t));
*/

/** Helper class method for extracting ip address and port number from
 * string argument. */
static int kkc_sock_tcp_extract_addr(char *addr, struct sockaddr_in *sin);

/** TCP architecture specific operations. */
static struct kkc_arch_ops tcp_arch_ops; 

/** TCP Socket specific operations. */
static struct kkc_sock_ops tcp_sock_ops; 

#endif /* KKC_SOCK_TCP_PRIVATE */


/**
 * @}
 */


#endif /* _KKC_SOCK_TCP_H */


