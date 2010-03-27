/**
 * @file proxyfs_real_file.c - Main real file class (used by proxyfs_server_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_real_file.c,v 1.8 2008/01/17 14:36:44 stavam2 Exp $
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
#include <linux/fs.h>
#include <linux/smp_lock.h>
#include <linux/sched.h>
#include <linux/vfs.h>

#include <asm/poll.h>

#include <dbg.h>
#include <proxyfs/proxyfs_peer.h>

#define PROXYFS_REAL_FILE_PRIVATE
#define PROXYFS_REAL_FILE_PROTECTED
#include "proxyfs_real_file.h"

/** \<\<public\>\> Do ioctl 
 * @param *self - pointer to this file instance
 * @param cmd - ioctl cmd
 * @param arg - ioctl arg
 *
 * @return - return code of ioctl
 * */
long proxyfs_real_file_do_ioctl(struct proxyfs_real_file *self, unsigned int cmd, unsigned long arg)
{
	int error = -ENOTTY;
	mm_segment_t old_fs;
	old_fs = get_fs();

	if (S_ISREG(self->file->f_path.dentry->d_inode->i_mode))
		goto out;

	if (!self->file->f_op)
		goto out;
	
	set_fs(KERNEL_DS);

	if (self->file->f_op->unlocked_ioctl) {
		error = self->file->f_op->unlocked_ioctl(self->file, cmd, arg);
		if (error == -ENOIOCTLCMD) {
			error = -EINVAL;
			mdbg(ERR2, "No ioctl cmd %d for file %lu", cmd, proxyfs_file_get_file_ident(PROXYFS_FILE(self)));
		}
		goto out;
	} else if (self->file->f_op->ioctl) {
		lock_kernel();
		error = self->file->f_op->ioctl(self->file->f_path.dentry->d_inode,
				self->file, cmd, arg);
		unlock_kernel();
	}

out:
	set_fs(old_fs);
	return error;
}

/** \<\<public\>\> Write to real file 
 * @param *self - pointer to this file instance
 *
 * @return amount of bytes written
 */
int proxyfs_real_file_write(struct proxyfs_real_file *self)
{
	if( self->ops && self->ops->write )
		return self->ops->write(self);
	else{
		mdbg(ERR3, "No write method");
		return -1;
	}
}

/** \<\<public\>\> Read from real file 
 * @param *self - pointer to this file instance
 *
 * @return amount of bytes read
 */
int proxyfs_real_file_read(struct proxyfs_real_file *self)
{
	if( self->ops && self->ops->read )
		return self->ops->read(self);
	else{
		mdbg(ERR3, "No read method");
		return -1;
	}
}

/** \<\<public\>\> Initialize real_file 
 * @param *self - pointer to this file instance
 *
 * @return zero on success
 */
int proxyfs_real_file_init(struct proxyfs_real_file *self)
{
	int rtn = proxyfs_file_init( PROXYFS_FILE( self ) );
	if( rtn == 0 ) {
		PROXYFS_FILE( self )->ops = &real_file_ops;
		INIT_LIST_HEAD( & self->shadow_list );
	}
	return rtn;
}

/** \<\<public\>\> "Open" file and send response message 
 * @param *self - pointer to this file instance
 * @param *peer - instance of peer which is opening this file
 *
 * @return zero on success
 */
int proxyfs_real_file_open(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer)
{
	int res;
	if( self->ops && self->ops->open ) {
		res = self->ops->open(self, peer);
		if ( !res )
			proxyfs_file_set_peer(PROXYFS_FILE(self),peer);
		return res;
	} else{
		mdbg(ERR3, "No open method");
		return -EINVAL;
	}
}

/** \<\<public\>\> "Close" file  
 * @param *self - pointer to this file instance
 * @param *peer - instance of peer which is closing this file
 *
 * @return zero on success
 */
int proxyfs_real_file_close(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer)
{
	struct proxyfs_msg *msg;
	int rtn = 0;

	if( PROXYFS_FILE(self)->peer == peer ){
		msg = proxyfs_msg_new(MSG_CLOSE_RESP, proxyfs_file_get_file_ident(PROXYFS_FILE(self)), 0, NULL );
		if ( !msg )
			return -ENOMEM;

		proxyfs_peer_send_msg( peer, msg );
		proxyfs_file_set_peer(PROXYFS_FILE(self),NULL);
		PROXYFS_FILE(self)->write_buf_unconfirmed = 0;
	}
	else{
		mdbg(ERR3, "Close message come from peer which hasn't opened the file");
		rtn = -1;
	}

	return rtn;
}

/** \<\<public\>\> fsync file  
 * @param *self - pointer to this file instance
 * @param *peer - instance of peer which is fsyncing this file
 *
 * @return zero on success
 */
int proxyfs_real_file_fsync(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer)
{
	struct proxyfs_msg *msg;
	int rtn;

	if( PROXYFS_FILE(self)->peer == peer ){
		rtn = vfs_fsync( self->file, self->file->f_path.dentry, 0 );
		// TODO: How do we send the result of fsync back? Or is there some fsync failed msg to be sent?
		msg = proxyfs_msg_new(MSG_FSYNC_RESP, proxyfs_file_get_file_ident(PROXYFS_FILE(self)), 0, NULL );
		if ( !msg )
			return -ENOMEM;

		proxyfs_peer_send_msg( peer, msg );
	}
	else{
		mdbg(ERR3, "Fsync message come from peer which hasn't opened the file");
		rtn = -1;
	}

	return rtn;
}

