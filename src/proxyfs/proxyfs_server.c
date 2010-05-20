/**
 * @file proxyfs_server.c - Proxyfs server task.
 *                      
 * 
 *
 *
 * Date: 08/12/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_server.c,v 1.13 2008/05/02 19:59:20 stavam2 Exp $
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

#include <proxyfs/buffer.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/kthread.h>
#include <kkc/kkc.h>

#include <dbg.h>
#include <proxyfs/proxyfs_ioctl.h>
#include <asm/ioctls.h>
#include <asm/termbits.h>

static int errno;
#include <arch/current/make_syscall.h>
static inline _syscall3(long, ioctl, unsigned int, fd, unsigned int, cmd, unsigned long, arg);

#define PROXYFS_SERVER_PRIVATE       // Own header
#define PROXYFS_SERVER_PROTECTED       // Own header
#include "proxyfs_server.h"

#define PROXYFS_REAL_FILE_PROTECTED  // Friend
#include "proxyfs_real_file.h"
#include "proxyfs_tty_real_file.h"
#include "proxyfs_generic_real_file.h"
#include "proxyfs_pipe_real_file.h"

#include <linux/mount.h>

static struct proxyfs_server* self = NULL;

/** \<\<public\>\> */
int proxyfs_server_thread(void* ss)
{
	struct proxyfs_task_start_struct *start_struct = (struct proxyfs_task_start_struct*)ss;
	self = kmalloc(sizeof(struct proxyfs_server), GFP_KERNEL);
	if ( !self )
		return -EINVAL;

	proxyfs_task_init( PROXYFS_TASK(self) );
	PROXYFS_TASK(self)->tsk = current;
	PROXYFS_TASK(self)->ops = &server_ops;
	init_MUTEX(&self->files_sem );

	if( proxyfs_server_listen(start_struct->addr_str) != 0 ){
		proxyfs_task_complete( NULL, start_struct ); 
		kfree(self);
		return -EINVAL;	
	}

	proxyfs_task_complete( PROXYFS_TASK(self), start_struct ); 

	proxyfs_server_loop();

	return 0;
}

/** \<\<private\>\> Start listening on socket
 * @param *listen_str - kkc address on which server will listen
 *
 * @return zero on succes
 * */
int proxyfs_server_listen(const char *listen_str){
	int err;
	if ((err = kkc_listen(&self->server_sock, listen_str))) {
              mdbg(ERR3, "Failed creating KKC listening on '%s' error %d", listen_str, err);
	      return -1;
	}
        mdbg(INFO3, "Server listening on '%s'", listen_str);
	kkc_sock_register_read_callback(self->server_sock, proxyfs_task_data_ready_callback, self);
	mdbg(INFO3, "Callback registered");
	init_waitqueue_entry(&self->server_sock_wait, current );
	return kkc_sock_add_wait_queue( self->server_sock, &self->server_sock_wait );
}

/** \<\<private\>\> Try to accept new connection 
 * @return 1 if we accepted a new conection (and there can be another one waiting)
 * */
int proxyfs_server_accept(void){
	struct kkc_sock *peer_sock;
	struct proxyfs_peer_t *p;
	int err;
	if ((err = kkc_sock_accept( self->server_sock, &peer_sock, 
                             KKC_SOCK_NONBLOCK)) < 0) {
                //mdbg(INFO4, "No connection, accept would block %d", err);
                return 0;
        }
        
	mdbg(INFO3, "Accepted new connection");

	p = proxyfs_task_add_to_peers(PROXYFS_TASK(self), peer_sock); // FIXME osetrit chybu
	if( p != NULL ) {
		kkc_sock_register_read_callback(peer_sock,proxyfs_task_data_ready_callback, self);
		proxyfs_peer_wait(p);
	}

	return 1;
}

/** \<\<private\>\> Main server loop */
void proxyfs_server_loop(void){
	while(!kthread_should_stop()){
		proxyfs_task_wait_for_data_ready(PROXYFS_TASK(self));
		
		// Accept all incoming conections
		while( proxyfs_server_accept() == 1 );
		// Try to recv from all socket.  
		proxyfs_task_recv_peers( PROXYFS_TASK(self) );
		//mdbg(INFO3, "STATE: %lx", current->state);
		// Try to write all buffers to files and read from files.  
		proxyfs_server_read_and_write_real_files();
		//mdbg(INFO3, "STATE: %lx", current->state);

		proxyfs_task_send_buffers( PROXYFS_TASK(self) );
		//mdbg(INFO3, "STATE: %lx", current->state);
		proxyfs_task_send_peers( PROXYFS_TASK(self) );
		proxyfs_task_handle_dead_peers( PROXYFS_TASK(self) );	
	}

	mdbg(INFO1, "Finished server thread");
}

