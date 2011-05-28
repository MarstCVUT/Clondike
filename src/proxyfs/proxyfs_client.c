/**
 * @file proxyfs_client.c - proxyfs_client task.
 *                      
 * 
 *
 *
 * Date: 08/12/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_client.c,v 1.10 2009-04-06 21:48:46 stavam2 Exp $
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
#include <asm/uaccess.h>
#include <linux/fs.h>

#define PROXYFS_CLIENT_PROTECTED       // Own header
#define PROXYFS_CLIENT_PRIVATE       // Own header
#include "proxyfs_client.h"

#define PROXYFS_PROXY_FILE_PROTECTED // Friend
#include "proxyfs_proxy_file.h"
#include "proxyfs_generic_proxy_file.h"
#include "proxyfs_pipe_proxy_file.h"

/** \<\<private\>\> Proxyfs client constructor 
 * It's used by proxyfs_client_thread for data allocation and initialization 
 *
 * @return new proxyfs client class instance 
 **/
struct proxyfs_client_task* proxyfs_client_new(void)
{
	struct proxyfs_client_task *self;

	self = (struct proxyfs_client_task*)kmalloc(sizeof(struct proxyfs_client_task), GFP_KERNEL);
	if( self == NULL ){
		mdbg(ERR2, "Allocating struct proxyfs_client failed");
		return NULL;
	}

	proxyfs_task_init( PROXYFS_TASK(self) );

	PROXYFS_TASK(self)->ops = &client_ops;

	INIT_LIST_HEAD( & self->try_open );
	INIT_LIST_HEAD( & self->try_open_send );
	init_MUTEX( & self->try_open_sem );
	init_MUTEX( & self->files_sem );

	return self;
}

/** \<\<public\>\> This function is used as startup function for client thread
 * @param start_struct - proxyfs task startup structure
 */
int proxyfs_client_thread(void *ss)
{
	struct proxyfs_client_task *self;
	struct proxyfs_task_start_struct *start_struct = (struct proxyfs_task_start_struct *)ss;

	if( (self = proxyfs_client_new()) == NULL )
		goto exit0;

	PROXYFS_TASK(self)->tsk = current;

	//init_waitqueue_entry( &self.socket_wait, current );

	if( (self->server = proxyfs_peer_new()) == NULL) {
		mdbg(ERR2, "Failed to create a peer");
		goto exit1;
	}
	
	mdbg(INFO3, "Proxy fs client going to connect");

	if( proxyfs_peer_connect( self->server, start_struct->addr_str) != 0 ) {
		mdbg(ERR2, "Failed to connect a peer");
		goto exit2;
	}

	kkc_sock_register_read_callback(self->server->sock, proxyfs_task_data_ready_callback, self);

	mdbg(INFO3, "Proxy fs client connection wait");

	if( proxyfs_peer_wait( self->server ) != 0 ) {
		mdbg(ERR2, "Failed to wait for a peer");
		goto exit3;
	}

	mdbg(INFO3, "Proxy fs client connected succesfully");

	list_add(&self->server->peers, &PROXYFS_TASK(self)->peers );

	proxyfs_task_complete( PROXYFS_TASK(self), start_struct ); // After this call *start_struct is invalid

	proxyfs_client_loop(self);
	return 0;

exit3:
	proxyfs_peer_disconnect(self->server);
exit2:
	proxyfs_peer_put(self->server);
exit1:
	proxyfs_task_put(PROXYFS_TASK(self));
exit0:
	mdbg(ERR2, "Failed to start client thread");
	proxyfs_task_complete( NULL, start_struct );
	return -EINVAL;
}

/** \<\<private\>\> Main client loop 
 * @param self - pointer to proxyfs_client_task instance
 **/
void proxyfs_client_loop(struct proxyfs_client_task* self)
{
	while(!kthread_should_stop()){		
		proxyfs_task_wait_for_data_ready(PROXYFS_TASK(self));
		
		down( & self->files_sem); // TODO: It may be nicer to make it more granual and lock only when needed, right? ;) At least add & remove must be locked
		proxyfs_client_open_files( self );
		proxyfs_task_send_buffers( PROXYFS_TASK(self) );
		proxyfs_peer_real_send( self->server );
		proxyfs_task_recv_peers( PROXYFS_TASK(self) );
		up( & self->files_sem);

		proxyfs_task_handle_dead_peers( PROXYFS_TASK(self) );		
		
		if ( !self->server ) {
		    mdbg(ERR2, "Terminating client loop thread, peer is marked as dead");
		    break;
		}		  
	}
}

/** \<\<private\>\> Send open message for all files submited to open since
 * last execution of this method
 *
 * @param self - pointer to proxyfs_client_task instance
 * */
