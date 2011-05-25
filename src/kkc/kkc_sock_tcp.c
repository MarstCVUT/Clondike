/** @file kkc_sock_tcp.c - Generic Kernel to Kernelin Communcation abstraction - socket
 *
 * Date: 04/13/2005
 *
 * Author: Jan Capek
 *
 * $Id: kkc_sock_tcp.c,v 1.4 2008/05/02 19:59:20 stavam2 Exp $
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

#include <linux/types.h>
#include <linux/string.h>
#include <linux/socket.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <linux/in.h>


#include "kkc_arch.h"

#define KKC_SOCK_TCP_PRIVATE
#include "kkc_sock_tcp.h"

#include <dbg.h>


/** Declare the TCP architecture */
KKC_ARCH_DECLARE(tcp, &tcp_arch_ops, NULL);

/** @addtogroup kkc_sock_tcp_class
 *
 * @{
 */

/** 
 * \<\<private\>\> Creates an empty TCP socket.  The BSD socket is
 * created for a stream connection using TCP protocol. In addition,
 * socket reuse option is set.  Eventhough a regular TCP socket
 * instance is created it is casted to kkc_arch_obj_t, which is a
 * generic object supported by kkc_arch.  The user of the architecture
 * class instance always knows, what instance to expect, so he/she can
 * cast it to kkc_sock without problems. The reason, why we cannot
 * return directly kkc_sock, is that it would create a circular
 * dependency between \link kkc_arch_class kkc_arch \endlink and \link
 * kkc_sock_class kkc_sock \endlink.
 *
 *
 * @return architecture object instance or NULL
 */
static kkc_arch_obj_t kkc_sock_tcp_new(void)
{
	
	struct kkc_sock_tcp *sock;
	mdbg(INFO4, "Creating a new TCP socket..");

	/* Allocate the instance */
	if (!(sock = KKC_SOCK_TCP(kmalloc(sizeof(struct kkc_sock_tcp), GFP_KERNEL)))) {
		mdbg(ERR4, "Can't allocate memory for TCP socket");
		goto exit0;
	}
	/* Initialize the generic socket */
	if (kkc_sock_init(KKC_SOCK(sock), kkc_arch_get(&KKC_ARCH(tcp)), &tcp_sock_ops)) {
		mdbg(ERR4, "Error initializing TCP socket");
		goto exit1;
	}

	mdbg(INFO4, "Creating BSD socket");
	if (sock_create_kern(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock->sock)) {
		mdbg(ERR4, "Cannot create socket");
		goto exit1;
	}

	// TODO: At the moment, we set this on all TCP connections, but in a future we may want to use this selectively
	kkc_sock_tcp_disable_nagle(KKC_SOCK(sock));
	kkc_sock_tcp_enable_quickack(KKC_SOCK(sock));
	kkc_sock_enable_keepalive(KKC_SOCK(sock));

	/* setup reuse option - this is how it was in original KKC, why? */
	sock->sock->sk->sk_reuse = 1;
	sock->read_callback = NULL;
	sock->sock->sk->sk_user_data = sock; // Back reference to kkc socket
	return KKC_ARCH_OBJ(sock);

	/* error handling */
 exit1:
	kfree(sock);
 exit0:
	return NULL;

}

/** 
 * \<\<private\>\> Connects to the given address. 
 * Stores the address inside the socket and delegates all work
 * to the architecture specific socket.
 *
 * @param *self - pointer to this socket instance
 * @param *addr - address where to connect to.
 * @return 0 upon successful connection
 */
static int kkc_sock_tcp_connect(struct kkc_sock *self, char *addr)
{
	int err = 0;
	struct socket *sock = KKC_SOCK_TCP(self)->sock;
	struct sockaddr_in sin;
	
	/* Extract ip address and port number from addr argument */
	if ((err = kkc_sock_tcp_extract_addr(addr, &sin))) {
		mdbg(ERR4, "Cannot extract ip address from argument '%s'", addr);
		goto exit0;
	}
	mdbg(INFO4, "Connecting to '%s' ..", addr);
	if ((err = sock->ops->connect(sock, (struct sockaddr*)&sin, sizeof(sin), O_RDWR)) < 0) {
		mdbg(ERR4, "Cannot connect to '%s', error=%d", addr, err);
		goto exit0;
	}
	mdbg(INFO4, "Connected to '%s' ..", addr);	
	return 0;
	/* error handling */
 exit0:
	return err;
}


