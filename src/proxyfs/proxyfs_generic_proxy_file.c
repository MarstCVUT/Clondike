/**
 * @file proxyfs_generic_proxy_file.c -  client generic fileclass (used by proxyfs_client_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_generic_proxy_file.c,v 1.1 2007/09/03 03:40:45 malatp1 Exp $
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

#define PROXYFS_GENERIC_PROXY_FILE_PRIVATE
#define PROXYFS_GENERIC_PROXY_FILE_PROTECTED // Own header
#include "proxyfs_generic_proxy_file.h"

/** \<\<private\>\> Write to file from user space buffer 
 * @param *self - pointer to this file instance
 * @param *buf - pointer to data in userspace
 * @param count - size of data
 *
 * @return the number of bytes written or error (negative number)
 * */
static int proxyfs_generic_proxy_file_write_user(struct proxyfs_proxy_file_t *self, const char *buf, size_t count)
{
	int lenght;
	if( ! access_ok(VERIFY_WRITE, buf, count) )
		return -EFAULT;

	down( & self->write_buf_sem );
	lenght = circ_buf_write_user( & PROXYFS_FILE(self)->write_buf, buf, count );
	up( & self->write_buf_sem );

	return lenght;
}


/** \<\<private\>\> Read from file to user space buffer 
 * @param *self - pointer to this file instance
 * @param *buf - pointer to buffer in userspace
 * @param count - size of data
 *
 * @return the number of bytes read or error (negative number)
 */
static int proxyfs_generic_proxy_file_read_user(struct proxyfs_proxy_file_t *self, char *buf, size_t count)
{
	int lenght;
	if( ! access_ok(VERIFY_WRITE, buf, count) )
		return -EFAULT;

	down( & self->read_buf_sem );
	lenght = circ_buf_read_user( & PROXYFS_FILE(self)->read_buf, buf, count );
	up( & self->read_buf_sem );

	return lenght;
}

/** \<\<public\>\> Specialize proxyfs_proxy_file to proxyfs_generic_proxy_file
 * @param self - pointer to initialized proxyfs_proxy_file structure
 *
 * @return proxyfs_generic_proxy_file instance
 * */
struct proxyfs_generic_proxy_file *proxyfs_generic_proxy_file_from_proxy_file(struct proxyfs_proxy_file_t* self)
{
	self->ops = &generic_proxy_file_ops;
	return PROXYFS_GENERIC_PROXY_FILE(self);
}
