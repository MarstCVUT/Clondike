/**
 * @file proxyfs_peer.c - Represents connection between client and server.
 *                      
 * 
 *
 *
 * Date: 08/12/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_peer.c,v 1.8 2009-04-06 21:48:46 stavam2 Exp $
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
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <kkc/kkc.h>

#define PROXYFS_PEER_PRIVATE
#include "proxyfs_peer.h"

#define USE_VMALLOC ( 4 << 10 )

/** \<\<public\>\> Create proxyfs_peer instance
 * @return new proxyfs_peer instance or NULL on error
 */
struct proxyfs_peer_t *proxyfs_peer_new(void)
{
	struct proxyfs_peer_t *self;
	self = (struct proxyfs_peer_t *)kmalloc( sizeof(struct proxyfs_peer_t), GFP_KERNEL );
	if( self == NULL ){
		mdbg(ERR3, "Allocating proxyfs_peer failed");
		goto exit0;
	}

	if( proxyfs_peer_init(self) != 0 )
		goto exit1;

	return self;
exit1:
	kfree(self);
exit0:
	return NULL;
}

/** \<\<public\>\> Initialize proxyfs_peer instance
 * @param self - pointer to proxyfs_peer_t instance
 * @return 0 on success
 */
int proxyfs_peer_init(struct proxyfs_peer_t *self)
{
	if( MSG_MAX_SIZE >= USE_VMALLOC )
		self->recv_buf = vmalloc( MSG_MAX_SIZE );
	else
		self->recv_buf = kmalloc( MSG_MAX_SIZE, GFP_KERNEL );

	if( self->recv_buf == NULL ){
		mdbg(ERR3, "Allocating recv_buf failed");
		return -1;
	}
	self->recv_start = 0;
	atomic_set(&self->ref_count, 1);
	spin_lock_init(&self->enqueue_msg_lock);
	self->state = PEER_CREATED;
	INIT_LIST_HEAD( & self->msg_queue );
	return 0;
}

/** \<\<public\>\> Puts proxyfs_peer instance reference and on reaching zero destroys the instance
 * @param self - pointer to proxyfs_peer_t instance
 */ 
void proxyfs_peer_put(struct proxyfs_peer_t *self)
{	
	if (!self)
		return;

	if ( self->sock )
		mdbg(INFO4, "Dropping peer reference %s - ref count: %d", kkc_sock_getpeername2(self->sock), atomic_read(&self->ref_count));
	else
		mdbg(INFO4, "Dropping peer reference - ref count: %d", atomic_read(&self->ref_count));

	if (atomic_dec_and_test(&self->ref_count)) {		  
		mdbg(INFO3, "Freeing peer");
		if( MSG_MAX_SIZE >= USE_VMALLOC )
			vfree(self->recv_buf);
		else
			kfree(self->recv_buf);
		kkc_sock_put(self->sock);
		
		self->state = PEER_DISCONNECTED;
		self->sock = NULL;
		
		kfree(self);
		mdbg(INFO3, "Peer released");
	}
}

/** \<\<public\>\> Nonblocking send queued messages 
 * @param self - pointer to proxyfs_peer_t instance 
 */
void proxyfs_peer_real_send(struct proxyfs_peer_t *self)
{
	struct proxyfs_msg *msg;
	int status;

	// Cannot send in non-connected state
	if ( self->state == PEER_DISCONNECTED || !self->sock) {
		mdbg(INFO3, "Skipping real send for peer %s. Not connected (was in state %d)", kkc_sock_getpeername2(self->sock), self->state );
		return;
	}

send_n:	if( ! list_empty( & self->msg_queue ) ){
		msg = list_entry( self->msg_queue.next, struct proxyfs_msg, msg_queue );
		status = proxyfs_msg_real_send( msg, self->sock );
		if( status == 1 ){
			mdbg(INFO3, "Message was send");
			list_del( self->msg_queue.next );
			proxyfs_msg_destroy(msg);
			goto send_n;
		}
		else
			mdbg(INFO3, "Sending returned %d", status);
	}
	else {
		//mdbg(INFO3, "Message queue for %s is empty", kkc_sock_getpeername2(self->sock));
	}
}

/** \<\<public\>\> Nonblocking recv messages 
 * @param self - pointer to proxyfs_peer_t instance
 *
 * @return 1 when complete message is received
 */
int proxyfs_peer_real_recv(struct proxyfs_peer_t *self)
{
	int result = 0;
	int msg_size;

	if( self->recv_start < MSG_HDR_SIZE ){
		result = kkc_sock_recv( self->sock, self->recv_buf + self->recv_start,
		       	MSG_HDR_SIZE - self->recv_start, KKC_SOCK_NONBLOCK);

		if( result == -EAGAIN ){
			//mdbg(INFO3, "No incoming data from %s", kkc_sock_getpeername2(self->sock));
		}
		else if( result > 0 ){
			mdbg(INFO3, "Received %d bytes from %s", result, kkc_sock_getpeername2(self->sock));
			self->recv_start += result;
		}
		else
			mdbg(ERR3, "recv(%s) returned %d", kkc_sock_getpeername2(self->sock), result);
	}
	if( self->recv_start >=  MSG_HDR_SIZE ){
		msg_size = proxyfs_msg_get_size( self->recv_buf );

		if( msg_size == self->recv_start )
			return 1; // Message is complete

		result = kkc_sock_recv( self->sock, self->recv_buf + self->recv_start,
		       	msg_size - self->recv_start, KKC_SOCK_NONBLOCK);

		if( result == -EAGAIN ){
			//mdbg(INFO3, "No incoming data from %s", kkc_sock_getpeername2(self->sock));
		}
		else if( result > 0 ){
			mdbg(INFO3, "Received %d bytes from %s", result, kkc_sock_getpeername2(self->sock));
			self->recv_start += result;
			if( self->recv_start == proxyfs_msg_get_size(self->recv_buf) )
				return 1; // Message is complete
			else
				return 0;
		}
		else
			mdbg(ERR3, "recv(%s) returned %d", kkc_sock_getpeername2(self->sock), result);
	}
	return result;
}

/** Conects to peer
 * @param self - pointer to proxyfs_peer_t instance
 * @param connect_str - connection string for peer we are connecting to
 *
 * @return 0 when success
 */
int proxyfs_peer_connect(struct proxyfs_peer_t *self, const char *connect_str){
	int err;
	if ((err = kkc_connect(&(self->sock), connect_str))) {
	      self->sock = NULL;
              mdbg(ERR3, "Failed creating KKC connecting to '%s' error %d", connect_str, err);
	      return -1;
	}

	self->state = PEER_CONNECTED;

	return 0;
}

/** \<\<public\>\> Disconnects  peer 
 * @param self - pointer to proxyfs_peer_t instance
 *
 * @return 0 when success
 */
int proxyfs_peer_disconnect(struct proxyfs_peer_t *self){
	if( self->sock != NULL && self->state == PEER_CONNECTED ) {
		self->state = PEER_DISCONNECTED;
		return kkc_sock_shutdown(self->sock);
	}

	return 0;

}

/** \<\<public> Wait on  peer 
 * @param self - pointer to proxyfs_peer_t instance
 *
 * @return 0 when success
 */
int proxyfs_peer_wait(struct proxyfs_peer_t *self){
	init_waitqueue_entry( &self->socket_wait, current );

	return kkc_sock_add_wait_queue( self->sock, &self->socket_wait );
}
