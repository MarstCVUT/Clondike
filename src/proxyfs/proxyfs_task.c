/**
 * @file proxyfs_task.c - proxyfs task.
 *                      
 * 
 *
 *
 * Date: 08/12/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_task.c,v 1.10 2008/05/05 19:50:44 stavam2 Exp $
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
#include <linux/jiffies.h>

#define PROXYFS_TASK_PROTECTED
#define PROXYFS_TASK_PRIVATE    // Own header
#include "proxyfs_task.h"

#define PROXYFS_TASK_PROTECTED  // Friend
#include "proxyfs_peer.h"

#define PROXYFS_FILE_PROTECTED  // Friend
#include "proxyfs_file.h"


/** \<\<public\>\> Run new task 
 * @param *func - pointer to thread function (proxyfs_client_thread or proxyfs_server_thread)
 * @param *addr_str - server address for listenning or to connect to
 *
 * @return new task or NULL on error
 */
struct proxyfs_task *proxyfs_task_run( int(*func)(void*), const char *addr_str )
{
	struct proxyfs_task_start_struct start_struct;
	start_struct.addr_str = addr_str;
	init_completion( & start_struct.startup_done );

	if( kthread_run( func, (void*)&start_struct, "ProxyFS %s", addr_str ) == ERR_PTR(-ENOMEM) )
		return NULL;
	wait_for_completion( & start_struct.startup_done );
	return start_struct.task;
}

/** \<\<protected\>\> This is used by thread func, to tell caller, that we are ready with inicialization 
 * @param *self - pointer to self instance or NULL
 * @param *start_struct - proxyfs_thread_start_struct* that was used as thread parameter
 */ 
void proxyfs_task_complete(struct proxyfs_task *self, struct proxyfs_task_start_struct *start_struct)
{
	start_struct->task = self;
	complete( & start_struct->startup_done ); // After this call *start_struct is invalid
}

/** Get reference and increase reference counter 
 * @param *self - pointer to self instance
 * 
 * @return reference to proxyfs_task
 */
struct proxyfs_task* proxyfs_task_get(struct proxyfs_task* task) {
	if ( task )
		atomic_inc(&task->ref_count);

	return task;
}

/** Releases reference and if it was the last reference, frees the task 
 * @param *self - pointer to self instance
 *
 * */
void proxyfs_task_put(struct proxyfs_task* task) {
	if ( !task )
		return;

	if (atomic_dec_and_test(&task->ref_count)) {
		mdbg(INFO3, "Destroying proxy task: %p", task);
		proxyfs_task_free(task);
		kfree(task);
	}
}

/** \<\<public\>\> Initialize task 
 * @param *self - pointer to self instance
 *
 * @return zero on success
 * */
int proxyfs_task_init(struct proxyfs_task *self)
{
	INIT_LIST_HEAD( & self->files );
	INIT_LIST_HEAD( & self->peers );
	self->ops = NULL;
	atomic_set(&self->ref_count, 1);

	self->data_ready = 0;
	spin_lock_init(&self->data_ready_lock);
	init_waitqueue_head( & self->data_ready_wait );

	mdbg(INFO3, "Created proxy task: %p Current: %p", self, current);

	return 0;
}

/** \<\<public\>\> Notifies task that there are data ready (either for writing/reading/both) */
void proxyfs_task_notify_data_ready(struct proxyfs_task *self) {	
	unsigned long flags;

	spin_lock_irqsave(&self->data_ready_lock, flags);
	self->data_ready = 1;
	spin_unlock_irqrestore(&self->data_ready_lock,flags);
	
	wake_up( &self->data_ready_wait );
	mdbg(INFO3, "Task: %p Data ready done", self);
}

/** Callback function that should be registered to all associated sockets. It will notify task about pending data to be read */
void proxyfs_task_data_ready_callback(void* data, int bytes) {
	struct proxyfs_task* self = data;
	
	mdbg(INFO3, "Task: %p Data arrived: %d", self, bytes);
	proxyfs_task_notify_data_ready(self);
}

/**
 * This method will wait till there are some data ready to be read, or the thread is requested to stop.
 */
