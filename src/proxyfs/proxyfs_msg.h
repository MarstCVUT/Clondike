/**
 * @file proxyfs_msg.h - Proxyfs messages.
 *                      
 * 
 *
 *
 * Date: 08/12/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_msg.h,v 1.7 2008/01/17 14:36:44 stavam2 Exp $
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
#ifndef _PROXYFS_MSG_H
#define _PROXYFS_MSG_H
#include <dbg.h>
#include <kkc/kkc.h>
/** @defgroup proxyfs_msg_class proxyfs_msg class
 *
 * @ingroup proxyfs_module_class
 *
 * This class represents connection between two peers
 * allows sending and receiving messages
 * 
 * @{
 */

/** Proxyfs message header structure 
 * We allways start sending with sending this structure 
 **/
struct proxyfs_msg_header {
	/** Magic for error detection */
	unsigned long magic;
	/** Message type */
	unsigned long msg_num;
	/** Identifiktion of file which is the message afecting */
	unsigned long file_ident;
	/** Size of data which are send after header */
	unsigned long data_size;
	/** Open structure... */
	u_int32_t data[];
};

/** Proxyfs message structure */
struct proxyfs_msg {
	/** Message header */
	struct proxyfs_msg_header header;
	/** Pointer to data which are send after header */
	void *data;
	/** Struct for queueing messagges */
	struct list_head msg_queue;
	/** How far we are in sending progress */
	size_t send_start;
	/** Call kfree on data after sending message */
	int free;
};


/** \<\<public\>\> Size od message header */ 
#define MSG_HDR_SIZE (sizeof(struct proxyfs_msg_header))
/** \<\<public\>\> Magic number, each message starts with it */
#define MSG_MAGIC 0x1ee7babe
/** \<\<public\>\> Max. messagge size */
#define MSG_MAX_SIZE (4 << 10)

/** \<\<public\>\> msg_num for message which is send when opening file */
#define MSG_OPEN 	1
/** \<\<public\>\> msg_num for write message */
#define MSG_WRITE 	3
/** \<\<public\>\> msg_num for ioctl message */
#define MSG_IOCTL	4
/** \<\<public\>\> msg_num for message which is send when closing file */
#define MSG_CLOSE	5
/** \<\<public\>\> msg_num for message which is send when syncing file */
#define MSG_FSYNC	6
/** \<\<public\>\> Read request is used to notify server that the client is interested in reading from 
 * a file. It is a workaround for not nice behaving programs like make, which open pipe, fork child and
 * use correctly pipe as unidiractional, but they do not close the other end. We may then send the data to
 * a reading end, which is not interested in this data, and the process that really wants the data from pipe
 * won't ever get them..
 */
#define MSG_READ_REQUEST 7

/** \<\<public\>\> msg_num for message which can be send as reply for #MSG_OPEN */
#define MSG_OPEN_RESP_OK	101
/** \<\<public\>\> msg_num for message which can be send as reply for #MSG_OPEN */
#define MSG_OPEN_RESP_FAILED	102
/** \<\<public\>\> msg_num for message which can be send as reply for #MSG_WRITE */
#define MSG_WRITE_RESP		301
/** \<\<public\>\> msg_num for message which can be send as reply for #MSG_IOCTL */
#define MSG_IOCTL_RESP		401
/** \<\<public\>\> msg_num for message which can be send as reply for #MSG_CLOSE */
#define MSG_CLOSE_RESP		501
/** \<\<public\>\> msg_num for message which can be send as reply for #MSG_FSYNC */
#define MSG_FSYNC_RESP		601
/** \<\<public\>\> msg_num for message which can be send as reply for #MSG_READ_REQUEST */
#define MSG_READ_REQUEST_RESP	701

/** write_resp msg constants */
#define WRITE_RESP_BUFFER_FULL 1
#define WRITE_RESP_BUFFER_NOT_FULL 2

/** \<\<public\>\> Returns size of message (header + data)
 * @param self - pointer to proxyfs_msg 
 * @return message size
 */
static inline int proxyfs_msg_get_size(struct proxyfs_msg *self)
{
	if( self->header.magic != MSG_MAGIC ){
		mdbg(ERR3, "Bad magic");
		return 0;
	}
	else
		return self->header.data_size + MSG_HDR_SIZE;
}

/** \<\<public\>\> Proxyfs message constructor */
struct proxyfs_msg *proxyfs_msg_new(unsigned long msg_num, 
		unsigned long file_ident, unsigned long data_size, void *data_ptr);

/** \<\<public\>\> Proxyfs message constructor. In this version the message takes ownership of the data */
struct proxyfs_msg *proxyfs_msg_new_takeover_ownership(unsigned long msg_num, 
		unsigned long file_ident, unsigned long data_size, void *data_ptr);


/** \<\<public\>\> Proxyfs message constructor */
struct proxyfs_msg *proxyfs_msg_compose_new(unsigned long msg_num, 
		unsigned long file_ident, ... );

/** \<\<public\>\> Releases a message instance */
void proxyfs_msg_destroy(struct proxyfs_msg* self);

/** \<\<public\>\> Send message to socket */
int proxyfs_msg_real_send(struct proxyfs_msg *self, struct kkc_sock *sock);

/**
 * @}
 */

#endif // _PROXYFS_MSG_H
