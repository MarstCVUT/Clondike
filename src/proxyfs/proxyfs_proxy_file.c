/**
 * @file proxyfs_proxy_file.c - Main client file class (used by proxyfs_client_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_proxy_file.c,v 1.7 2008/05/02 19:59:20 stavam2 Exp $
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

#define PROXYFS_PROXY_FILE_PRIVATE
#define PROXYFS_PROXY_FILE_PROTECTED // Own header
#include "proxyfs_proxy_file.h"
#include "proxyfs_peer.h"
#include "proxyfs_ioctl.h"
#include "virtual.h"

/** \<\<public\>\> Write to file from user space buffer 
 * @param *self - pointer to this file instance
 * @param *buf - pointer to data in userspace
 * @param count - size of data
 *
 * @return the number of bytes written or error (negative number)
 * */
int proxyfs_proxy_file_write_user(struct proxyfs_proxy_file_t *self, const char *buf, size_t count)
{
	VIRTUAL_FUNC(write_user, buf, count);
}


static inline int virtual_proxyfs_proxy_file_read_user(struct proxyfs_proxy_file_t *self, char *buf, size_t count) {
	VIRTUAL_FUNC(read_user, buf, count);
}

/** \<\<public\>\> Read from file to user space buffer 
 * @param *self - pointer to this file instance
 * @param *buf - pointer to buffer in userspace
 * @param count - size of data
 *
 * @return the number of bytes read or error (negative number)
 */
int proxyfs_proxy_file_read_user(struct proxyfs_proxy_file_t *self, char *buf, size_t count)
{
	struct proxyfs_msg *msg;
	u_int32_t read_count = virtual_proxyfs_proxy_file_read_user(self, buf, count);
	
	msg = proxyfs_msg_compose_new(MSG_WRITE_RESP, proxyfs_file_get_file_ident(PROXYFS_FILE(self)),
		sizeof(u_int32_t), &read_count, 0 );			
	proxyfs_client_send_message( self->task, self, msg );

	return read_count;
}

/** \<\<private\>\> Writes to file from peer - writes to read buffer 
 * @param *self - pointer to this file instance
 * @param *buf - pointer to data in kernelspace
 * @param count - size of data
 *
 * @return the number of bytes written 
 * */
static int proxyfs_proxy_file_write_from_peer(struct proxyfs_file_t *self, const char *buf, size_t count)
{
	int lenght = 0;
	down( & PROXYFS_PROXY_FILE(self)->read_buf_sem );
	if( count == 0 ){
		proxyfs_file_set_status( self, PROXYFS_FILE_READ_EOF );
		mdbg(INFO2, "EOF on file %lu", proxyfs_file_get_file_ident(self));
	}
	else
		lenght = circ_buf_write( & self->read_buf, buf, count );
	up( & PROXYFS_PROXY_FILE(self)->read_buf_sem );

	mdbg(INFO4, "Waking up sleepers on %lu", proxyfs_file_get_file_ident(self));
	wake_up( &PROXYFS_PROXY_FILE(self)->waiting_procs );
	return lenght;
}

/** \<\<private\>\> Reads from file to peer - reads from write buffer 
 * @param *self - pointer to this file instance
 * @param *buf - pointer to buffer in kernelspace
 * @param count - size of data
 *
 * @TODO: This is never used, is it correct? The functionality is done directly in task buffer sending..
 * @return the number of bytes read or error (negative number)
 */
static int proxyfs_proxy_file_read_to_peer(struct proxyfs_file_t *self, char *buf, size_t count)
{
	int lenght;
	down( & PROXYFS_PROXY_FILE(self)->write_buf_sem );

	lenght = circ_buf_look_at( & self->write_buf, buf, count, self->write_buf_unconfirmed);
	if( lenght > 0 )
		self->write_buf_unconfirmed += lenght;
	
	up( & PROXYFS_PROXY_FILE(self)->write_buf_sem );
	return lenght;
}


/** \<\<private\>\> Submit that data was written to file on other side 
 * (can be safely deleted from proxyfs_file buffer) 
 * @param *self - pointer to this file instance
 * @param count - amount of submited data  
 */
