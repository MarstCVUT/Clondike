/** @file kkc_sock.h - Generic Kernel to Kernelin Communcation abstraction - socket
 *
 * Date: 04/13/2005
 *
 * Author: Jan Capek
 *
 * $Id: kkc_sock.h,v 1.3 2007/10/07 15:53:59 stavam2 Exp $
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

#ifndef _KKC_SOCK_H
#define _KKC_SOCK_H

#include <linux/list.h>
#include <asm/atomic.h>
#include <linux/semaphore.h>

#include "kkc_arch.h"

/** @defgroup kkc_sock_class kkc_sock class
 *
 * @ingroup kkc_class
 *
 * This is a generic KKC socket. It handles basic administration of
 * the generic socket, initialization of common socket parameters, and
 * releasing the socket instance. It is questionable if a regular BSD
 * socket should be included into the compound structure or
 * not. Currently, there is no other communication abstraction in Linux
 * other than sockets. For now, the BSD socket handling functionality
 * will be in child classes (e.g. kkc_sock_tcp). For the common part
 * for all network architectures, we will keep only the address in
 * string form in the kkc_socket super class.
 *
 * KKC socket is meant for full duplex communication. In addition, it
 * provides locking mechanism that allows separate serialization of
 * send and receive requests.
 * 
 * @{
 */

/** Maximum address length */
#define KKC_SOCK_MAX_ADDR_LENGTH 100


/** Flags for receive and accept operations */
typedef enum {
	/* blocking mode operation */
	KKC_SOCK_BLOCK,
	/* non-blocking mode operation */
	KKC_SOCK_NONBLOCK
} kkc_sock_flags_t;

/** A coumpound structure that contains generic information about a KKC
 * socket. */
struct kkc_sock {
	/** architecture for this socket - the reason why we keep the
	 * reference is the release the reference to the architecture,
	 * if the socket has been created based on user string. */
	struct kkc_arch *arch;

	/** local address ('name') of the socket object - temporary
	 * storage.*/
	char sockname[KKC_SOCK_MAX_ADDR_LENGTH];

	/** remote address ('name') of the socket object - temporary
	 * storage. */
	char peername[KKC_SOCK_MAX_ADDR_LENGTH];

	/** message operations */
	struct kkc_sock_ops *sock_ops;

	/** reference counter */
	atomic_t ref_count;

	/** mutex semaphore to serialize receiving from the socket. */
	struct semaphore sock_recv_sem;
	/** mutex semaphore to serialize sending via the socket. */
	struct semaphore sock_send_sem;

	/** general purpose list entry for storing sockets */
	struct list_head pub_list;
};

/** Callback function prototype, that can be registered to be notified about in case there are some data ready to be read on a socket */
typedef void (*kkc_data_ready)(void* data, int bytes);

/** Socket operations that support polymorphism. */
struct kkc_sock_ops {
	/** Connects to the given address. */
	int (*connect)(struct kkc_sock*, char*);
	/** Connects to the given address. */
	int (*listen)(struct kkc_sock*, char*);
	/** Accepts an incoming socket. */
	int (*accept)(struct kkc_sock*, struct kkc_sock**, kkc_sock_flags_t);
	/** Sends out specified data. */
	int (*send)(struct kkc_sock*, void*, int, kkc_sock_flags_t);
	/** Receives requested number of bytes of data. */
	int (*recv)(struct kkc_sock*, void*, int, kkc_sock_flags_t);
	/** Adds a task to the queue of processes waiting on incoming data. */
	int (*add_wait_queue)(struct kkc_sock*, wait_queue_t*);
	/** Removes a task from the queue of processes waiting on incoming data. */
	int (*remove_wait_queue)(struct kkc_sock*, wait_queue_t*);
	/** Registers read callback. */
	int (*register_read_callback)(struct kkc_sock*, kkc_data_ready callback, void* callback_data);
	/** Disconnects the socket. */
	int (*shutdown)(struct kkc_sock*);
	/** Frees socket architecture specific resources. */
	void (*free)(struct kkc_sock*);
	/** Socket name/peer name accessor. */
	int (*getname)(struct kkc_sock *, char *, int, int);
	/** Compare addresses */
	int (*is_address_equal_to)(struct kkc_sock *, const char *, int, int);
};