/** \<\<private\>\> Handle message specific for server
 * @param self - pointer to this proxyfs_task instance
 * @param peer - peer struct with received message
 * */
static void proxyfs_server_handle_msg(struct proxyfs_task *self, struct proxyfs_peer_t *peer )
{
	struct proxyfs_msg *msg = proxyfs_peer_get_msg(peer);
	struct proxyfs_file_t *real_file;
	u_int32_t exitting;
	//long rtn;
	
	mdbg(INFO1, "Msg%lu DATA: (%p)",msg->header.file_ident ,msg);

	switch( msg->header.msg_num ){
		case MSG_OPEN:
			mdbg(INFO1, "MSG_OPEN ident=%lu", msg->header.file_ident);
			real_file = proxyfs_task_find_file( self, msg->header.file_ident);
			if( real_file != NULL ){ 
				mdbg(INFO1, "File found");
				if( 0 && real_file->peer != NULL )
					mdbg(INFO1, "Atempt to open already opened file, rejecting...");
				else{
					proxyfs_real_file_open( PROXYFS_REAL_FILE(real_file), peer );
					return;
				}
			}
			mdbg(INFO1, "File not found");
			msg = proxyfs_msg_new(MSG_OPEN_RESP_FAILED, msg->header.file_ident, 0, NULL );
			proxyfs_peer_send_msg( peer, msg );
			break;
		case MSG_IOCTL:
			mdbg(INFO1, "MSG_IOCTL");
			real_file = proxyfs_task_find_file( self, msg->header.file_ident);
			if( real_file != NULL ){ 
				u_int32_t ioctl_info = ioctl_get_info(msg->header.data[0]);
				char* ioctl_buffer = kmalloc(IOCTL_ARG_SIZE(ioctl_info), GFP_KERNEL);
				if ( ioctl_buffer ) {
					u_int32_t result;
					mdbg(INFO2, "MSG_IOCTL execute command: %d Arg size: %d", msg->header.data[0], IOCTL_ARG_SIZE(ioctl_info));
					result = proxyfs_real_file_do_ioctl(PROXYFS_REAL_FILE(real_file), 
							msg->header.data[0], (unsigned long)ioctl_buffer);
	

					mdbg(INFO2, "MSG_IOCTL performed with result: %d", result);
					if( IOCTL_WILL_READ(ioctl_info) )
						msg = proxyfs_msg_compose_new(MSG_IOCTL_RESP, proxyfs_file_get_file_ident(real_file), 
							sizeof(u_int32_t), &result,
							IOCTL_ARG_SIZE(ioctl_info), ioctl_buffer, 
							(u_int32_t)0);
					else
						msg = proxyfs_msg_compose_new(MSG_IOCTL_RESP, proxyfs_file_get_file_ident(real_file), 
							sizeof(u_int32_t), &result, 
							(u_int32_t)0);

					kfree(ioctl_buffer);
					if( msg )
						proxyfs_peer_send_msg( peer, msg );
					else
						mdbg(ERR2, "Failed creating response");
				} else {
					mdbg(ERR2, "Not enough memory to create ioctl read buffer of size: %d", IOCTL_ARG_SIZE(ioctl_info));
				}
				return;
			}
			mdbg(INFO1, "File not found");
			/*msg = proxyfs_msg_new(MSG_OPEN_RESP_FAILED, msg->header.file_ident, 0, NULL );
			proxyfs_peer_send_msg( peer, msg );*/
			break;
		case MSG_CLOSE:
			exitting = msg->header.data[0];
			mdbg(INFO1, "MSG_CLOSE ident=%lu was exitting: %d", msg->header.file_ident, exitting);
			real_file = proxyfs_task_find_file( self, msg->header.file_ident);
			if( real_file != NULL ){ 
				mdbg(INFO1, "File found");
				proxyfs_real_file_close( PROXYFS_REAL_FILE(real_file), peer );
				if ( !exitting ) {
					/* If the process was not exitted, the close was ordinary close request made 
					   by the process, so we can mark the file closed, since nobody is going to 
					   open it again */
					mdbg(INFO1, "Marking physical file closed");
					proxyfs_file_set_status( real_file, PROXYFS_FILE_CLOSED );	
				}
				return;
			}
			msg = proxyfs_msg_new(MSG_CLOSE_RESP, msg->header.file_ident, 0, NULL );
			proxyfs_peer_send_msg( peer, msg );
			break;
		case MSG_FSYNC:
			mdbg(INFO1, "MSG_FSYNC ident=%lu", msg->header.file_ident);
			real_file = proxyfs_task_find_file( self, msg->header.file_ident);
			if( real_file != NULL ){ 
				mdbg(INFO1, "File found");
				proxyfs_real_file_fsync( PROXYFS_REAL_FILE(real_file), peer );
				return;
			}
			msg = proxyfs_msg_new(MSG_FSYNC_RESP, msg->header.file_ident, 0, NULL );
			proxyfs_peer_send_msg( peer, msg );
			break;
		case MSG_READ_REQUEST:
			mdbg(INFO1, "MSG_READ_REQUEST ident=%lu", msg->header.file_ident);
			real_file = proxyfs_task_find_file( self, msg->header.file_ident);
			if( real_file != NULL ){ 
				mdbg(INFO1, "File found");
				proxyfs_real_file_mark_read_request( PROXYFS_REAL_FILE(real_file), peer );
				return;
			}
			msg = proxyfs_msg_new(MSG_READ_REQUEST_RESP, msg->header.file_ident, 0, NULL );
			proxyfs_peer_send_msg( peer, msg );
			break;
		default:
			mdbg(ERR3, "Unknown msg number");
	}
}



