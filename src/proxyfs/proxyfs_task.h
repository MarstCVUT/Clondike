/**
 * @file proxyfs_task.h - Proxyfs task class.
 *                      
 * 
 *
 *
 * Date: 08/12/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_task.h,v 1.4 2007/10/07 15:54:00 stavam2 Exp $
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

#include <linux/kthread.h> 
#include <linux/completion.h> 
#include <linux/spinlock.h>
#ifndef _PROXYFS_TASK_H_PUBLIC
#define _PROXYFS_TASK_H_PUBLIC

/** @defgroup proxyfs_task_class proxyfs_task class 
 * 
 * @ingroup proxyfs_module_class
 *
 * \<\<class\>\> representing proxyfs task
 * 
 * @{
 *
 */

/** \<\<public\>\> Task declaration */
struct proxyfs_task;

/** \<\<public\>\> Initialize task */
int proxyfs_task_init(struct proxyfs_task *self);

/** \<\<public\>\> Task declaration */
struct proxyfs_task_start_struct; 

/** \<\<public\>\> Cast to struct proxyfs_task* */
#define PROXYFS_TASK(arg) ((struct proxyfs_task*)(arg))

/** \<\<public\>\> Wake up client TODO: Obsolete, remove*/
void proxyfs_task_wakeup(struct proxyfs_task *self);

/** \<\<public\>\> Notifies task that there are data ready (either for writing/reading/both) */
void proxyfs_task_notify_data_ready(struct proxyfs_task *self);

/** \<\<public\>\> Run new task */
struct proxyfs_task *proxyfs_task_run( int (*func)(void*), const char *addr_str );

/** \<\<public\>\> Releases reference and if it was the last reference, frees the task */
void proxyfs_task_put(struct proxyfs_task* task);

/** \<\<public\>\> Get reference and increase reference counter */
struct proxyfs_task* proxyfs_task_get(struct proxyfs_task* task);


#endif  // _PROXYFS_TASK_H_PUBLIC

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef PROXYFS_TASK_PROTECTED

#ifndef _PROXYFS_TASK_H_PROTECTED
#define _PROXYFS_TASK_H_PROTECTED
#include <proxyfs/proxyfs_msg.h>
#include <proxyfs/proxyfs_file.h>
#include <proxyfs/proxyfs_peer.h>

/** \<\<protected\>\> Structure used for polymorphism */
struct proxyfs_task_ops {
	/** Handles task specefic messages */
	void (*handle_msg)(struct proxyfs_task*, struct proxyfs_peer_t*);
	/** Actions to be performed on detection of dead peer */
	void (*handle_peer_dead)(struct proxyfs_task*, struct proxyfs_peer_t*);
	/** Frees dynamicaly allocated resources */ 
	void (*free)(struct proxyfs_task*);
};

/** \<\<protected\>\> Structure used for starting new task */
struct proxyfs_task_start_struct {
	/** Server addr (for listenning or to connect to) */
	const char *addr_str;
	/** Completion */
	struct completion startup_done;
	/** Pointer to created task */
	struct proxyfs_task *task;
};

/** \<\<protected\>\> Proxyfs_task class definition */
struct proxyfs_task {
	/** process descriptor of current task */
	struct task_struct *tsk;

	/** Files list */
	struct list_head files;
	/** Peers list */
	struct list_head peers;

	/** Reference counter */
	atomic_t ref_count;
	/** Operations specific to server or client tasks */
	struct proxyfs_task_ops *ops;

	/** Guards data_ready flag */
	spinlock_t data_ready_lock;
	/** Flag, indicating whether there are some data ready to be read in any of the connected socket */
	int data_ready;
	/** For waiting on data arrival */
	wait_queue_head_t data_ready_wait;
};

/** \<\<protected\>\> This is used by thread func, to tell caller, that we are ready with inicialization */
void proxyfs_task_complete(struct proxyfs_task *self, struct proxyfs_task_start_struct *start_struct);

/** \<\<protected\>\> Find file by ident 
 * @param self - pointer to self instance 
 * @param ident - Unique file identifikator
 *
 * @return pointer to proxyfs_file_t or NULL 
 */
struct proxyfs_file_t *proxyfs_task_find_file(struct proxyfs_task *task, unsigned long ident);
	/*
#define proxyfs_task_find_file(task, ident) ({    \
	struct proxyfs_file_t *file = NULL;       \
	struct list_head *l;                      \
	list_for_each( l, &((task)->files) ){       \
			if( list_entry(l, struct proxyfs_file_t, files)->file_ident == (ident) ){ \
				file = list_entry(l, struct proxyfs_file_t, files); \
				break;			  \
			}                                 \
	}                                         \
	file;})*/

/** \<\<protected\>\> Releases task resources */
void proxyfs_task_free(struct proxyfs_task* self);

/** \<\<protected\>\> Receives data from peer */
void proxyfs_task_recv_peers(struct proxyfs_task *self);

/** \<\<protected\>\> Sends data to peer */
void proxyfs_task_send_peers(struct proxyfs_task *self);

/** \<\<protected\>\> Handle received message */
void proxyfs_task_handle_msg(struct proxyfs_task *self, struct proxyfs_peer_t *peer);

/** \<\<protected\>\> Handle deed peers */
void proxyfs_task_handle_dead_peers(struct proxyfs_task *self);

/** \<\<protected\>\> Try to queue write messages for each file */
void proxyfs_task_send_buffers(struct proxyfs_task *self);

/** \<\<protected\>\> Queue write message */
int proxyfs_task_send_write_buf(struct proxyfs_task *self, struct proxyfs_file_t *proxy_file);

/** \<\<protected\>\> Add socket to peers */
struct proxyfs_peer_t* proxyfs_task_add_to_peers(struct proxyfs_task *self, struct kkc_sock *peer_sock);

/** \<\<protected\>\> Callback function on data ready */
void proxyfs_task_data_ready_callback(void* data, int bytes);
/** \<\<protected\>\> Function to wait for data ready */
void proxyfs_task_wait_for_data_ready(struct proxyfs_task* self);


#endif // _PROXYFS_TASK_H_PROTECTED
#endif // PROXYFS_TASK_PROTECTED

/**
 * @}
 */