/** 
 * \<\<private\>\> Starts listening on a specified address.
 * Extracts the address from the string, binds the socket
 * to this address and starts listening.
 *
 * @param *self - pointer to this socket instance
 * @param *addr - address where to connect to.
 * @return 0 upon successfully establishing the listening
 */
static int kkc_sock_tcp_listen(struct kkc_sock *self, char *addr)
{
	int err = 0;
	struct socket *sock = KKC_SOCK_TCP(self)->sock;
	struct sockaddr_in sin;



	/* Extract ip address and port number from addr argument */
	if ((err = kkc_sock_tcp_extract_addr(addr, &sin))) {
		mdbg(ERR4, "Cannot extract ip address from argument '%s'", addr);
		goto exit0;
	}
	if ((err = sock->ops->bind(sock, (struct sockaddr*)&sin, sizeof(sin))) < 0) {
		mdbg(ERR4, "Cannot bind to '%s'", addr);
		goto exit0;
	}
	if ((err = sock->ops->listen(sock, 32)) < 0) {
		mdbg(ERR4, "Cannot start listening on '%s'", addr);
		goto exit0;
	}
	mdbg(INFO4, "TCP Listening on '%s' ..", addr);
	return 0;
	/* error handling */
 exit0:
	return err;
}

/** 
 * \<\<private\>\> Creates a new socket by accepting the incoming connection.
 * After instantiating a new socket, a TCP socket specific method is
 * called to accept a connection. This will setup the rest of the newly
 * created TCP socket. 
 *
 * @param *self - pointer to this socket instance
 * @param **new_kkc_sock - storage for the new socket created for the incoming 
 * connection.
 * @param flags - specify blocking/nonblocking mode
 * @return 0 upon successful connection
 * @todo - this needs an optimization, so that a full socket doesn't get 
 * manufactured everytime. This is a performance problem when using nonblock mode
 */
static int kkc_sock_tcp_accept(struct kkc_sock *self, struct kkc_sock **new_kkc_sock, 
			       kkc_sock_flags_t flags)
{
	int err = 0;
	int tmp_flags;
	struct socket *sock, *new_sock;
	/* Create a new TCP socket that will handle the incoming connection */
	if (!(*new_kkc_sock = KKC_SOCK(kkc_sock_tcp_new()))) {
		mdbg(ERR4, "Failed to create a new KKC TCP socket");
		err = -ENOMEM;
		goto exit0;
	}
	/* BSD socket where we check for incoming connections */
	sock = KKC_SOCK_TCP(self)->sock;
	/* newly created BSD socket that handles the incoming connection */
	new_sock = KKC_SOCK_TCP(*new_kkc_sock)->sock;

	/* compose flags for accepting */
	tmp_flags = O_RDWR | ((flags & KKC_SOCK_NONBLOCK) != 0 ? O_NONBLOCK : 0);

	mdbg(INFO4, "Accepting incoming connection");
	if ((err = sock->ops->accept(sock, new_sock, tmp_flags)) < 0) {
		if ( err != -EAGAIN ) {
			mdbg(ERR4, "Cannot accept new connection %d", err);
		} else {
			mdbg(INFO4, "Accept timeouted.");
		}
		goto exit1;
	}

	mdbg(INFO4, "Accepted a TCP connection..");

	return 0;
	/* error handling */
 exit1:
	kkc_sock_put(*new_kkc_sock);
	new_kkc_sock = NULL; /* user will get back NULL */
 exit0:
	return err;
}

/** 
 * \<\<private\>\> Sends out specified data. 
 * What needs to be done:
 * - setup a message header
 * - setup iovec and store the buffer base address and length into it
 * - asks the kernel to receive the message
 *
 * @param *self - pointer to this socket instance
 * @param *buf - buffer with the data to be sent
 * @param buflen - length of the buffer to be sent
 * @param flags - specify blocking or non-blocking mode.
 * @return number of bytes actually sent or error when < 0
 */
static int kkc_sock_tcp_send(struct kkc_sock *self, void *buf, int buflen, 
			     kkc_sock_flags_t flags)
{
	struct socket *sock;
	struct msghdr msg;
	struct iovec iov;
	int result = 0;
	
	mdbg(INFO4, "TCP Sending buffer %p length: %d bytes", buf, buflen);
	sock = KKC_SOCK_TCP(self)->sock;

	/* Setup message header */
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;	/* blocking mode do we need to set
				 * this, it should be default for
				 * sending??*/

	iov.iov_base = buf;
	iov.iov_len = buflen;

	result = kernel_sendmsg(sock, &msg,(struct kvec*)&iov, 1, buflen);

	mdbg(INFO4, "TCP sending buffer %p length: %d bytes, sent %d", buf, buflen, result);	


	return result;
}