void proxyfs_client_open_files(struct proxyfs_client_task* self)
{
	struct proxyfs_file_t *proxy_file;
	struct proxyfs_msg *msg;
	struct list_head *p, *l;

	down( & self->try_open_sem ); // Serialize access
	list_for_each( l, &self->try_open ){
		// up( & self.try_open_sem ); - list wouldn't be big

		p = l->prev;
		proxy_file = list_entry(l, struct proxyfs_file_t, files);
		
		msg = proxyfs_msg_new(MSG_OPEN, proxyfs_file_get_file_ident(proxy_file), 0, NULL );
		
		proxyfs_peer_send_msg( self->server, msg );
		
		list_move_tail(l, &self->try_open_send);
		l = p;

		// down( & self.try_open_sem );
	}
	up( & self->try_open_sem );
}

/** \<\<private\>\> Helper method that retrieves file by id either from open files or from "standard" files */
static struct proxyfs_file_t* get_proxy_file_by_id(struct proxyfs_task *self, unsigned long identifier) {
	struct proxyfs_file_t* proxy_file = NULL;

	// First check in open files
	proxy_file = proxyfs_file_find_in_list( identifier, &self->files );

	if ( proxy_file == NULL ) // If not found in open files try in try open files
		proxy_file = proxyfs_file_find_in_list( identifier, &PROXYFS_CLIENT_TASK(self)->try_open_send );

	return proxy_file;
}

/** \<\<private\>\> Handle message specific for client
 * @param self - pointer to proxyfs_client_task instance
 * @param peer - peer struct with received message
 * */
void proxyfs_client_handle_msg(struct proxyfs_task *self, struct proxyfs_peer_t *peer)
{
	struct proxyfs_file_t *proxy_file;
	struct proxyfs_msg *msg = proxyfs_peer_get_msg(peer);

	switch( msg->header.msg_num ){
		case MSG_OPEN_RESP_OK:
			proxy_file = proxyfs_file_find_in_list( msg->header.file_ident, 
					&PROXYFS_CLIENT_TASK(self)->try_open_send );
			if( proxy_file != NULL ){
				list_move_tail( &proxy_file->files, &self->files ); // Move to opened files
				switch( msg->header.data[0] ){
					case PROXYFS_FILE_TTY:
					case PROXYFS_FILE_GENERIC:
						mdbg(INFO2, "OPEN_OK: %lu is generic file or tty", 
								msg->header.file_ident);
						proxyfs_generic_proxy_file_from_proxy_file( 
								PROXYFS_PROXY_FILE(proxy_file) );
						break;
					case PROXYFS_FILE_PIPE:
						mdbg(INFO2, "OPEN_OK: %lu is pipe", msg->header.file_ident);
						proxyfs_pipe_proxy_file_from_proxy_file( 
								PROXYFS_PROXY_FILE(proxy_file) );
						break;
					default:
						mdbg(ERR3, "OPEN_OK: Unknown file type, trying generic");
						proxyfs_generic_proxy_file_from_proxy_file( 
								PROXYFS_PROXY_FILE(proxy_file) );
				}
				proxyfs_file_set_peer(proxy_file, peer);
				proxyfs_file_set_status( proxy_file, PROXYFS_FILE_OPENED );
				wake_up( & PROXYFS_PROXY_FILE(proxy_file)->waiting_procs );
				return;
			}
			mdbg(ERR3, "OPEN_OK: Unknown file identificator");
			break;
		case MSG_OPEN_RESP_FAILED:
			proxy_file = proxyfs_file_find_in_list( msg->header.file_ident, 
					&PROXYFS_CLIENT_TASK(self)->try_open_send );
			if( proxy_file != NULL ){
				list_del( & proxy_file->files ); // Delete from list
				
				proxyfs_file_set_status( proxy_file, PROXYFS_FILE_DONT_EXISTS  );
				wake_up( & PROXYFS_PROXY_FILE(proxy_file)->waiting_procs );
				return;
			}
			mdbg(ERR3, "OPEN_FAILED: Unknown file identificator");
			break;
		case MSG_IOCTL_RESP:
			proxy_file = get_proxy_file_by_id(self, msg->header.file_ident);

			if( proxy_file != NULL ){
				*(PROXYFS_PROXY_FILE(proxy_file)->syscall_resp) = 
					kmalloc( proxyfs_msg_get_size(msg), GFP_KERNEL );
			       	if( *(PROXYFS_PROXY_FILE(proxy_file)->syscall_resp) != NULL ){
					memcpy(*(PROXYFS_PROXY_FILE(proxy_file)->syscall_resp), msg, 
							proxyfs_msg_get_size(msg)); 
					PROXYFS_PROXY_FILE(proxy_file)->syscall_resp = NULL;
				}
				proxyfs_file_unset_status( proxy_file, PROXYFS_FILE_IN_SYSCALL );
				wake_up( & PROXYFS_PROXY_FILE(proxy_file)->waiting_procs );
				return;
			}
			mdbg(ERR3, "IOCTL_RESP: Unknown file identificator");
			break;
		case MSG_FSYNC_RESP:
			proxy_file = get_proxy_file_by_id(self, msg->header.file_ident);

			if( proxy_file != NULL ){
				*(PROXYFS_PROXY_FILE(proxy_file)->syscall_resp) = (void*)MSG_FSYNC_RESP;
				PROXYFS_PROXY_FILE(proxy_file)->syscall_resp = NULL;
				proxyfs_file_unset_status( proxy_file, PROXYFS_FILE_IN_SYSCALL );
				// Why would we want to remove from list here??? list_del( & proxy_file->files ); // Delete from list
				wake_up( & PROXYFS_PROXY_FILE(proxy_file)->waiting_procs );
				return;
			}
			mdbg(ERR3, "FSYNC_RESP: Unknown file identificator");
			break;
		case MSG_READ_REQUEST_RESP:
			proxy_file = get_proxy_file_by_id(self, msg->header.file_ident);
		
			if( proxy_file != NULL ){
				*(PROXYFS_PROXY_FILE(proxy_file)->syscall_resp) = (void*)MSG_READ_REQUEST_RESP;
				PROXYFS_PROXY_FILE(proxy_file)->syscall_resp = NULL;
				proxyfs_file_unset_status( proxy_file, PROXYFS_FILE_IN_SYSCALL );
				wake_up( & PROXYFS_PROXY_FILE(proxy_file)->waiting_procs );
				return;
			}
			mdbg(ERR3, "READ_REQUEST_RESP: Unknown file identificator: %lu", msg->header.file_ident);
			break;
		case MSG_CLOSE_RESP:
			proxy_file = get_proxy_file_by_id(self, msg->header.file_ident);
		
			if( proxy_file != NULL ){
				*(PROXYFS_PROXY_FILE(proxy_file)->syscall_resp) = (void*)MSG_CLOSE_RESP;
				PROXYFS_PROXY_FILE(proxy_file)->syscall_resp = NULL;
				proxyfs_file_unset_status( proxy_file, PROXYFS_FILE_IN_SYSCALL );
				list_del( & proxy_file->files ); // Delete from list
				wake_up( & PROXYFS_PROXY_FILE(proxy_file)->waiting_procs );
				return;
			}
			mdbg(ERR3, "CLOSE_RESP: Unknown file identificator: %lu", msg->header.file_ident);
			break;

		default:
			mdbg(ERR3, "Unknown msg number");
	}

}