void proxyfs_task_wait_for_data_ready(struct proxyfs_task* self) {
	unsigned long flags;
	// TODO: There is 100ms timeout.. it seems we are missing some events still.. I suspect it is file data ready event
	// When we solve this, remove this timeout, it should not be required.. but can be kept as a "fallback" bulgarian fix
	wait_event_timeout( self->data_ready_wait, self->data_ready || kthread_should_stop(), msecs_to_jiffies(100));

	// Note: Here we assume, that all already arrived data will be read before next wait call. 
	// If this is not the case, the mechanism must be changed
	spin_lock_irqsave(&self->data_ready_lock, flags);
	self->data_ready = 0;
	spin_unlock_irqrestore(&self->data_ready_lock, flags);
}

/** \<\<protected\>\> Releases task files.. in fact they all should be already release we do just a consistency check at this place
 * @param *self - pointer to self instance
 */
void proxyfs_task_release_files(struct list_head *files) {
	struct list_head *l;
	struct proxyfs_file_t *proxy_file;

	list_for_each( l, files ) {
		proxy_file = list_entry(l, struct proxyfs_file_t, files);

		if ( !proxyfs_file_get_status( proxy_file, PROXYFS_FILE_CLOSED ) ) {
			mdbg(WARN3, "Proxy file not closed on release: %lu", proxyfs_file_get_file_ident(proxy_file));
		}
		
	}	
}


/** \<\<protected\>\> Releases task resources 
 * @param *self - pointer to self instance
 */
void proxyfs_task_free(struct proxyfs_task* self) {
	struct list_head *l, *nxt;
	struct proxyfs_peer_t *peer;

	// First stop the processing thread
	int result;
	if ( current == self->tsk ) { // Special case when we are invoked from the proxyfs task thread directly
		result = -EINVAL;
		mdbg(INFO3, "Failed to startup proxyfs task listening => release it");
	} else {
		mdbg(INFO3, "Proxy task requesting thread stop");
		result = kthread_stop(self->tsk);
	}

	mdbg(INFO3, "Proxy task kthread stop result: %d Task: %p", result, self->tsk);

	if ( result )
		return;

	// Process custom freeing	
	if ( self->ops && self->ops->free )
		self->ops->free(self);

	// TODO: We can do this also in client free for open&open send files, right?
	proxyfs_task_release_files(&self->files);

	// Finally free all peers (we do this after custom free as custom free will first stop possible server listening)
	list_for_each_safe( l, nxt, &self->peers ){
		peer = list_entry(l, struct proxyfs_peer_t, peers);
		proxyfs_peer_disconnect(peer);
		proxyfs_peer_put(peer);	
	}
}


/** \<\<protected\>\> Try to receive data from all peers 
 * @param *self - pointer to self instance
 */
void proxyfs_task_recv_peers(struct proxyfs_task *self)
{
	struct list_head *l;
	struct proxyfs_peer_t *peer;
	int recv_res;

	list_for_each( l,  &self->peers ){
		peer = list_entry(l, struct proxyfs_peer_t, peers);
recv_next:	recv_res = proxyfs_peer_real_recv(peer);
		if ( recv_res == 1 ){ // Message received
			mdbg(INFO3, "Message received");
			proxyfs_task_handle_msg( self, peer );
			proxyfs_peer_clear_msg(peer);
			// Consider adding protection from receiving from only one peer
			goto recv_next;
		} else if ( recv_res == -ECONNRESET || recv_res == -ECONNABORTED ) {
			// TODO: Some nicer general solution for removing (and detecting) lost peers.. but for now we need at least this
			proxyfs_peer_set_state(peer, PEER_DEAD);
			mdbg(INFO1, "Peer marked dead");
		} else if (recv_res == -EAGAIN ) {
			// Do nothing, repeat
		} else {
			mdbg(INFO1, "Unknown return code: %d", recv_res);
		}

	}
}

/** \<\<protected\>\> Try to send data to all peers 
 * @param *self - pointer to self instance
 */
void proxyfs_task_send_peers(struct proxyfs_task *self)
{
	struct list_head *l;
	struct proxyfs_peer_t *peer;

	list_for_each( l, &self->peers ){
		peer =  list_entry(l, struct proxyfs_peer_t, peers);
		proxyfs_peer_real_send(peer);
	}
}

/**
 * Iterates over all peers and if the peer is marked as dead, handle its removal
 *
 */