/** 
 * \<\<private\>\> Receives requested number of bytes of data. 
 * What needs to be done:
 * - setup a message header
 * - setup iovec and store the buffer base address and length into it
 * - asks the kernel to receive the message
 *
 * @param *self - pointer to this socket instance
 * @param *buf - buffer where to store received data.
 * @param buflen - length of the buffer to be sent
 * @param flags - specify blocking or non-blocking mode.
 * @return number of bytes actually received or error when < 0
 */
static int kkc_sock_tcp_recv(struct kkc_sock *self, void *buf, int buflen,
			     kkc_sock_flags_t flags)
{
	struct socket *sock = KKC_SOCK_TCP(self)->sock;
	struct msghdr msg;
	struct iovec iov;
	int tmp_flags;
	int result = 0;

	/* Setup message header */
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	tmp_flags = (flags & KKC_SOCK_NONBLOCK ? MSG_DONTWAIT : 0);

	iov.iov_base = buf;
	iov.iov_len = buflen;

	result = kernel_recvmsg(sock, &msg, (struct kvec*)&iov, 1, buflen, tmp_flags);

	mdbg(INFO4, "TCP receiving buffer %p length: %d bytes, received %d", buf, buflen, result);	


	/* connection reset is marked as 0, so if the user requested
	 * more than 0bytes to be sent, we have to check for this and
	 * convert the error */
	return ((result == 0) && (buflen > 0)) ? -ECONNRESET : result;
}


/** 
 * \<\<private\>\> Disconnects the socket.  Connection is shutdown in
 * both directions, it might be a good idea, to add the option for the
 * user to specify which part of the connection is to be closed.
 *
 * @param *self - pointer to this socket instance
 * @return 0 upon success
 */
static int kkc_sock_tcp_shutdown(struct kkc_sock *self)
{
	int err = 0;

	struct socket *sock = KKC_SOCK_TCP(self)->sock;

	mdbg(INFO4, "TCP disconnecting..");
	err = sock->ops->shutdown(sock, SEND_SHUTDOWN | RCV_SHUTDOWN);

	return err;
}

/** 
 * \<\<private\>\> Adds a task to the queue of processes sleeping on
 * this socket. We use the sk_sleep wait queue, no locking needed
 * waitqueues handle serialization on their own.
 *
 * @param *self - pointer to this socket instance
 * @param *wait - pointer to the waitqueue element with the task that
 * wants to sleep on this socket.
 * @return 0 upon success
 */
static int kkc_sock_tcp_add_wait_queue(struct kkc_sock *self, wait_queue_t *wait)
{
	int err = 0;

	mdbg(INFO4, "TCP adding to wait queue..");
	/* add to socket wait queue */
	add_wait_queue(KKC_SOCK_TCP(self)->sock->sk->sk_sleep, wait);

	return err;
}

/** 
 * \<\<private\>\> Removes a task from the queue of processes sleeping
 * on this socket.
 *
 * @param *self - pointer to this socket instance
 * @param *wait - pointer to the waitqueue element with the task that
 * wants to sleep on this socket.
 * @return 0 upon success
 */
static int kkc_sock_tcp_remove_wait_queue(struct kkc_sock *self, wait_queue_t *wait)
{
	int err = 0;

	mdbg(INFO4, "TCP removing from wait queue..");
	/* remove from socket wait queue */
	remove_wait_queue(KKC_SOCK_TCP(self)->sock->sk->sk_sleep, wait);

	return err;
}

/** 
 * \<\<private\>\> Method that will be registered to standard sock as a callback function and will dispatch the request
 * to our registered callback
 */
static void kkc_sock_tcp_callback(struct sock *sk, int count) {
	if ( sk->sk_user_data ) {
		struct kkc_sock* self = sk->sk_user_data;
		if ( KKC_SOCK_TCP(self)->read_callback ) {
			KKC_SOCK_TCP(self)->read_callback(KKC_SOCK_TCP(self)->callback_data, count);
		}
	}
}

/** 
 * \<\<private\>\> Registeres callback function that will be called when there are
 * some data to be read in the socket
 *
 * @param *self - pointer to this socket instance
 * @param *wait - pointer to the callback function
 * @return 0 upon success
 */
static int kkc_sock_tcp_register_read_callback(struct kkc_sock *self, kkc_data_ready callback, void* callback_data)
{
	int err = 0;

	mdbg(INFO4, "TCP registering read callback..");
	KKC_SOCK_TCP(self)->read_callback = callback;
	KKC_SOCK_TCP(self)->callback_data = callback_data;
	KKC_SOCK_TCP(self)->sock->sk->sk_data_ready = kkc_sock_tcp_callback;

	return err;
}