/** \<\<private\>\> Try to write all write buffers to file 
 *
 */
void proxyfs_server_read_and_write_real_files(void){
	struct list_head *l, *nxt;
	struct proxyfs_msg *msg;
	struct proxyfs_real_file *real_file;
	struct proxyfs_peer_t *peer;
	u_int32_t length;

	down( & self->files_sem ); // Serialize access
	list_for_each_safe( l, nxt, & PROXYFS_TASK(self)->files ){
		// up( & self.files_sem ); // Adding new file can wait :-)
		real_file = PROXYFS_REAL_FILE( list_entry(l, struct proxyfs_file_t, files) );
		peer = PROXYFS_FILE(real_file)->peer;
		mdbg(INFO4, "Processing %lu, status %x", proxyfs_file_get_file_ident(PROXYFS_FILE(real_file)),
			proxyfs_file_get_status( PROXYFS_FILE(real_file), 0xffff ) );
		// write to file
		if( (length = proxyfs_real_file_write(real_file)) > 0 ){
			mdbg(INFO3, "Sending WRITE_RESP msg for file %lu, ack %d", 
				proxyfs_file_get_file_ident(PROXYFS_FILE(real_file)), length );

			if ( peer ) {
				msg = proxyfs_msg_compose_new(MSG_WRITE_RESP, proxyfs_file_get_file_ident(PROXYFS_FILE(real_file)),
					sizeof(u_int32_t), &length, 0 );			
				proxyfs_peer_send_msg( peer, msg );
			}
		}
		// Close file if it should be closed
		if( proxyfs_file_get_status( PROXYFS_FILE(real_file), PROXYFS_FILE_CLOSED ) && 
		    proxyfs_file_get_status( PROXYFS_FILE(real_file), PROXYFS_FILE_ALL_READ )){
			struct file* physical_file = real_file->file;
			mdbg(INFO3, "Closing file %lu. Ref count: %ld",	proxyfs_file_get_file_ident(PROXYFS_FILE(real_file)), file_count(real_file->file) );
			//l = l->next;
			list_del( & PROXYFS_FILE(real_file)->files ); // Delete from list
			proxyfs_real_file_destroy( real_file );
			kfree( real_file );
			// Close file AFTER we de-list proxy file.. fput may actually destroy the file, if we hold last reference
			fput( physical_file );
			continue;
		}
		// read from file
		if ( peer && !proxyfs_file_get_status( PROXYFS_FILE(real_file), PROXYFS_FILE_READ_EOF ) &&
				proxyfs_real_file_read(real_file) == 0 ){ // EOF
			msg = proxyfs_msg_compose_new(MSG_WRITE, proxyfs_file_get_file_ident(PROXYFS_FILE(real_file)), 0);
			proxyfs_peer_send_msg(peer , msg );
			proxyfs_file_set_status( PROXYFS_FILE(real_file), PROXYFS_FILE_READ_EOF );
		}
		// down( & self.files_sem );
	}
	up( & self->files_sem );

}

