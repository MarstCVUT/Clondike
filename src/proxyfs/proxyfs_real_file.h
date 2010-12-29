/**
 * @file proxyfs_real_file.h - Main real file class (used by proxyfs_server_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_real_file.h,v 1.6 2008/05/02 19:59:20 stavam2 Exp $
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
/********************** PUBLIC METHODS AND DATA *********************************/
#ifndef _PROXYFS_REAL_FILE_H_PUBLIC
#define _PROXYFS_REAL_FILE_H_PUBLIC

#include <linux/types.h>
#include "proxyfs_peer.h"

/**
 * @defgroup proxyfs_real_file_class proxyfs_real_file class
 * @ingroup proxyfs_file_class
 *
 * This is used on real file 
 * and proxyfs_server task.
 *
 * @{
 */

/** \<\<public\>\> Declaration of proxyfs_real_file */
struct proxyfs_real_file;

/** \<\<public\>\> Do ioctl */
long proxyfs_real_file_do_ioctl(struct proxyfs_real_file *self, unsigned int cmd, unsigned long arg);

/** \<\<public\>\> Write to real file */
int proxyfs_real_file_write(struct proxyfs_real_file *self);

/** \<\<public\>\> Read from real file */
int proxyfs_real_file_read(struct proxyfs_real_file *self);

/** \<\<public\>\> "Open" file and send response message */
int proxyfs_real_file_open(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer);

/** \<\<public\>\> "Close" file */
int proxyfs_real_file_close(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer);
/** \<\<public\>\> Marks file as read requested.. see MSG_READ_REQUEST comment for details */
int proxyfs_real_file_mark_read_request(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer);

/** \<\<public\>\> Initialize real_file */
int proxyfs_real_file_init(struct proxyfs_real_file *self);

/** \<\<public\>\> Deinitialize real_file */
void proxyfs_real_file_destroy(struct proxyfs_real_file *self);

/** \<\<public\>\> Casts to struct proxyfs_real_file_t * */
#define PROXYFS_REAL_FILE(arg) ((struct proxyfs_real_file*)(arg))

/** \<\<public\>\> fsync file  */
int proxyfs_real_file_fsync(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer);

#endif // _PROXYFS_REAL_FILE_H_PUBLIC

/********************** PROTECTED METHODS AND DATA ******************************/
#ifdef  PROXYFS_REAL_FILE_PROTECTED
#ifndef _PROXYFS_REAL_FILE_H_PROTECTED
#define _PROXYFS_REAL_FILE_H_PROTECTED

#define PROXYFS_FILE_PROTECTED // Parent
#include "proxyfs_file.h"

#include <asm/ioctls.h>
#include <linux/smp_lock.h>

/** \<\<private\>\> Structure for methods supporting polymorphism */
struct proxyfs_real_file_ops{
	/** Write to file from read buffer */
	int (*write)(struct proxyfs_real_file *);
	/** Reads from file and write to writte buffer */
	int (*read)(struct proxyfs_real_file *);
	/** "Open" file and send response message */
	int (*open)(struct proxyfs_real_file *, struct proxyfs_peer_t *);

	struct proxyfs_real_file* (*duplicate)(struct proxyfs_real_file* file);
};


/** \<\<private\>\> Proxyfs_real_file structure */
struct proxyfs_real_file {
	/** Parent class */
	struct proxyfs_file_t parent;
	/** "Real" file */
	struct file *file;
	/** Used to queue to wait queues.. keeps collection of "proxyfs_real_file_waiter"s */
	struct list_head waiters_list;
	/** Operations supporting polymorphism */
	struct proxyfs_real_file_ops *ops;
	/** 
         * In this collection we keep the task we from which we stole the file + all tasks, that were forked from that task
         * after the stealing
         */
	struct list_head shadow_list;
};

/** 
 * Helper structure for registering proxy files to poll wait queues of real files
 */
struct proxyfs_real_file_waiter {
	/** Element used to register into the "physical" file queue */
	wait_queue_t wait;
	/** Wait queue, in which is this waiter enqueued.. required for dequeueing */
	wait_queue_head_t* wait_head;
	/** Element used to register into a proxyfs_real_file "waiters_list" */
	struct list_head waiters_list;
};

/** Helper structure to enlist shadow tasks into real files */
struct proxyfs_real_file_task {
	/** Associated shadow */
	struct task_struct* shadow;
	/** Used for enlisting into a shadow_list */
	struct list_head shadow_list;
};

/** 
 * \<\<public\>\> Adds shadow task into a shadow list of the file  
 *
 * @return 0 on success
 */
int proxyfs_real_file_add_shadow(struct proxyfs_real_file *self, struct task_struct* shadow);