static void proxyfs_proxy_file_submit_read_to_peer(struct proxyfs_file_t *self, size_t count)
{
	down( & PROXYFS_PROXY_FILE(self)->write_buf_sem );
	circ_buf_adjust_read_ptr( & self->write_buf, count);
	self->write_buf_unconfirmed -= count;
	up( & PROXYFS_PROXY_FILE(self)->write_buf_sem );

	mdbg(INFO3, "Waking up file after peer read (%lu). Unconfirmed size : %lu", (unsigned long)count, (unsigned long) self->write_buf_unconfirmed);

	wake_up( & PROXYFS_PROXY_FILE(self)->waiting_procs );
}

/** \<\<public\>\> Interruptible wait on file 
 * @param *self - pointer to this file instance
 * @param status - status on which function returns
 *
 * @return - file status or zero if interrupted by signal
 * */
unsigned proxyfs_proxy_file_wait_interruptible(struct proxyfs_proxy_file_t *self, unsigned status)
{
	unsigned rtn = 0;
wait:	if ( wait_event_interruptible( self->waiting_procs, 
			proxyfs_file_get_status( PROXYFS_FILE(self), status ) ||
			proxyfs_peer_get_state(PROXYFS_FILE(self)->peer) != PEER_CONNECTED) == -ERESTARTSYS) 
		return 0;
	else {
		if ( proxyfs_peer_get_state(PROXYFS_FILE(self)->peer) != PEER_CONNECTED ) {
			mdbg(INFO3, "Peer not connected in wait. Peer state: %d\n", proxyfs_peer_get_state(PROXYFS_FILE(self)->peer));		
			return PROXYFS_FILE_CLOSED; // TODO: Or rather some BROKEN state?
		}

		rtn = proxyfs_file_get_status( PROXYFS_FILE(self), status );
		mdbg(INFO3, "Waiting on state: %u ... was interrupted in state %u\n", status, proxyfs_file_get_status( PROXYFS_FILE(self), status ));
		if( rtn & status)
			return rtn;
		goto wait;
	}
}

/** \<\<public\>\> Interruptible wait for space in file 
 * @param *self - pointer to this file instance
 * @param space - wanted amount of bytes
 *
 * @return - file status or zero if interrupted by signal
 * */
unsigned proxyfs_proxy_file_wait_for_space_interruptible(struct proxyfs_proxy_file_t *self, unsigned space)
{
	if( wait_event_interruptible( self->waiting_procs, 
			circ_buf_free_space( &PROXYFS_FILE(self)->write_buf ) >= space) == -ERESTARTSYS )
		return 0;
	else{
		return proxyfs_file_get_status( PROXYFS_FILE(self), 0xffffFFFF );
	}
}

/** \<\<public\>\> Do ioctl on remote file
 * @param *self - pointer to this file instance
 * @param cmd - ioctl cmd
 * @param arg - ioctl arg
 *
 * @return - return code of ioctl
 * */