/** \<\<public\>\> \<\<exported\>\> Release all files
 * taken from current proccess
 */
void proxyfs_server_release_all(void)
{
	struct proxyfs_file_t *file;
	struct list_head *l;

	if ( !self )
		return;

	down( & self->files_sem );
	list_for_each( l, &PROXYFS_TASK(self)->files ){
		file = list_entry(l, struct proxyfs_file_t, files);
		if( proxyfs_real_file_remove_shadow(PROXYFS_REAL_FILE(file),current) == 1 ){ // Did we remove last shadow reference?
			proxyfs_file_set_status( file, PROXYFS_FILE_CLOSED );
			mdbg(INFO2, "Releasing %lu (from task %d)", proxyfs_file_get_file_ident(file), current->pid);
		}
	}
	up( & self->files_sem );
}

EXPORT_SYMBOL_GPL(proxyfs_server_release_all);

static int proxyfs_server_register_poll_callback(struct proxyfs_real_file* file) {
	struct hacked_poll_table table;

	// Poll hack
	INIT_LIST_HEAD(& file->waiters_list );
	table.parent.qproc = proxyfs_server_poll_queue_proc;
	table.real_file = file;

	if (file->file->f_op && file->file->f_op->poll ) {
		file->file->f_op->poll( file->file, (struct poll_table_struct*)&table );
	} else {
		mdbg(ERR3, "Poll unavailable! File op: %p", file->file->f_op);
		if ( file->file->f_vfsmnt)
			mdbg(ERR3, "Mount point: %s", file->file->f_vfsmnt->mnt_devname);	
		return -EINVAL;
	}

	return 0;
}

/** \<\<public\>\> \<\<exported\>\> Make file available trought proxyfs 
 * @param fd - file descriptor of a file, which will be proxied 
 *
 * @return File identifier or null
 */
struct proxyfs_file_identifier* proxyfs_server_overtake_file(int fd){
	struct file *filp;
	struct proxyfs_real_file *file_row;
	static int ident = 1; 
	//struct termios termios_buf;
	mm_segment_t old_fs;

	if ( !self )
		goto exit0;

	filp = fget(fd);
	if( filp == NULL ){
		goto exit0;
	}

	if( S_ISCHR(filp->f_dentry->d_inode->i_mode) ){
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		// Does not work on 64 bit :( => commented out
		//if( ioctl(fd, TCGETS, (unsigned long)&termios_buf) == -ENOTTY )
		//	file_row = PROXYFS_REAL_FILE( proxyfs_generic_real_file_new() );
		//else
			file_row = PROXYFS_REAL_FILE( proxyfs_tty_real_file_new()  );
		set_fs(old_fs);
	}
	else if( S_ISFIFO(filp->f_dentry->d_inode->i_mode) ){
		file_row = PROXYFS_REAL_FILE( proxyfs_pipe_real_file_new() );
	}
	else{
		mdbg(ERR3, "Unsupported file type!");
		goto exit1;
	}

	if( file_row == NULL ){
		mdbg(ERR3, "File allocation failed!");
		goto exit1;
	}

	down( & self->files_sem );

	PROXYFS_FILE(file_row)->file_identifier.file_ident = ++ident;
	PROXYFS_FILE(file_row)->file_identifier.file_owner_pid = task_tgid_vnr(current);
	file_row->file = filp;
	proxyfs_real_file_add_shadow(file_row, current);
	//file_row->shadow_current = current;

	// DO NOT DO THIS! The file structure is (potentially) shared among multiple process and it will change behavior of the other processes!
	//filp->f_flags |= O_NONBLOCK; // Force nonblocking asynchronous mode
	if ( proxyfs_server_register_poll_callback(file_row) )
		goto exit2;

	list_add_tail( & PROXYFS_FILE(file_row)->files, &PROXYFS_TASK(self)->files ); 

	up( & self->files_sem );
	mdbg(INFO3, "Added file %d", ident);