void proxyfs_task_handle_dead_peers(struct proxyfs_task *self) {
	struct list_head *l, *tmp;
	struct proxyfs_peer_t *peer;
	//int recv_res;

	list_for_each_safe( l, tmp, &self->peers ){
		peer = list_entry(l, struct proxyfs_peer_t, peers);
		if ( proxyfs_peer_get_state(peer) == PEER_DEAD ) {
			mdbg(INFO2, "Removing dead peer %s (%p)", kkc_sock_getpeername2(peer->sock), peer);
			list_del(l);
			proxyfs_peer_disconnect(peer);
			if ( self->ops && self->ops->handle_peer_dead )
				self->ops->handle_peer_dead(self, peer);			
			proxyfs_peer_put(peer);				
		}
	}
};

/** \<\<protected\>\> Handle receive messagge 
 * @param *self - pointer to self instance
 * @param *peer - peer instance with message we wanted to handle
 */
void proxyfs_task_handle_msg(struct proxyfs_task *self, struct proxyfs_peer_t *peer)
{
	struct proxyfs_file_t *file;
	struct proxyfs_msg *msg = proxyfs_peer_get_msg(peer);
	if ( !msg ) // Case, when we had incomplete message in the buffer
		return;

	mdbg(INFO1, "Msg for identifier has arrived: %lu",msg->header.file_ident);

	switch( msg->header.msg_num ){
		case MSG_WRITE_RESP:
			if(( file = proxyfs_task_find_file(self, msg->header.file_ident)) != NULL){
				if( msg->header.data[0] <= 0 ){
					mdbg(ERR3, "Read must be > 0 but it is %lu", (unsigned long)msg->header.data[0]);
					break;
				}
				else{
					mdbg(INFO3, "Peer reads %lu bytes from file %lu buffer", 
							(unsigned long)msg->header.data[0], proxyfs_file_get_file_ident(file));
					/*if( self->ops && self->ops->file_read )
						self->ops->file_read(file, msg->header.data[0]);*/
					proxyfs_file_submit_read_to_peer( file, msg->header.data[0]);
					break;
				}
			}
			mdbg(ERR3, "WRITE_RESP: Unknown file identificator %lu", msg->header.file_ident);
			break;
		case MSG_WRITE:
			if(( file = proxyfs_task_find_file(self, msg->header.file_ident)) != NULL){
				mdbg(INFO3, "Peer writes %lu bytes to file %lu", msg->header.data_size, proxyfs_file_get_file_ident(file));

				if( msg->header.data_size >= 0 ){
					proxyfs_file_write_from_peer( file, (void*)msg->header.data, msg->header.data_size);
				}
				break;
			}
			mdbg(ERR3, "WRITE: Unknown file identificator %lu", msg->header.file_ident);
			break;
		default:
			mdbg(INFO3, "Calling task specific msg handler");
			if( self->ops && self->ops->handle_msg )
				self->ops->handle_msg(self, peer);
	}

	// Do not free message here. Incoming messages are kept only in rcv buffer and the buffer is always reused, so no need to free
}

/** \<\<protected\>\> Try to create new messages from data in all file buffers
 * @param *self - pointer to self instance
 */
void proxyfs_task_send_buffers(struct proxyfs_task *self)
{
	struct proxyfs_file_t *proxy_file;
	struct list_head *l;
	int res;

	list_for_each( l, &self->files ){
		proxy_file = list_entry(l, struct proxyfs_file_t, files);

		// Send as long as there are some data
		// We can consider some "starvation" prevention here. In this case, we have to notify task about "data_ready" if we do not send all data, so that we ensure a next iteration in a main processing loop is started
		while ( (res = proxyfs_task_send_write_buf( self, proxy_file )) == -EAGAIN ) {};
	}
}


/** \<\<protected\>\> Try to create new messages from data in buffer
 * @param *self - pointer to self instance
 * @param *file - proxyfs_file instance which data we wanted to send
 * @return 0, if all required data were written, -EAGAIN if there are still some pending data
 */