long proxyfs_proxy_file_ioctl_user(struct proxyfs_proxy_file_t *self, unsigned int cmd, unsigned long arg)
{
	unsigned long ioctl_info;
	struct proxyfs_msg *msg;
	struct proxyfs_msg *msg_resp;
	size_t arg_size;
	void *ker_buf = NULL;
	u_int32_t command = cmd;
	long rtn = -ERESTARTSYS;

	ioctl_info = ioctl_get_info(cmd);
	arg_size = IOCTL_ARG_SIZE(ioctl_info);

	// Check access to memory 
	if( IOCTL_WILL_READ(ioctl_info) && !access_ok(VERIFY_WRITE, (void*)arg, arg_size) ){
		mdbg(ERR3, "Bad read pointer");
		return -EFAULT;
	}
	else if( IOCTL_WILL_WRITE(ioctl_info) && !access_ok(VERIFY_READ, (void*)arg, arg_size) ){
		mdbg(ERR3, "Bad write pointer");
		return -EFAULT;
	}

	// empty write buf
	switch ( proxyfs_proxy_file_wait_interruptible(self, PROXYFS_FILE_ALL_WRITTEN ) ) {
		case 0:
			return -ERESTARTSYS;
		case PROXYFS_FILE_CLOSED:
			return -EFAULT;
		// Otherwise process further
	}

	if( IOCTL_WILL_WRITE(ioctl_info) ){
		ker_buf = kmalloc( arg_size + sizeof(u_int32_t), GFP_KERNEL );
		if( ker_buf == NULL ){
			mdbg(ERR3, "Allocating memory for message data failed");
			goto exit0;
		}
		*(u_int32_t*)ker_buf = command;
		if( __copy_from_user( ker_buf + sizeof(u_int32_t), (void*)arg, arg_size ) != 0 )
			goto exit1;

		msg = proxyfs_msg_new(MSG_IOCTL, proxyfs_file_get_file_ident(PROXYFS_FILE(self)), sizeof(u_int32_t) + arg_size, ker_buf); 
		if(msg == NULL)
			goto exit1;
	}
	else{
		msg = proxyfs_msg_new(MSG_IOCTL, proxyfs_file_get_file_ident(PROXYFS_FILE(self)), sizeof(u_int32_t), &command); 
		if(msg == NULL)
			goto exit0;
	}

	msg_resp = proxyfs_client_do_syscall(self->task, self, msg);

	if( msg_resp ){
		rtn = msg_resp->header.data[0];
		if( IOCTL_WILL_READ(ioctl_info) ){
			if( __copy_to_user( (void*)arg, &msg_resp->header.data[1], arg_size ) != 0 )
				mdbg(ERR3, "Copying result back failed");
		}
		kfree( msg_resp );	
	}
	if( IOCTL_WILL_WRITE(ioctl_info) )
		kfree(ker_buf);
	return rtn;

exit1:
	kfree(ker_buf);
exit0:
	return -ERESTARTSYS;

}

/** \<\<public\>\> Close proxyfile
 * @param *self - pointer to this file instance
 *
 * @return zero on succes
 * */
int proxyfs_proxy_file_close(struct proxyfs_proxy_file_t *self)
{
	struct proxyfs_msg *msg;
	struct proxyfs_msg *msg_resp;
	long rtn = -ERESTARTSYS;
	u_int32_t exitting = current->flags & PF_EXITING;

	mdbg(INFO2, "Close proxy file requested. Ident: %lu Exitting: %d", proxyfs_file_get_file_ident(PROXYFS_FILE(self)), exitting );
	
	// We have to wait for a write buffer empty event..
	// TODO: however, if the peer connection is broken, this will never happen, and the close method will block.. what to do? Likely we should wait with some long timeout and then just break (or at least recheck connection)
	if( proxyfs_proxy_file_wait_interruptible(self, PROXYFS_FILE_ALL_WRITTEN ) == 0 ) {
		minfo(ERR2, "Closing of ident: %lu failed. Not all written", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));
		return -ERESTARTSYS;
	}

	msg = proxyfs_msg_new(MSG_CLOSE, proxyfs_file_get_file_ident(PROXYFS_FILE(self)), sizeof(exitting), &exitting); 
	if(msg != NULL){
		msg_resp = proxyfs_client_do_syscall(self->task, self, msg);
		// What the fck?????
		if( msg_resp == (void*)MSG_CLOSE_RESP )
			rtn = 0;
	}

  
	if ( rtn != 0 && proxyfs_peer_get_state(PROXYFS_FILE(self)->peer) != PEER_CONNECTED) {
	      // Even if our remote syscall has failed, we have to pretend it succeded as the peer is no longer connected and there is no chance of further close success
	      rtn = 0;
	}

	mdbg(INFO2, "Close proxy file request DONE, Res: %ld", rtn);

	return rtn;
}

/** \<\<public\>\> Fsync proxyfile and coresponding real file
 * @param *self - pointer to this file instance
 *
 * @return zero on succes
 * */
