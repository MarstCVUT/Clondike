/**
 * @file proxyfs_pipe_proxy_file.h - Main client file class (used by proxyfs_client_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_pipe_proxy_file.h,v 1.2 2007/09/03 09:27:08 malatp1 Exp $
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
#ifndef _PROXYFS_PIPE_PROXY_FILE_H_PUBLIC
#define _PROXYFS_PIPE_PROXY_FILE_H_PUBLIC

#include "proxyfs_proxy_file.h" // Parent

/**
 * @defgroup proxyfs_pipe_proxy_file_class proxyfs_pipe_proxy_file class
 * @ingroup proxyfs_proxy_file_class
 *
 * This is used between VFS and proxyfs_client task for proxying pipes
 *
 * @{
 */

/** \<\<public\>\> proxyfs_proxy_file_t declaration */
struct proxyfs_pipe_proxy_file;

/** \<\<public\>\> Specialize proxyfs_proxy_file to proxyfs_pipe_proxy_file */
struct proxyfs_pipe_proxy_file *proxyfs_pipe_proxy_file_from_proxy_file(struct proxyfs_proxy_file_t* self);

/** \<\<public\>\> Cast to struct proxyfs_proxy_file_t * */
#define PROXYFS_PIPE_PROXY_FILE(arg) ((struct proxyfs_pipe_proxy_file *)(arg))

#endif // _PROXYFS_PIPE_PROXY_FILE_H_PUBLIC

/********************** PROTECTED METHODS AND DATA ******************************/

#ifdef PROXYFS_PIPE_PROXY_FILE_PROTECTED
#ifndef _PROXYFS_PIPE_PROXY_FILE_H_PROTECTED
#define _PROXYFS_PIPE_PROXY_FILE_H_PROTECTED

#define PROXYFS_PROXY_FILE_PROTECTED  // parent
#include "proxyfs_proxy_file.h"

/** \<\<protected\>\> Structure on which VFS and proxyfs_client task are interacting */
struct proxyfs_pipe_proxy_file {
	/** Parrent structure */
	struct proxyfs_proxy_file_t parent;
};

#endif // _PROXYFS_PIPE_PROXY_FILE_H_PROTECTED
#endif // PROXYFS_PIPE_PROXY_FILE_PROTECTED

#ifdef PROXYFS_PIPE_PROXY_FILE_PRIVATE
#ifndef _PROXYFS_PIPE_PROXY_FILE_H_PRIVATE
#define _PROXYFS_PIPE_PROXY_FILE_H_PRIVATE

/** \<\<private\>\> Write to file from user space buffer */
static int proxyfs_pipe_proxy_file_write_user(struct proxyfs_proxy_file_t *proxyfile, const char *buf, size_t count);

/** \<\<private\>\> Read from file to user space buffer */
static int proxyfs_pipe_proxy_file_read_user(struct proxyfs_proxy_file_t *proxyfile, char *buf, size_t count);

/** <\<\private\>\> Structure with pointers to virtual methods */
static struct proxyfs_proxy_file_ops pipe_proxy_file_ops = {
	/** Write to file from user space buffer */
	.write_user = &proxyfs_pipe_proxy_file_write_user,
	/** Read from file to user space buffer */
	.read_user = &proxyfs_pipe_proxy_file_read_user,
};


#endif // _PROXYFS_PIPE_PROXY_FILE_H_PRIVATE
#endif // PROXYFS_PIPE_PROXY_FILE_PRIVATE

/**
 * @}
 */

