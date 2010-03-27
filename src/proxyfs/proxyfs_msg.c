/**
 * @file proxyfs_msg.c - Proxyfs messages.
 *                      
 * 
 *
 *
 * Date: 08/12/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_msg.c,v 1.4 2007/10/20 14:24:20 stavam2 Exp $
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

#include <asm/uaccess.h>
#include <kkc/kkc.h>

#define PROXYFS_MSG_PRIVATE
#include "proxyfs_msg.h"

/** Maximum message size */



static struct proxyfs_msg *proxyfs_msg_new_internal(unsigned long msg_num, 
		unsigned long file_ident, unsigned long data_size, void *data_ptr, int take_ownership)
{
	struct proxyfs_msg *msg;

	if( data_size > MSG_MAX_SIZE - MSG_HDR_SIZE ){
		mdbg(ERR3, "Data too long");
		return NULL;
	}

	msg = (struct proxyfs_msg *)kmalloc( sizeof(struct proxyfs_msg), GFP_KERNEL );
	if( msg == NULL ){
		mdbg(ERR3, "Proxyfs message allocation failed");
		return NULL;
	}

	msg->header.magic = MSG_MAGIC;
	msg->header.msg_num = msg_num;
	msg->header.file_ident = file_ident;
	msg->header.data_size = data_size;

	msg->free = take_ownership;
	msg->data = data_ptr;
	msg->send_start = 0;

	return msg;
}

/** \<\<public\>\> Proxyfs message constructor 
 * @param msg_num - Message type identifikator
 * @param file_ident - File identifikator 
 * @param data_size - size of data which will be send after header
 * @param data_ptr - pointer to data
 *
 * @return new message or NULL on error
 */
struct proxyfs_msg *proxyfs_msg_new(unsigned long msg_num, 
		unsigned long file_ident, unsigned long data_size, void *data_ptr)
{
	return proxyfs_msg_new_internal(msg_num, file_ident, data_size, data_ptr, 0);
}

/** \<\<public\>\> Proxyfs message constructor. In this version the message takes ownership of the data */
struct proxyfs_msg *proxyfs_msg_new_takeover_ownership(unsigned long msg_num, 
		unsigned long file_ident, unsigned long data_size, void *data_ptr) {
	return proxyfs_msg_new_internal(msg_num, file_ident, data_size, data_ptr, 1);
}


/** \<\<public\>\> Proxyfs message constructor 
 * @param msg_num - Message type identifikator
 * @param file_ident - File identifikator 
 * @param ... - size of the first arg., pointer to the first arg., size of the second arg. 
 * pointer to the second arg. etc... ended by 0
 *
 * @return new message or NULL on error
 */
struct proxyfs_msg *proxyfs_msg_compose_new(unsigned long msg_num, 
		unsigned long file_ident, ... )
{
	struct proxyfs_msg *msg;
	va_list args;
	size_t msg_size, element_size;

	msg = (struct proxyfs_msg *)kmalloc( sizeof(struct proxyfs_msg), GFP_KERNEL );
	if( msg == NULL ){
		mdbg(ERR3, "Proxyfs message allocation failed");
		goto exit0;
	}

	va_start(args, file_ident);
	for(msg_size = 0; (element_size = va_arg(args, u_int32_t)) != 0; va_arg(args, void*)) {
		msg_size += element_size;
	}
	va_end(args);

	if( (msg->data = kmalloc( msg_size, GFP_KERNEL ) ) == NULL ){
		mdbg(ERR3, "Memory allocation for data (%lu bytes)  failed", (unsigned long)msg_size);
		goto exit1;
	}

	va_start(args, file_ident);
	for(msg_size = 0; (element_size = va_arg(args, u_int32_t)) != 0; msg_size += element_size ) {
		void* from_pointer = va_arg(args, void*);
		mdbg(INFO3, "Appending argument on post %lu. Arg size: %lu", (unsigned long)msg_size, (unsigned long)element_size);
		memcpy( msg->data + msg_size, from_pointer, element_size );		
	}
	va_end(args);

	msg->header.magic = MSG_MAGIC;
	msg->header.msg_num = msg_num;
	msg->header.file_ident = file_ident;
	msg->header.data_size = msg_size;

	msg->free = 1;
	msg->send_start = 0;

	return msg;
exit1:
	kfree(msg);
exit0:
	return NULL;
}

/** \<\<public\>\> Send message to socket 
 * @param self - pointer to proxyfs_msg instance
 * @param sock - socket used for sending
 *
 * @return 1 if the message was completely send
 * */
int proxyfs_msg_real_send(struct proxyfs_msg *self, struct kkc_sock *sock)
{
	int msg_size;
	int result;

	msg_size = proxyfs_msg_get_size( self );

	mdbg(INFO3, "Sending message %lu for file %lu size %d peer_name %s;", 
		self->header.msg_num, self->header.file_ident, msg_size,
		kkc_sock_getpeername2(sock));

	if( self->send_start < MSG_HDR_SIZE ){ // Header hasn't been send yet
		result = kkc_sock_send( sock, self + self->send_start,
		       	MSG_HDR_SIZE - self->send_start, KKC_SOCK_NONBLOCK);
		if( result > 0 ){
			mdbg(INFO3, "Send %d bytes", result); 
			self->send_start += result;
		}
		else
			return result;
	}
	if( self->send_start >= MSG_HDR_SIZE ){ // Header has been send 
		if( self->send_start == msg_size )
			return 1; // Message send
		result = kkc_sock_send( sock, self->data + self->send_start - MSG_HDR_SIZE,
		       	msg_size - self->send_start, KKC_SOCK_NONBLOCK);
		if( result > 0 ){
			mdbg(INFO3, "Send %d bytes", result); 
			self->send_start += result;
			if( self->send_start == msg_size )
				return 1; // Message send
		}
		else
			return result;
	}
	return 0;

}

/** \<\<public\>\> Releases a message instance */
void proxyfs_msg_destroy(struct proxyfs_msg* self) {	
	if ( self->free ) {
		mdbg(INFO3, "Destroying message data: %p (Size: %lu)", self->data, self->header.data_size); 
		kfree(self->data);
	}
	mdbg(INFO3, "Destroying message: %p", self); 	
	kfree(self);
};