/** \<\<public\>\> Open new proxyfile - this is used by proxy_fs class
 * @param self - pointer to proxyfs_client_task instance
 * @param inode_num - unique file identification number
 *
 * @return Proxyfile or err ptr
 */
struct proxyfs_proxy_file_t *proxyfs_client_open_proxy(struct proxyfs_client_task* self, unsigned long inode_num){
	struct proxyfs_proxy_file_t *proxy_file;

	down( & self->files_sem);
	proxy_file = PROXYFS_PROXY_FILE(get_proxy_file_by_id(PROXYFS_TASK(self), inode_num));
	up( & self->files_sem);

	if ( proxy_file ) {
		mdbg(ERR3, "File is already open by the same task in this proxy client => cannot open once more!");
		proxy_file = ERR_PTR(-EACCES);
		goto exit0;		
	}

	proxy_file = (struct proxyfs_proxy_file_t *)kmalloc(sizeof(struct proxyfs_proxy_file_t), GFP_KERNEL);
	if( proxy_file == NULL ){
		mdbg(ERR3, "Allocating memory for proxy_file failed");
		proxy_file = ERR_PTR(-ENOMEM);
		goto exit0;
	}

	proxyfs_proxy_file_init(proxy_file); 
	proxyfs_file_set_file_ident(PROXYFS_FILE(proxy_file), inode_num);
	proxy_file->task = self;

	down( & self->try_open_sem );
	list_add_tail( & PROXYFS_FILE(proxy_file)->files, & self->try_open );
	up( & self->try_open_sem );

	mdbg(INFO3, "Proxyfile (file_ident=%lu) created and added to open queue", inode_num);
	//proxyfs_task_wakeup(PROXYFS_TASK(self));
	proxyfs_task_notify_data_ready(PROXYFS_TASK(self));