/** 
 * \<\<private\>\> Free TCP socket related resources.  Essentially, it
 * terminates the connection and releases the socket.
 * This method is called by the super class when the last reference
 * to the socket is dropped.
 *
 * @param *self - pointer to this socket instance
 */
static void kkc_sock_tcp_free(struct kkc_sock *self)
{

	mdbg(INFO4, "TCP freeing socket resources..");
	kkc_sock_tcp_shutdown(self);
	sock_release(KKC_SOCK_TCP(self)->sock);
}

/**
 * \<\<private\>\> Socket name accessor.  The socket name is returned in format
 * 'a.b.c.d:p' We use a socket operation to retrieve the desired part
 * of the address(local or peer). The address is then formated as
 * mentioned above and stored in the buffer.
 *
 * @param *self - pointer to this socket instance
 * @param *name - pointer to the buffer where the sockname is to be stored
 * @param size - maximum string length that fits into the sockname buffer
 * @param local - if set to 0, retrieves the local name, otherwise retreives
 * the remote(peer) name
 * @return 0 upon success
 */
static int kkc_sock_tcp_getname(struct kkc_sock *self, char *name, int size, int local)
{
	int err = 0;
	int len = sizeof(struct sockaddr_in);
	struct socket *sock = KKC_SOCK_TCP(self)->sock;
	struct sockaddr_in address;

	/* used to remap the s_addr for byte access */
	unsigned char *paddr;
	if ((err = sock->ops->getname(sock, (struct sockaddr *)&address, 
				      &len, local)) < 0) {
		goto exit0;
	}
	paddr = (char *) &address.sin_addr.s_addr;
	
	snprintf(name, size, "%u.%u.%u.%u:%u", paddr[0], paddr[1], paddr[2], paddr[3],
		 ntohs(address.sin_port));
	
	return 0;
	/* error handling */
 exit0:
	memset(name, 0, size);
	return err;
}

/**
 * \<\<private\>\> IP Comparator
 *
 * @param *address - string with the address in the form a.b.c.d:p
 * @param *addr_length
 * @param *local - 1 to compare with local IP, 0 if to compare with peer IP
 * @return 1, if address equals to local or peer addres (depending on local flag)
 */
static int kkc_sock_tcp_is_address_equal_to(struct kkc_sock *self, const char* addr, int addr_length, int local) {
	int err = 0;
	int len = sizeof(struct sockaddr_in);
	struct socket *sock = KKC_SOCK_TCP(self)->sock;
	struct sockaddr_in address;
	unsigned char *paddr;
	char name[KKC_SOCK_MAX_ADDR_LENGTH];
	
	if ( !addr )
	    return 0;	

	/* used to remap the s_addr for byte access */	
	if ((err = sock->ops->getname(sock, (struct sockaddr *)&address, 
				      &len, local)) < 0) {
		goto exit0;
	}
	paddr = (char *) &address.sin_addr.s_addr;	
	snprintf(name, KKC_SOCK_MAX_ADDR_LENGTH, "tcp:%u.%u.%u.%u:%u", paddr[0], paddr[1], paddr[2], paddr[3],
		 ntohs(address.sin_port));
	
//	printk("Comparing: %s with %s local %d, %d, %d\n", name, addr, local, strncmp(addr, name, min((size_t)addr_length, strlen(name))), strcmp(addr, name));
			
	return strncmp(addr, name, min((size_t)addr_length, strlen(name))) == 0;
	/* error handling */
 exit0:
	return 0;  
}

/**
 * \<\<private\>\> Helper class method for extracting ip address and port
 * number from string argument.
 *
 * @param *addr - string with the address in the form a.b.c.d:p
 * @param *sin - output parameter that will have ip address and 
 * port filled out upon success
 * @return 0 if a valid IP address and port have been extracted
 */
