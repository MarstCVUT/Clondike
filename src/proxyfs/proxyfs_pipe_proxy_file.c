/**
 * @file proxyfs_pipe_proxy_file.c -  client pipe file class (used by proxyfs_client_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_pipe_proxy_file.c,v 1.4 2008/01/17 14:36:44 stavam2 Exp $
 *
 * This file is part of Proxy filesystem (ProxyFS)
 * Copyleft (C) 2007 Petr Malat
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
#include <linux/types.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <linux/semaphore.h>

#define PROXYFS_PIPE_PROXY_FILE_PRIVATE
#define PROXYFS_PIPE_PROXY_FILE_PROTECTED // Own header
#include "proxyfs_pipe_proxy_file.h"

/** \<\<private\>\> Write to file from user space buffer 
 * @param *self - pointer to this file instance
 * @param *buf - pointer to data in userspace
 * @param count - size of data
 *
 * @return the number of bytes written or error (negative number)
 * */
static int proxyfs_pipe_proxy_file_write_user(struct proxyfs_proxy_file_t *self, const char *buf, size_t count)
{
	int lenght = 0;
	uint32_t write_size;

	if( ! access_ok(VERIFY_WRITE, buf, count) )
		return -EFAULT;

	down( & self->write_buf_sem );
	write_size = circ_buf_free_space( & PROXYFS_FILE(self)->write_buf );

	mdbg(INFO2, "[File %lu] Space in buf: %lu, writting %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)), (unsigned long)write_size, (unsigned long)count);

	if( write_size < 2*sizeof(write_size) ) // Won't write less then 4 bytes
		lenght = -(2*(int)sizeof(write_size));
	else if( count <= PIPE_BUF && write_size < sizeof(write_size) + count )
			lenght = -(sizeof(write_size) + count); // Not enough space
	else
	{
		write_size -= sizeof( write_size ); // Never overflow
		write_size = write_size < count ? write_size : count;
		circ_buf_write_user( & PROXYFS_FILE(self)->write_buf, (void*)&write_size, sizeof(write_size) );
		lenght = circ_buf_write_user( & PROXYFS_FILE(self)->write_buf, buf, write_size );
	}
	up( & self->write_buf_sem );

	mdbg(INFO2, "written %d", lenght);

	return lenght;
}


/** \<\<private\>\> Read from file to user space buffer 
 * @param *self - pointer to this file instance
 * @param *buf - pointer to buffer in userspace
 * @param count - size of data
 *
 * @return the number of bytes read or error (negative number)
 */
static int proxyfs_pipe_proxy_file_read_user(struct proxyfs_proxy_file_t *self, char *buf, size_t count)
{
	int lenght;
	if( ! access_ok(VERIFY_WRITE, buf, count) )
		return -EFAULT;

	if ( !proxyfs_file_was_read_requested(PROXYFS_FILE(self)) ) {
		// Pipe files need a special trick with read requested marking.. do it only 1st time read is called
		struct proxyfs_msg *msg;
		struct proxyfs_msg *msg_resp;

		mdbg(INFO2, "[File %lu] read request syscall", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));
	
		msg = proxyfs_msg_new(MSG_READ_REQUEST, proxyfs_file_get_file_ident(PROXYFS_FILE(self)), 0, NULL); 
		if(msg != NULL){
			msg_resp = proxyfs_client_do_syscall(self->task, self, msg);
			
			if( msg_resp != (void*)MSG_READ_REQUEST_RESP ) {
				mdbg(ERR4, "[File %lu] read request failed", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));
				return -EINTR; // Or EAGAIN, or retry, or...??
			}

			mdbg(INFO2, "[File %lu] read request succeeded", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));
			proxyfs_file_mark_read_requested(PROXYFS_FILE(self));
		}
	}


	down( & self->read_buf_sem );
	lenght = circ_buf_read_user( & PROXYFS_FILE(self)->read_buf, buf, count );
	up( & self->read_buf_sem );

	return lenght;
}

/** \<\<public\>\> Specialize proxyfs_proxy_file to proxyfs_pipe_proxy_file
 * @param self - pointer to initialized proxyfs_proxy_file structure
 *
 * @return proxyfs_pipe_proxy_file instance
 * */
struct proxyfs_pipe_proxy_file *proxyfs_pipe_proxy_file_from_proxy_file(struct proxyfs_proxy_file_t* self)
{
	self->ops = &pipe_proxy_file_ops;
	return PROXYFS_PIPE_PROXY_FILE(self);
}