/** \<\<public\>\> Initializes generic socket. */
extern int kkc_sock_init(struct kkc_sock *self, struct kkc_arch *arch, 
			 struct kkc_sock_ops *sock_ops);
/** \<\<public\>\>  Connects to the given address */
extern int kkc_sock_connect(struct kkc_sock *self, char *addr);
/** \<\<public\>\> Starts listening on a specified address */
extern int kkc_sock_listen(struct kkc_sock *self, char *addr);
/** \<\<public\>\> Creates a new socket by accepting the incoming socket. */
extern int kkc_sock_accept(struct kkc_sock *self, struct kkc_sock **new_kkc_sock, 
			   kkc_sock_flags_t flags);
/** \<\<public\>\> Sends out specified data. */
extern int kkc_sock_send(struct kkc_sock *self, void *buf, int buflen,
			 kkc_sock_flags_t flags);
/** \<\<public\>\> Receives requested number of bytes of data. */
extern int kkc_sock_recv(struct kkc_sock *self, void *buf, int buflen,
			 kkc_sock_flags_t flags);
/** \<\<public\>\> Disconnects the socket. */
extern int kkc_sock_shutdown(struct kkc_sock *self);
/** \<\<public\>\> Adds a task to the queue of processes sleeping on the socket. */
extern int kkc_sock_add_wait_queue(struct kkc_sock *self, wait_queue_t *wait);
/** \<\<public\>\> Removes a task from the queue of processes sleeping on the socket. */
extern int kkc_sock_remove_wait_queue(struct kkc_sock *self, wait_queue_t *wait);

/** \<\<public\>\> Registers data_ready callback function, that should be called when there are some data read to be read on the socket*/
extern int kkc_sock_register_read_callback(struct kkc_sock*, kkc_data_ready callback, void* callback_data);

/** \<\<public\>\> Checks, whether an address passed as string is equal to this socket local/peer port. Can return true only if current socket is connected. */
static inline int kkc_sock_is_address_equal_to(struct kkc_sock *self, const char *addr, int addr_length, int local) {
	int err = 0;
	if (self->sock_ops && self->sock_ops->is_address_equal_to)
		err = self->sock_ops->is_address_equal_to(self, addr, addr_length, local);
	return err;  
}


/**
 * \<\<public\>\> Local socket name accessor.  Asks the architecture specific socket to
 * fill in its local name (address).  The generic socket operation
 * getname() is used, passing 0 selects the local name.
 *
 * @param *self - pointer to this socket instance
 * @param *sockname - pointer to the buffer where the sockname is to be stored
 * @param size - maximum string length that fits into the sockname buffer
 * @return 0 upon success
 */
static inline int kkc_sock_getsockname(struct kkc_sock *self, char *sockname, int size)
{
	int err = 0;
	if (self->sock_ops && self->sock_ops->getname)
		err = self->sock_ops->getname(self, sockname, size, 0);
	return err;
}
/**
 * \<\<public\>\> Peer socket name accessor.  Asks the architecture
 * specific socket to fill in its remote name (destination address).
 * The generic socket operation getname() is used, passing 1 selects
 * the peer name.
 *
 * @param *self - pointer to this socket instance
 * @param *sockname - pointer to the buffer where the sockname is to be stored
 * @param size - maximum string length that fits into the sockname buffer
 * @return 0 upon success
 */
static inline int kkc_sock_getpeername(struct kkc_sock *self, char *sockname, int size)
{
	int err = 0;
	if (self->sock_ops && self->sock_ops->getname)
		err = self->sock_ops->getname(self, sockname, size, 1);
	return err;
}

/**
 * \<\<public\>\> Architecture name accessor.  Asks the architecture
 * to fill in its name.
 *
 * @param *self - pointer to this socket instance
 * @param *archname - pointer to the buffer where the architecture name is to be stored
 * @param size - maximum string length that fits into the sockname buffer
 */
static inline void kkc_sock_getarchname(struct kkc_sock *self, char *archname, int size)
{
	strncpy(archname, kkc_arch_name(self->arch), size);
}

/**
 * \<\<public\>\> Socket name accessor.  Unlike the previous version,
 * returns pointer to the socket local name. The socket local name is
 * stored inside the instance. Having this method makes it easier to
 * use it inside print statements.
 * 
 * @param *self - pointer to this socket instance
 * @return to the local socket name
 */