int proxyfs_real_file_mark_read_request(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer) {
	struct proxyfs_msg *msg;

	msg = proxyfs_msg_new(MSG_READ_REQUEST_RESP, proxyfs_file_get_file_ident(PROXYFS_FILE(self)), 0, NULL );
	if ( !msg )
		return -ENOMEM;

	proxyfs_file_mark_read_requested(PROXYFS_FILE(self));

	proxyfs_peer_send_msg( peer, msg );

	return 0;
}

/** \<\<public\>\> Deinitialize real_file 
 * @param *self - pointer to this file instance
 */
void proxyfs_real_file_destroy(struct proxyfs_real_file *self)
{	
	struct list_head *l, *nxt;
	struct proxyfs_real_file_waiter* waiter;
	mdbg(INFO3, "Destroying real file: %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));

	list_for_each_safe( l, nxt, &self->waiters_list ){
		waiter = list_entry(l, struct proxyfs_real_file_waiter, waiters_list);
		remove_wait_queue(waiter->wait_head, &waiter->wait);
		mdbg(INFO3, "Remove from waitqueue: %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));
		kfree(waiter);
	}

	proxyfs_file_destroy( PROXYFS_FILE( self ) );
}

/** \<\<private\>\> Get maximum amount and address of data, that can be read at one time
 * @param *self - pointer to this file instance
 * @param *total_size - pointer to size_t into which a total available data count will be written
 * @param *size - pointer to size_t into which the data size will be written 
 * @param **addr - pointer to void* into which the data address will be written 
 *
 * */
static void proxyfs_real_file_write_buf_info(struct proxyfs_file_t *self, size_t* total_size, size_t* size, void** addr)
{
	// TODO: No need of lock here? Comment why ;)
	*total_size = circ_buf_count( & self->write_buf );
	*size = circ_buf_get_read_size( & self->write_buf );
	*addr = circ_buf_get_read_addr( & self->write_buf );
}

/** \<\<private\>\> Writes to file from peer - writes to read buffer 
 * @param *self - pointer to this file instance
 * @param *buf - pointer to data in kernelspace
 * @param count - size of data
 *
 * @return the number of bytes written 
 * */
static int proxyfs_real_file_write_from_peer(struct proxyfs_file_t *self, const char *buf, size_t count)
{
	int lenght;

	lenght = circ_buf_write( & self->read_buf, buf, count );
	
	return lenght;
}

/** \<\<private\>\> Reads from file to peer - reads from write buffer 
 * @param *self - pointer to this file instance
 * @param *buf - pointer to buffer in kernelspace
 * @param count - size of data
 *
 * @return the number of bytes read or error (negative number)
 */
static int proxyfs_real_file_read_to_peer(struct proxyfs_file_t *self, char *buf, size_t count)
{
	int lenght;

	lenght = circ_buf_look_at( & self->write_buf, buf, count, self->write_buf_unconfirmed);
	if( lenght > 0 )
		self->write_buf_unconfirmed += lenght;
	
	return lenght;
}

/** \<\<private\>\> Submit that data was written to file on other side 
 * (can be safely deleted from proxyfs_file buffer) 
 * @param *self - pointer to this file instance
 * @param count - ammount of submited data  
 */
static void proxyfs_real_file_submit_read_to_peer(struct proxyfs_file_t *self, size_t count)
{
	circ_buf_adjust_read_ptr( & self->write_buf, count);
	self->write_buf_unconfirmed -= count;
}

/** 
 * \<\<public\>\> Adds shadow task into a shadow list of the file  
 *
 * @return 0 on success
 */
int proxyfs_real_file_add_shadow(struct proxyfs_real_file *self, struct task_struct* shadow) {
	struct proxyfs_real_file_task* file_task = kmalloc(sizeof(struct proxyfs_real_file_task), GFP_KERNEL);
	if ( !file_task )
		return -ENOMEM;

	file_task->shadow = shadow;
	list_add( &file_task->shadow_list, &self->shadow_list );

	return 0;	
}

/** 
 * \<\<public\>\> Removes shadow task from a shadow list of the file  
 *
 * @return 1, if list become empty, 0 otherwise
 */
int proxyfs_real_file_remove_shadow(struct proxyfs_real_file *self, struct task_struct* shadow) {
	struct list_head *l, *nxt;
	struct proxyfs_real_file_task* file_task;

	list_for_each_safe( l, nxt, &self->shadow_list ){
		file_task = list_entry(l, struct proxyfs_real_file_task, shadow_list);
		if ( file_task->shadow == shadow ) {
			list_del(l);
			kfree(file_task);
			break;
		}
	}

	return list_empty(&self->shadow_list);
}

/** 
 * \<\<public\>\> Checks, whether shadow task is in list of shadow tasks of this file
 *
 * @return 1, if the file is used also by provided shadow task
 */
int proxyfs_real_file_has_shadow(struct proxyfs_real_file *self, struct task_struct* shadow) {
	struct list_head *l;
	struct proxyfs_real_file_task* file_task;

	list_for_each( l,  &self->shadow_list ){
		file_task = list_entry(l, struct proxyfs_real_file_task, shadow_list);
		if ( file_task->shadow == shadow ) {
			return 1;
		}
	}

	return 0;
}