	return &PROXYFS_FILE(file_row)->file_identifier;

exit2:
	proxyfs_real_file_destroy( file_row );
exit1:
	fput(filp);
exit0:
	return NULL;
}

EXPORT_SYMBOL_GPL(proxyfs_server_overtake_file);


static void proxyfs_server_duplicate_overtaken_file(struct proxyfs_real_file* file, struct task_struct* new_owner) {
	struct proxyfs_real_file* clone = NULL;

	if ( file->ops == NULL || file->ops->duplicate == NULL ) {
		mdbg(ERR1, "Cannot duplicate file [%lu], it does not have duplicate method defined", proxyfs_file_get_file_ident(PROXYFS_FILE(file)));
		return;
	}

	clone = file->ops->duplicate(file);
	if ( clone == NULL ) {
		mdbg(ERR1, "Failed to duplicate file [%lu].", proxyfs_file_get_file_ident(PROXYFS_FILE(file)));
		return;
	}

	PROXYFS_FILE(clone)->file_identifier.file_owner_pid = task_tgid_vnr(new_owner);
	proxyfs_real_file_add_shadow(clone, new_owner);

	if ( proxyfs_server_register_poll_callback(clone) )
		goto exit;

	list_add_tail( & PROXYFS_FILE(clone)->files, &PROXYFS_TASK(self)->files ); 

	return;

exit:
	fput(clone->file);
	proxyfs_real_file_destroy(clone);
}

/** 
 * \<\<public\>\> \<\<exported\>\> Duplicates all file that were overtaken by a "parent" process and makes child as a owner of duplicates.
 */
void proxyfs_server_duplicate_all_parent(struct task_struct* parent, struct task_struct* child)
{
	struct proxyfs_file_t *file;
	struct list_head *l, *nxt;

	if ( !self )
		return;

	if ( task_tgid_vnr(parent) == task_tgid_vnr(child) ) {
		mdbg(INFO2, "No duplication performed as tasks are from a same group (task %d in group %d)", child->pid, task_tgid_vnr(child));
		return;
	}

	down( & self->files_sem );
	
	list_for_each_safe( l, nxt, &PROXYFS_TASK(self)->files ) {
		file = list_entry(l, struct proxyfs_file_t, files);
		if( proxyfs_real_file_has_shadow(PROXYFS_REAL_FILE(file),parent) ){
			mdbg(INFO2, "Going to duplicate file %lu (for task %d)", proxyfs_file_get_file_ident(file), child->pid);
			proxyfs_server_duplicate_overtaken_file(PROXYFS_REAL_FILE(file), child);
		}
	}
	up( & self->files_sem );

	mdbg(INFO2, "File duplicating done");
}

EXPORT_SYMBOL_GPL(proxyfs_server_duplicate_all_parent);


/** \<\<private\>\> Method used for waiting on files, callback for poll structure 
 * @param *file - pointer to file in which wait queue this task will be added
 * @param *wait_head - pointer to struct used for queueing in wait queues
 * @param *wait_table - pointer to poll table
 * */
void proxyfs_server_poll_queue_proc(struct file *file, 
		wait_queue_head_t *wait_head, struct poll_table_struct *wait_table)
{
	struct hacked_poll_table *table = (struct hacked_poll_table *)wait_table;
	struct proxyfs_real_file_waiter *waiter;

	waiter = (struct proxyfs_real_file_waiter *)kmalloc( sizeof(struct proxyfs_real_file_waiter), GFP_KERNEL);
	if( waiter == NULL ){
		mdbg(ERR3, "Waiter allocation failed");
		return;
	}

	waiter->wait_head = wait_head;

	// File keeps track of all its waiters in its "waiter_list" member
	list_add( &waiter->waiters_list, &table->real_file->waiters_list );
	init_waitqueue_entry(&waiter->wait, PROXYFS_TASK(self)->tsk);
	// The waiter itself is registered to a file wait queue, this establishing relation between the physical file and the proxy_real_file
	add_wait_queue(wait_head, &waiter->wait);

	mdbg(INFO3, "Server added to wait queue on file %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(table->real_file)));
}

static void proxyfs_server_free(struct proxyfs_task* self) {
	mdbg(INFO1, "Freeing proxy server");
	kkc_sock_put( PROXYFS_SERVER_TASK(self)->server_sock );
};