static inline const char* kkc_sock_getsockname2(struct kkc_sock *self)
{
	memset(self->sockname, 0, KKC_SOCK_MAX_ADDR_LENGTH);
	kkc_sock_getsockname(self, self->sockname, KKC_SOCK_MAX_ADDR_LENGTH);
	return self->sockname;
}
/**
 * \<\<public\>\> Socket name accessor.  Unlike the previous version,
 * returns pointer to the socket local name. The socket local name is
 * stored inside the instance. Having this method makes it easier to
 * use it inside print statements.
 * 
 * @param *self - pointer to this socket instance
 * @return to the local socket name
 */
static inline const char* kkc_sock_getpeername2(struct kkc_sock *self)
{
	memset(self->peername, 0, KKC_SOCK_MAX_ADDR_LENGTH);
	kkc_sock_getpeername(self, self->peername, KKC_SOCK_MAX_ADDR_LENGTH);
	return self->peername;
}

/** 
 * \<\<public\>\> Instance accessor, increments the reference count.
 *
 * @param *self - pointer to this message instance
 */
static inline struct kkc_sock* kkc_sock_get(struct kkc_sock *self)
{
	if (self) {
		atomic_inc(&self->ref_count);
	}
	return self;
}

/** 
 * \<\<public\>\> Decrements reference counter, if it reaches 0 the
 * custom free method is called if defined.  Also the architecture is
 * released.
 *
 * The user is responsible for taking the socket out of any lists
 * where he had stored it.
 *
 * @param *self - pointer to this socket instance
 */
static inline void kkc_sock_put(struct kkc_sock *self)
{
	if (!self)
		return;
	if (atomic_dec_and_test(&self->ref_count)) {
		mdbg(INFO4, "Destroying KKC socket local: '%s', remote: '%s', %p", 
		     kkc_sock_getsockname2(self), 
		     kkc_sock_getpeername2(self), self); 
		if (self->sock_ops && self->sock_ops->free) 
			self->sock_ops->free(self);
		/* release the architecture */
		kkc_arch_put(self->arch);
		
		kfree(self);
	}

}


/**
 * \<\<public\>\> Locks this socket instance for sending,
 * interruptible version, the process can be woken up by a signal.
 *
 * @param *self - pointer to this file instance
 */
static inline int kkc_sock_snd_lock_interruptible(struct kkc_sock *self)
{
	return (down_interruptible(&self->sock_send_sem));
}

/**
 * \<\<public\>\> Locks this socket instance for sending.
 *
 * @param *self - pointer to this socket instance
 */
static inline void kkc_sock_snd_lock(struct kkc_sock *self)
{
		down(&self->sock_send_sem);
}

/**
 * \<\<public\>\> Unlocks this socket instance for sending.
 *
 * @param *self - pointer to this socket instance
 */
static inline void kkc_sock_snd_unlock(struct kkc_sock *self)
{
		up(&self->sock_send_sem);
}

/**
 * \<\<public\>\> Locks this socket instance for receiving,
 * interruptible version, the process can be woken up by a signal.
 *
 * @param *self - pointer to this file instance
 */
static inline int kkc_sock_rcv_lock_interruptible(struct kkc_sock *self)
{
	return (down_interruptible(&self->sock_recv_sem));
}

/**
 * \<\<public\>\> Locks this socket instance for receiving.
 *
 * @param *self - pointer to this socket instance
 */
static inline void kkc_sock_rcv_lock(struct kkc_sock *self)
{
		down(&self->sock_recv_sem);
}

/**
 * \<\<public\>\> Unlocks this socket instance for receiving.
 *
 * @param *self - pointer to this socket instance
 */
static inline void kkc_sock_rcv_unlock(struct kkc_sock *self)
{
		up(&self->sock_recv_sem);
}

/** Casts to the kkc_sock instance. */
#define KKC_SOCK(s) ((struct kkc_sock*)s)

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef KKC_SOCK_PRIVATE

/** Transfers a buffer using the specified method. */
static int kkc_sock_send_recv(struct kkc_sock *self, void *buf, int buflen, 
			      kkc_sock_flags_t flags,
			      int (*kkc_sock_method)(struct kkc_sock *, void *, 
						     int, kkc_sock_flags_t));
#endif /* KKC_SOCK_PRIVATE */


/**
 * @}
 */


#endif /* _KKC_SOCK_H */