static int kkc_sock_tcp_extract_addr(char *addr, struct sockaddr_in *sin)
{
	char *colon_ptr;
	int colon_pos;
	char tmp_ip[16];
	unsigned long int tmp_port;	
	char *dummy;
	int err = 0;

	/* find colon in address */
	if ((colon_ptr = strchr(addr, ':')) == NULL) {
		mdbg(ERR4, "Invalid IP address syntax");
		err = -EINVAL;
		goto leave;
	}
		
	/*
	 * Extract ip address 
	 */
	colon_pos = colon_ptr - addr;
	if (colon_pos > 15) {
		mdbg(ERR4, "Invalid IP address syntax");
		err = -EINVAL;
		goto leave;
	}
	strncpy(tmp_ip, addr, colon_pos);
	tmp_ip[colon_pos] = '\0';
	mdbg(INFO4, "Extracted ip address is '%s'", tmp_ip);

	/* extract port */
	tmp_port = simple_strtoul(colon_ptr + 1, &dummy, 10);
	mdbg(INFO4, "Extracted port number is '%lu'", tmp_port);

	/* fill results */
	
	sin->sin_addr.s_addr = in_aton(tmp_ip);
	
	sin->sin_port = htons((unsigned short int)tmp_port);
	sin->sin_family = AF_INET;

leave:
	return err;
}

/** TCP architecture specific operations. */
static struct kkc_arch_ops tcp_arch_ops = {
	.kkc_arch_obj_new = kkc_sock_tcp_new
};

/** Socket operations that support polymorphism. */
static struct kkc_sock_ops tcp_sock_ops = {
	.connect = kkc_sock_tcp_connect,
	.listen  = kkc_sock_tcp_listen,
	.accept  = kkc_sock_tcp_accept,
	.send    = kkc_sock_tcp_send,
	.recv    = kkc_sock_tcp_recv,
	.shutdown = kkc_sock_tcp_shutdown,
	.add_wait_queue = kkc_sock_tcp_add_wait_queue,
	.remove_wait_queue = kkc_sock_tcp_remove_wait_queue,
	.register_read_callback = kkc_sock_tcp_register_read_callback,
	.free = kkc_sock_tcp_free,
	.getname = kkc_sock_tcp_getname,
	.is_address_equal_to = kkc_sock_tcp_is_address_equal_to
};


/** Method used for enabling of keep alive on socket and setting of aggressive timing of timeout detection */
int kkc_sock_enable_keepalive(struct kkc_sock* self) {
	struct kkc_sock_tcp* kkc_tcp = KKC_SOCK_TCP(self);
	struct socket* socket = kkc_tcp->sock;
	int val = 1;
	int ret;

	// Note: must be done via those calls, direct setting does not update timer values!

	val = 10; // Start tracking timeout after 10 seconds
	ret = kernel_setsockopt(socket, SOL_TCP, TCP_KEEPIDLE, (char __user *) &val, sizeof(val));
	if ( ret ) minfo(ERR4, "Failed to set keep idle flag");

	val = 1; // Check tracking message every second after first timeout
	ret = kernel_setsockopt(socket, SOL_TCP, TCP_KEEPINTVL, (char __user *) &val, sizeof(val));
	if ( ret ) minfo(ERR4, "Failed to set keep intl flag");

	val = 10; // After 10 missed packets, mark the connection as dead
	ret = kernel_setsockopt(socket, SOL_TCP, TCP_KEEPCNT, (char __user *) &val, sizeof(val));
	if ( ret ) minfo(ERR4, "Failed to set keep cnt flag");	 

	ret = kernel_setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE,
			(char *)&val, sizeof(val));
	if (ret < 0) minfo(ERR4, "Failed to set keep-alive");


	return ret;
}



/** Method used for disabling of nagels algorithm on a tcp connection */
int kkc_sock_tcp_disable_nagle(struct kkc_sock* self) {
	// COPIED FORM tcp.c
	struct kkc_sock_tcp* kkc_tcp = KKC_SOCK_TCP(self);
	struct socket* tcp_socket = kkc_tcp->sock;
	struct sock* sock = tcp_socket->sk;
 	struct tcp_sock *tp = tcp_sk(sock);
        //struct inet_connection_sock *icsk = inet_csk(sock);

	tp->nonagle |= TCP_NAGLE_OFF|TCP_NAGLE_PUSH;
	// TODO: This is not exported... but we likely do not need that as long as we disable nagle before any transmissions, right?
        //tcp_push_pending_frames(sock);	

	return 0;
}

/** Method used for enabling quick-ack on a tcp connection */
int kkc_sock_tcp_enable_quickack(struct kkc_sock* self) {
	// COPIED FORM tcp.c
	struct kkc_sock_tcp* kkc_tcp = KKC_SOCK_TCP(self);
	struct socket* tcp_socket = kkc_tcp->sock;
	struct sock* sock = tcp_socket->sk;
        struct inet_connection_sock *icsk = inet_csk(sock);

	// TODO: Can be called safely only before the connection is established
	// Otherwise we shall copy other bits from tcp.c
	icsk->icsk_ack.pingpong = 0;
	return 0;
}

/**
 * @}
 */