int proxyfs_task_send_write_buf(struct proxyfs_task *self, struct proxyfs_file_t *file)
{
	struct proxyfs_msg *msg;
	size_t total_size, data_size, send;
	void *data_ptr;
	char* read_buffer = NULL;
	int res = 0;

        mdbg(INFO3, "Proccessing file %lu", proxyfs_file_get_file_ident(file));

	if( file->peer == NULL ){
        	mdbg(INFO3, "No peer, skiping");
		return 0;
	}

	proxyfs_file_write_buf_info(file, &total_size, &data_size, &data_ptr);
        mdbg(INFO3, "(Total size %lu, Data size %lu, Data addr %p, Unconfirmed size %lu)", (unsigned long)total_size, (unsigned long)data_size, data_ptr, (unsigned long)file->write_buf_unconfirmed);

	send = total_size - file->write_buf_unconfirmed;
	if ( send == 0 )
		return 0;

	if( send > MSG_MAX_SIZE - MSG_HDR_SIZE ) {
		send = MSG_MAX_SIZE - MSG_HDR_SIZE;
		res = -EAGAIN;
	}

	read_buffer = kmalloc(send, GFP_KERNEL);
	if ( !read_buffer )
		goto exit0;

	send = proxyfs_file_read_to_peer(file, read_buffer, send);
	mdbg(INFO3, "Real send size: %lu", (unsigned long)send);
	if ( send == 0 ) 
		goto exit0;

	msg = proxyfs_msg_new_takeover_ownership(MSG_WRITE, proxyfs_file_get_file_ident(file), send, read_buffer);
	if ( !msg )
		goto exit0;
		
	proxyfs_peer_send_msg( file->peer, msg );

	return res;

exit0:
	kfree(read_buffer);
	return -ENOMEM;
/*
	

        mdbg(INFO3, "(Data size %lu, Data addr %p, Unconfirmed size %lu)", (unsigned long)data_size, data_ptr, (unsigned long)file->write_buf_unconfirmed);

	if( data_size > file->write_buf_unconfirmed ){
		if( data_size - file->write_buf_unconfirmed > MSG_MAX_SIZE - MSG_HDR_SIZE )
			send = MSG_MAX_SIZE - MSG_HDR_SIZE;
		else
			send = data_size - file->write_buf_unconfirmed;
        	mdbg(INFO3, "Data pending for file %lu (%d bytes at %p)(%d)", 
				file->file_ident, data_size, data_ptr, file->write_buf_unconfirmed);

		msg = proxyfs_msg_new(MSG_WRITE, file->file_ident, send, data_ptr + file->write_buf_unconfirmed );

        	mdbg(INFO3, "Message created %p", msg); 
		if ( file->peer ) {
			proxyfs_peer_send_msg( file->peer, msg );
			file->write_buf_unconfirmed += send;
		} else {
			mdbg(INFO3, "Msg ignored, peer NULL"); // TODO: Where is the msg free?
		}		
	}
*/
}

/** \<\<public\>\> Wake up task 
 * @param *self - pointer to self instance
 */
void proxyfs_task_wakeup(struct proxyfs_task *self){
	if( current == self->tsk )
		set_current_state(TASK_RUNNING);
	else
		wake_up_process(self->tsk);
}

/** \<\<private\>\> Create new peer from socket and add it to peers list
 * @param *self - pointer to self instance
 * @param *peer_sock - peer socket instance
 *
 * @return new proxyfs_peer instance or NULL on error
 */
struct proxyfs_peer_t* proxyfs_task_add_to_peers(struct proxyfs_task *self, struct kkc_sock *peer_sock){
	struct proxyfs_peer_t *peer;
	
	if((peer = proxyfs_peer_new() ) == NULL){
                mdbg(ERR2, "Couldn't add socket to peers");
		return NULL;
	}

	peer->sock = peer_sock;
	list_add( &peer->peers, &self->peers );

	return peer;
}

struct proxyfs_file_t *proxyfs_task_find_file(struct proxyfs_task *task, unsigned long ident) 
{
	struct proxyfs_file_t *file = NULL;       
	struct list_head *l;                      
	list_for_each( l, &((task)->files) ){
		mdbg(INFO3, "Checking file with id %lu", proxyfs_file_get_file_ident(list_entry(l, struct proxyfs_file_t, files)) );
		if( proxyfs_file_get_file_ident(list_entry(l, struct proxyfs_file_t, files)) == (ident) ){ 
			file = list_entry(l, struct proxyfs_file_t, files); 
			break;			  
		}                                 
	}                                         
	return file;
}
