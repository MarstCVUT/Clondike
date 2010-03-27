/**
 * @file proxyfs_proxy_file.h - Main client file class (used by proxyfs_client_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_proxy_file.h,v 1.3 2007/10/07 15:54:00 stavam2 Exp $
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
#ifndef _PROXYFS_PROXY_FILE_H_PUBLIC
#define _PROXYFS_PROXY_FILE_H_PUBLIC

#include <linux/types.h>
#include <linux/poll.h>
#include <dbg.h>
#include "proxyfs_client.h"

#include "proxyfs_file.h" // Parent

/**
 * @defgroup proxyfs_proxy_file_class proxyfs_proxy_file class
 * @ingroup proxyfs_file_class
 *
 * This is used on between VFS 
 * and proxyfs_client task.
 *
 * @{
 */

/** \<\<public\>\> proxyfs_proxy_file_t declaration */
struct proxyfs_proxy_file_t;

/** \<\<public\>\> Write to file from user space buffer */
int proxyfs_proxy_file_write_user(struct proxyfs_proxy_file_t *proxyfile, const char *buf, size_t count);

/** \<\<public\>\> Read from file to user space buffer */
int proxyfs_proxy_file_read_user(struct proxyfs_proxy_file_t *proxyfile, char *buf, size_t count);

/** \<\<public\>\> Reads from file to peer - reads from write buffer */
unsigned proxyfs_proxy_file_wait_interruptible(struct proxyfs_proxy_file_t *self, unsigned status);

/** \<\<public\>\> Call ioctl with arg in user space */
long proxyfs_proxy_file_ioctl_user(struct proxyfs_proxy_file_t *self, unsigned int cmd, unsigned long arg);

/** \<\<public\>\> Poll for VFS */
unsigned int proxyfs_file_poll(struct file *file, struct poll_table_struct *wait); 

/** \<\<public\>\> Initialize proxyfs_file struct */
int proxyfs_proxy_file_init(struct proxyfs_proxy_file_t* self);

/** \<\<public\>\> Deinitialize proxyfs_file struct */
void proxyfs_proxy_file_destroy(struct proxyfs_proxy_file_t* self);

/** \<\<public\>\> Interruptible wait for space in file */
unsigned proxyfs_proxy_file_wait_for_space_interruptible(struct proxyfs_proxy_file_t *self, unsigned space);

/** \<\<public\>\> Close proxyfile */
int proxyfs_proxy_file_close(struct proxyfs_proxy_file_t *self);

/** \<\<public\>\> Fsync proxyfile and coresponding real file */
int proxyfs_proxy_file_fsync(struct proxyfs_proxy_file_t *self);

/** \<\<public\>\> Cast to struct proxyfs_proxy_file_t * */
#define PROXYFS_PROXY_FILE(arg) ((struct proxyfs_proxy_file_t *)(arg))


#endif // _PROXYFS_PROXY_FILE_H_PUBLIC

/********************** PROTECTED METHODS AND DATA ******************************/

#ifdef PROXYFS_PROXY_FILE_PROTECTED
#ifndef _PROXYFS_PROXY_FILE_H_PROTECTED
#define _PROXYFS_PROXY_FILE_H_PROTECTED

#define PROXYFS_FILE_PROTECTED  // parent
#include "proxyfs_file.h"

struct proxyfs_proxy_file_t;

/** \<\<protected\>\> Operations supporting polymorphism */
struct proxyfs_proxy_file_ops {
	/** Write to file from user space buffer */
	int(*write_user)(struct proxyfs_proxy_file_t *, const char *, size_t);
	/** Read from file to user space buffer */
	int(*read_user)(struct proxyfs_proxy_file_t *, char *, size_t);
};

/** \<\<protected\>\> Structure on which VFS and proxyfs_client task are interacting */
struct proxyfs_proxy_file_t {
	/** Parrent structure */
	struct proxyfs_file_t parent;
	/** Semaphore protecting write_buf */
	struct semaphore write_buf_sem;
	/** Semaphore protecting read_buf */
	struct semaphore read_buf_sem;
	/** Processes waiting for change on this file */
	wait_queue_head_t waiting_procs;
	/** Processes waiting for change on this file */
	struct proxyfs_msg **syscall_resp;
	/** Processes waiting for change on this file */
	struct proxyfs_client_task *task;
	/** Polymorphism */
	struct proxyfs_proxy_file_ops *ops;
};


#endif // _PROXYFS_PROXY_FILE_H_PROTECTED
#endif // PROXYFS_PROXY_FILE_PROTECTED

#ifdef PROXYFS_PROXY_FILE_PRIVATE
#ifndef _PROXYFS_PROXY_FILE_H_PRIVATE
#define _PROXYFS_PROXY_FILE_H_PRIVATE

/** \<\<private\>\> Writes to file from peer - writes to read buffer */
static int proxyfs_proxy_file_write_from_peer(struct proxyfs_file_t *self, const char *buf, size_t count);

/** \<\<private\>\> Reads from file to peer - reads from write buffer */
static int proxyfs_proxy_file_read_to_peer(struct proxyfs_file_t *self, char *buf, size_t count);

/** \<\<private\>\> Submit that data was written to file on other side */
static void proxyfs_proxy_file_submit_read_to_peer(struct proxyfs_file_t *self, size_t count);

/** \<\<private\>\> Get maximum amount and address of data, that can be read at one time */
static void proxyfs_proxy_file_write_buf_info(struct proxyfs_file_t *self, size_t* total_size, size_t* size, void** addr);

/** <\<\private\>\> Structure with pointers to virtual methods */
static struct proxyfs_file_ops proxy_file_ops = {
	/** Writes to file from peer - writes to read buffer */
	.write_from_peer = &proxyfs_proxy_file_write_from_peer,
	/** Reads from file to peer - reads from write buffer */
	.read_to_peer = &proxyfs_proxy_file_read_to_peer,
	/** Call this to submit that data was writen to file on other side */
	.submit_read_to_peer = proxyfs_proxy_file_submit_read_to_peer,
	/** Get write buf info */
	.write_buf_info = proxyfs_proxy_file_write_buf_info,
};


#endif // _PROXYFS_PROXY_FILE_H_PRIVATE
#endif // PROXYFS_PROXY_FILE_PRIVATE

/**
 * @}
 */

