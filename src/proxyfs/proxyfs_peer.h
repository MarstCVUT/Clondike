/**
 * @file proxyfs_peer.h - Represents connection between client and server.
 *                      
 * 
 *
 *
 * Date: 08/12/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_peer.h,v 1.4 2008/01/17 14:36:44 stavam2 Exp $
 *
 * This file is part of Proxy filesystem (ProxyFS)
 * Copyleft (C) 2007  Petr Malat
 * 
 * ProxyFS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * ProxyFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef _PROXYFS_PEER_H
#define _PROXYFS_PEER_H

#include <linux/list.h>
#include <asm/atomic.h>
#include <linux/spinlock.h>

#include <dbg.h>
#include <kkc/kkc.h>
#include <proxyfs/proxyfs_msg.h>

/** @defgroup proxyfs_peer_class proxyfs_peer class
 *
 * @ingroup proxyfs_module_class
 *
 * This class represents connection between two peers
 * allows sending and receiving messages
 * 
 * @{
 */

typedef enum {
	PEER_CREATED,
	PEER_CONNECTED,
	PEER_DISCONNECTED,
	PEER_DEAD,
} proxyfs_peer_state_t;


/** Structure representing peer connection */
struct proxyfs_peer_t {
	/** Comunication socket */
	struct kkc_sock *sock;

	/** Buffer for receiving */
	void   *recv_buf;
	/** Receiving offset to rec_buf */
	size_t recv_start;

	/** Queue of messages */ 
	struct list_head msg_queue;

	/** Peers list */
	struct list_head peers;

	/** For waiting on this socket */
	wait_queue_t  socket_wait;
	/** Lock guarding message enqueueing */
	spinlock_t enqueue_msg_lock;

	/** reference counter */
	atomic_t ref_count;
	/** State of the peer */
	proxyfs_peer_state_t state;
};

/** Cast to struct proxyfs_peer_t * */
#define PROXYFS_PEER(arg) ((struct proxyfs_peer_t *)arg)

/** \<\<public\>\> Create proxyfs_peer instance */
struct proxyfs_peer_t *proxyfs_peer_new(void);

/** \<\<public\>\> Initialize proxyfs_peer instance */
int proxyfs_peer_init(struct proxyfs_peer_t *self);

/** \<\<public\>\> Gets proxyfs peers reference */
static inline struct proxyfs_peer_t* proxyfs_peer_get(struct proxyfs_peer_t *self) {
	if (self) {
		atomic_inc(&self->ref_count);
	}
	return self;	
};

/** \<\<public\>\> Puts proxyfs peers reference */
void proxyfs_peer_put(struct proxyfs_peer_t *self);

/** \<\<public\>\> Nonblocking send queued messages */
void proxyfs_peer_real_send(struct proxyfs_peer_t *self);

/** \<\<public\>\> Nonblocking recv messages  */
int proxyfs_peer_real_recv(struct proxyfs_peer_t *self);

/** \<\<public\>\> Getter of peer state */
static inline proxyfs_peer_state_t proxyfs_peer_get_state(struct proxyfs_peer_t *self) {
	return self->state;
};

/** \<\<public\>\> Setter of peer state */
static inline void proxyfs_peer_set_state(struct proxyfs_peer_t *self, proxyfs_peer_state_t state) {
	self->state = state;
};

/** \<\<public\>\> Delete received message, and prepare to receiving new one
 * @param self - pointer to proxyfs_peer_t instance 
 * @return Zero on success
 */
static inline int proxyfs_peer_clear_msg(struct proxyfs_peer_t *self)
{
	if( self->recv_start >=  MSG_HDR_SIZE &&
			proxyfs_msg_get_size( self->recv_buf ) == self->recv_start){
		self->recv_start = 0;
		return 0;
	}
	else
	{
		mdbg(ERR2, "Attempt to clear incomplete message");
		return -1;
	}
}

/** \<\<public\>\> Recv message  
 * @param self - pointer to proxyfs_peer_t instance 
 * @return Pointer to received message or NULL if there is none
 */
static inline struct proxyfs_msg *proxyfs_peer_get_msg(struct proxyfs_peer_t *self)
{
	if( self->recv_start >=  MSG_HDR_SIZE &&
			proxyfs_msg_get_size( self->recv_buf ) == self->recv_start)
		return (struct proxyfs_msg *)self->recv_buf;
	else{
		mdbg(INFO4, "Attempt to get incomplete message");
		return NULL;
	}
}

/** \<\<public\>\> Add message to sending queue
 * @param self - pointer to proxyfs_peer_t instance 
 * @param msg  - pointer to msg, which will be added to queue
 */
static inline void proxyfs_peer_send_msg(struct proxyfs_peer_t *self, struct proxyfs_msg *msg)
{
	mdbg(INFO4, "Adding msg %p to a send queue of peer %p", msg, self);
	spin_lock(&self->enqueue_msg_lock);
	list_add_tail( & msg->msg_queue, & self->msg_queue );
	spin_unlock(&self->enqueue_msg_lock);
	mdbg(INFO3, "Queued message %lu for file %lu", msg->header.msg_num, msg->header.file_ident);
}

/** \<\<public\>\> Connects to peer */
int proxyfs_peer_connect(struct proxyfs_peer_t *self, const char *connect_str);

/** \<\<public\>\> Disconnects  peer */
int proxyfs_peer_disconnect(struct proxyfs_peer_t *self);

/** \<\<public> Wait on till peer is connected */
int proxyfs_peer_wait(struct proxyfs_peer_t *self);

/**
 * @}
 */

#endif // _PROXYFS_PEER_H