	wait_event( proxy_file->waiting_procs, proxyfs_file_get_status( PROXYFS_FILE(proxy_file),
			       PROXYFS_FILE_OPENED | PROXYFS_FILE_DONT_EXISTS ));

	if( proxyfs_file_get_status( PROXYFS_FILE(proxy_file), PROXYFS_FILE_OPENED )){
		mdbg(INFO3, "File opened");
		return proxy_file;
	}	

	mdbg(INFO3, "File does not exists");
	proxyfs_file_destroy(PROXYFS_FILE(proxy_file)); 

	kfree( proxy_file );

	proxy_file = ERR_PTR(-ENOENT);
exit0:
	return proxy_file;
}

static int check_syscall_done(struct proxyfs_proxy_file_t *file, struct proxyfs_peer_t* peer) {
	int cond = ( !proxyfs_file_get_status( PROXYFS_FILE(file), PROXYFS_FILE_IN_SYSCALL) || 
          (proxyfs_peer_get_state(peer) != PEER_CONNECTED));	

	mdbg(INFO3, "File cond: %d.. ident: %lu... peer state: %d", cond, proxyfs_file_get_file_ident(PROXYFS_FILE(file)), proxyfs_peer_get_state(peer) );

	return cond;
};

/** \<\<public\>\> Forward a syscall on file 
 * @param *self - pointer to proxyfs_client_task instance
 * @param *file - pointer to proxy file. System call will by called on coresponding real file.
 * @param *msg  - pointer t system call messagge
 *
 * @return response messagge or NULL on error
 */
struct proxyfs_msg *proxyfs_client_do_syscall(struct proxyfs_client_task *self, 
		struct proxyfs_proxy_file_t *file, struct proxyfs_msg *msg)
{
	struct proxyfs_msg *msg_resp = NULL;
	struct proxyfs_peer_t* peer;

	peer = PROXYFS_FILE(file)->peer;
	file->syscall_resp = &msg_resp;
	// TODO: Does this handle case of multiple simultaneous sys_calls? Not likely..
	proxyfs_file_set_status( PROXYFS_FILE(file), PROXYFS_FILE_IN_SYSCALL );
	proxyfs_peer_send_msg( peer , msg );	
	proxyfs_task_notify_data_ready(PROXYFS_TASK(self));
	
	wait_event( file->waiting_procs, check_syscall_done(file, peer) );

	mdbg(INFO3, "File syscall finished");

	return msg_resp;
}

/** \<\<public\>\> Asynchronously sends a message */
void proxyfs_client_send_message(struct proxyfs_client_task *self, 
		struct proxyfs_proxy_file_t *file, struct proxyfs_msg *msg) {
	struct proxyfs_peer_t* peer;

	peer = PROXYFS_FILE(file)->peer;
	proxyfs_peer_send_msg( peer , msg );	
}


static void proxyfs_client_handle_dead_peer(struct proxyfs_task* self, struct proxyfs_peer_t* peer) {
	struct proxyfs_file_t *proxy_file;
	struct list_head *tmp, *l;

	mdbg(INFO1, "Client handling dead peer");

	// We cannot release proxy files directly in here as they are held by other processes
	// Instead, we just wake them all up in order to prevent all waiter to be stuck
	// All following calls to the files won't block, because state of the peer won't be connected any more
	list_for_each_safe( l, tmp, &self->files ) {
		proxy_file = list_entry(l, struct proxyfs_file_t, files);

		if ( proxy_file->peer == peer ) {
			list_del( &proxy_file->files );
			mdbg(INFO3, "Waking up file: %lu", proxyfs_file_get_file_ident(proxy_file));
			wake_up( & PROXYFS_PROXY_FILE(proxy_file)->waiting_procs );
		}
		
	}
	
	if ( PROXYFS_CLIENT_TASK(self)->server == peer ) {
	    PROXYFS_CLIENT_TASK(self)->server = NULL;
	} else {
	    mdbg(ERR1, "Unexpected peer death being handled. Server was %p, but peer %p", PROXYFS_CLIENT_TASK(self)->server, peer);
	}
	  

	mdbg(INFO1, "Client handling dead peers done");
}

static void proxyfs_client_free(struct proxyfs_task* self) {
	mdbg(INFO1, "Freeing proxy client");

/* Do not free peer direcly, it will be freed while freeing all peers of task
	if ( client_self->server ) {
		proxyfs_peer_disconnect(client_self->server);
		proxyfs_peer_destroy(client_self->server);
	}	
*/
	mdbg(INFO1, "Specific freeing proxy client finished");
}