int proxyfs_proxy_file_fsync(struct proxyfs_proxy_file_t *self)
{
	struct proxyfs_msg *msg;
	struct proxyfs_msg *msg_resp;
	long rtn = -ERESTARTSYS;

	mdbg(INFO2, "Fsync requested. Ident: %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));

	// empty write buf
	switch ( proxyfs_proxy_file_wait_interruptible(self, PROXYFS_FILE_ALL_WRITTEN ) ) {
		case 0:
			mdbg(INFO2, "Cannot fsync, interrupted. Ident: %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));
			return -ERESTARTSYS;
		case PROXYFS_FILE_CLOSED:
			mdbg(INFO2, "Cannot fsync, already closed. Ident: %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));
			return -EFAULT;
		default:
			mdbg(INFO3, "Fsync all written, may proceed");		
		// Otherwise process further
	}

	mdbg(INFO2, "Sending fsync request. Ident: %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));

	msg = proxyfs_msg_new(MSG_FSYNC, proxyfs_file_get_file_ident(PROXYFS_FILE(self)), 0, NULL); 
	if(msg != NULL){
		msg_resp = proxyfs_client_do_syscall(self->task, self, msg);
		if( msg_resp == (void*)MSG_FSYNC_RESP )
			rtn = 0;
	}
	return rtn;
}

/** \<\<public\>\> Poll call for VFS 
 * @param *self - pointer to this file instance
 * @param *wait - pointer to poll table
 *
 * @return - mask of file status
 * */
unsigned int proxyfs_file_poll(struct file *file, struct poll_table_struct *wait)
{
	struct proxyfs_proxy_file_t *proxyfile = file->private_data;
	unsigned mask = 0;

	poll_wait(file, &proxyfile->waiting_procs, wait);

	if( proxyfs_file_get_status( PROXYFS_FILE(proxyfile), PROXYFS_FILE_CAN_READ ) )
		mask |= POLLIN | POLLRDNORM;   
	if( proxyfs_file_get_status( PROXYFS_FILE(proxyfile), PROXYFS_FILE_CAN_WRITE ) )
    		mask |= POLLOUT | POLLWRNORM; 
	return mask;
}

/** \<\<private\>\> Get maximum amount and address of data, that can be read at one time
 * @param *self - pointer to this file instance
 * @param *total_size - pointer to size_t into which a total available data count will be written
 * @param *size - pointer to size_t into which the data size will be written 
 * @param **addr - pointer to void* into which the data address will be written 
 *
 * */
static void proxyfs_proxy_file_write_buf_info(struct proxyfs_file_t *self, size_t* total_size, size_t* size, void** addr)
{
	down( & PROXYFS_PROXY_FILE(self)->write_buf_sem );

	*total_size = circ_buf_count( & self->write_buf );
	*size = circ_buf_get_read_size( & self->write_buf );
	*addr = circ_buf_get_read_addr( & self->write_buf );

	up( & PROXYFS_PROXY_FILE(self)->write_buf_sem ); 
}

/** \<\<public\>\> Initialize proxyfs_file struct
 * @param self - pointer to uninitialized proxyfs_proxy_file_t structure
 *
 * @return zero on success 
 * */
int proxyfs_proxy_file_init(struct proxyfs_proxy_file_t* self)
{
	int rtn;

	if( (rtn = proxyfs_file_init( PROXYFS_FILE(self) )) == 0 ){
		init_MUTEX( & self->write_buf_sem );
		init_MUTEX( & self->read_buf_sem );
		init_waitqueue_head( & self->waiting_procs );
		PROXYFS_FILE(self)->ops = &proxy_file_ops;
	}

	return rtn;
}

/** \<\<public\>\> Deinitialize proxyfs_file struct
 * @param self - pointer to uninitialized proxyfs_proxy_file_t structure
 *
 * @return zero on success 
 * */
void proxyfs_proxy_file_destroy(struct proxyfs_proxy_file_t* self)
{
	wake_up( & self->waiting_procs );
	proxyfs_file_destroy( PROXYFS_FILE(self) );
}
