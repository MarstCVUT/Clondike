/**
 * @file proxyfs_server.h - Proxyfs server task.
 *                      
 * 
 *
 *
 * Date: 08/12/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_server.h,v 1.6 2007/10/20 14:24:20 stavam2 Exp $
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
#ifndef _PROXYFS_SERVER_H_PUBLIC
#define _PROXYFS_SERVER_H_PUBLIC

/** @defgroup proxyfs_server_class proxyfs_server class 
 * 
 * @ingroup proxyfs_task_class
 *
 * \<\<singleton\>\> representing proxyfs_server task
 * 
 * @{
 *
 */

#include <proxyfs/proxyfs_task.h>

/** \<\<public\>\> "Steal" file to server */
struct proxyfs_file_identifier* proxyfs_server_overtake_file(int fd);

/** \<\<public\>\> \<\<exported\>\> Release all files */
void proxyfs_server_release_all(void);

/** 
 * \<\<public\>\> \<\<exported\>\> Duplicates all file that were overtaken by a "parent" process and makes child as a owner of duplicates.
 */
void proxyfs_server_duplicate_all_parent(struct task_struct* parent, struct task_struct* child);

/** \<\<public\>\> Main server thread */
int proxyfs_server_thread(void *start_struct);


#endif // _PROXYFS_SERVER_H_PUBLIC

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef PROXYFS_SERVER_PRIVATE
#ifndef _PROXYFS_SERVER_H_PRIVATE
#define _PROXYFS_SERVER_H_PRIVATE

#include <linux/semaphore.h>
#include <proxyfs/proxyfs_msg.h>
#include <proxyfs/proxyfs_peer.h>
#include <linux/poll.h>

#define PROXYFS_REAL_FILE_PROTECTED   // Friend
#include <proxyfs/proxyfs_real_file.h>

#define PROXYFS_TASK_PROTECTED        // Parent
#include <proxyfs/proxyfs_task.h>

#define PROXYFS_SERVER_TASK(arg) ((struct proxyfs_server*)(arg))

//#include <linux/sched.h>
//#include <linux/list.h>


/** \<\<private\>\> Structure representing server */
struct proxyfs_server {
	/** Parent structure */
	struct proxyfs_task parent;
	/** This semaphore prevents race conditions between server task and task adding 
	 * file under server control */
	struct semaphore files_sem;
	/** Server is listening on this socket */
	struct kkc_sock *server_sock;
	/** For waiting on server socket */
	wait_queue_t server_sock_wait;
};

/** \<\<private\>\> Structure used to queueing on file wait queue */
struct hacked_poll_table {
	/** Parent kernel struct */
	struct poll_table_struct parent;
	/** Our data - corresponding real file instance */
	struct proxyfs_real_file *real_file;
};

/** \<\<private\>\> Writes data from buffer to file */
void proxyfs_server_write_file_buf(struct proxyfs_real_file *file_row);

/** \<\<private\>\> Start listening */
int proxyfs_server_listen(const char *listen_str);

/** \<\<private\>\> */
int proxyfs_server_add_to_peers(struct kkc_sock *peer_sock);

/** \<\<private\>\> Accept incoming connection (nonblocking) */
int proxyfs_server_accept(void);

/** \<\<private\>\> Main server loop */
void proxyfs_server_loop(void);

/** \<\<private\>\> Handle received message */
static void proxyfs_server_handle_msg(struct proxyfs_task *self, struct proxyfs_peer_t *peer);

/** \<\<private\>\> Try to write all write bufers to files (nonblocking) */
void proxyfs_server_write_buffers(void);

/** \<\<private\>\> Read and write from files */
void proxyfs_server_read_and_write_real_files(void);

static void proxyfs_server_free(struct proxyfs_task* self);

/** \<\<private\>\> Method used for waiting on files, callback for poll structure */
void proxyfs_server_poll_queue_proc(struct file *file, wait_queue_head_t *wait_head, struct poll_table_struct *wait_table);

/** <\<\private\>\> Structure with pointers to virtual methods */
static struct proxyfs_task_ops server_ops = {
	/** Handle received message */
	.handle_msg = proxyfs_server_handle_msg,
	.handle_peer_dead = NULL,
	/** Free task resources */
	.free = proxyfs_server_free,
};

#endif //  _PROXYFS_SERVER_H_PRIVATE
#endif // PROXYFS_SERVER_PRIVATE

/**
 * @}
 */