/** 
 * \<\<public\>\> Removes shadow task from a shadow list of the file  
 *
 * @return 1, if list become empty, 0 otherwise
 */
int proxyfs_real_file_remove_shadow(struct proxyfs_real_file *self, struct task_struct* shadow);

/** 
 * \<\<public\>\> Checks, whether shadow task is in list of shadow tasks of this file
 *
 * @return 1, if the file is used also by provided shadow task
 */
int proxyfs_real_file_has_shadow(struct proxyfs_real_file *self, struct task_struct* shadow);


/** 
 * Polls real file and checks, if there are some data that can be read 
 * @return 0 if there are no data, positive number otherwise (negative on error)
 */
static inline int proxyfs_real_file_poll_read(struct proxyfs_real_file* self) {
	unsigned long mask;

	if (self->file->f_op && self->file->f_op->poll ) {
		mask = self->file->f_op->poll( self->file, NULL );
		mdbg(INFO3, "TEMPORARY DEBUG.. poll mask %lu", mask);
		if ( !(mask & (POLLIN|POLLHUP)) ) // We chak as well for POLLHUP so that we get EOF
			return 0; // Nothing can be read
	} else {
		mdbg(ERR1, "Cannot poll real file!");
		return -1;
	}
	return 1;
}

/** 
 * Polls real file and checks, if there is some to which we can writte 
 * @return 0 if there is no space, positive number otherwise (negative on error)
 */
static inline int proxyfs_real_file_poll_write(struct proxyfs_real_file* self) {
	unsigned long mask;

	if (self->file->f_op && self->file->f_op->poll ) {
		mask = self->file->f_op->poll( self->file, NULL );
		mdbg(INFO3, "TEMPORARY DEBUG.. write poll mask %lu", mask);
		if ( !(mask & (POLLOUT)) )
			return 0; // Nothing can be written
	} else {
		mdbg(ERR1, "Cannot poll real file!");
		return -1;
	}
	return 1;
}

/** 
 * Tries to get via IOCTL count of bytes that can be read from the real file
 * @return counf of the files that can be read
 */
static inline int proxyfs_real_file_ioctl_nread(struct proxyfs_real_file* self) {
	if (self->file->f_op && self->file->f_op->ioctl ) {
		int nread, error;
		int cmd = FIONREAD;
		mm_segment_t old_fs;
		old_fs = get_fs();

			
		set_fs(KERNEL_DS);	
		lock_kernel();
		error = self->file->f_op->ioctl(self->file->f_path.dentry->d_inode, self->file, cmd, (unsigned long)&nread);
		unlock_kernel();
		set_fs(old_fs);

		mdbg(INFO3, "Nread: %d, error: %d", nread, error);
		if ( error < 0 )
			return error;

		return nread;
	}

	mdbg(INFO3, "Read file does not support IO ctl operations! Id: %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));
	return -1;
}

#endif // _PROXYFS_REAL_FILE_H_PROTECTED
#endif //  PROXYFS_REAL_FILE_PROTECTED

/********************** PRIVATE METHODS AND DATA *********************************/
#ifdef  PROXYFS_REAL_FILE_PRIVATE
#ifndef _PROXYFS_REAL_FILE_H_PRIVATE
#define _PROXYFS_REAL_FILE_H_PRIVATE

/** \<\<private\>\> Writes to file from peer - writes to read buffer */
static int proxyfs_real_file_write_from_peer(struct proxyfs_file_t *self, const char *buf, size_t count);

/** \<\<private\>\> Reads from file to peer - reads from write buffer */
static int proxyfs_real_file_read_to_peer(struct proxyfs_file_t *self, char *buf, size_t count);

/** \<\<private\>\> Submit that data was written to file on other side */
static void proxyfs_real_file_submit_read_to_peer(struct proxyfs_file_t *self, size_t count);

/** \<\<private\>\> Get maximum amount and address of data, that can be read at one time */
static void proxyfs_real_file_write_buf_info(struct proxyfs_file_t *self, size_t* total_size, size_t* size, void** addr);

/** <\<\private\>\> Structure with pointers to virtual methods */
static struct proxyfs_file_ops real_file_ops = {
	/** Writes to file from peer - writes to read buffer */
	.write_from_peer = &proxyfs_real_file_write_from_peer,
	/** Reads from file to peer - reads from write buffer */
	.read_to_peer = &proxyfs_real_file_read_to_peer,
	/** Call this to submit that data was writen to file on other side */
	.submit_read_to_peer = proxyfs_real_file_submit_read_to_peer,
	/** Get write buf info */
	.write_buf_info = proxyfs_real_file_write_buf_info,
};

#endif // _PROXYFS_REAL_FILE_H_PRIVATE
#endif //  PROXYFS_REAL_FILE_PRIVATE
/**
 * @}
 */
