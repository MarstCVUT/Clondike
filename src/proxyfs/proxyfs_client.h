/**
 * @file proxyfs_client.h - Proxyfs client task.
 *                      
 * 
 *
 *
 * Date: 08/12/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_client.h,v 1.7 2008/05/02 19:59:20 stavam2 Exp $
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
#ifndef _PROXYFS_CLIENT_H_PUBLIC
#define _PROXYFS_CLIENT_H_PUBLIC

#include "proxyfs_task.h"
#include "proxyfs_proxy_file.h"
#include "proxyfs_msg.h"

/** @defgroup proxyfs_client_class proxyfs_client class 
 * 
 * @ingroup proxyfs_task_class
 *
 * \<\<class\>\> representing proxyfs_client task
 * 
 * @{
 *
 */

/** \<\<public\>\> Client task declaration */
struct proxyfs_client_task; 

/** \<\<public\>\> Cast to struct proxyfs_client_task* */
#define PROXYFS_CLIENT_TASK(arg) ((struct proxyfs_client_task*)(arg))

/** \<\<public\>\> Startup function for proxyfs client */
int proxyfs_client_thread(void *start_struct);

/** \<\<public\>\> Create new proxy file */
struct proxyfs_proxy_file_t *proxyfs_client_open_proxy(struct proxyfs_client_task *self, unsigned long inode_num);

/** \<\<public\>\> Do syscall */
struct proxyfs_msg *proxyfs_client_do_syscall(struct proxyfs_client_task *self, 
		struct proxyfs_proxy_file_t *file, struct proxyfs_msg *msg);

/** \<\<public\>\> Asynchronously sends a message */
void proxyfs_client_send_message(struct proxyfs_client_task *self, 
		struct proxyfs_proxy_file_t *file, struct proxyfs_msg *msg);

#endif // _PROXYFS_CLIENT_H_PUBLIC

/********************** PROTECTED METHODS AND DATA ******************************/
#ifdef PROXYFS_CLIENT_PROTECTED
#ifndef _PROXYFS_CLIENT_H_PROTECTED
#define _PROXYFS_CLIENT_H_PROTECTED

#define PROXYFS_TASK_PROTECTED // Parent
#include "proxyfs_task.h"

#include <linux/list.h>
#include <linux/semaphore.h>

#include <proxyfs/proxyfs_peer.h>

struct proxyfs_client_task {
	struct proxyfs_task parent;

	struct list_head try_open;
	struct list_head try_open_send;
	/** Guards try_open list.. */
	struct semaphore try_open_sem;
	/** This semaphore prevents race conditions between client task and checking for task presence in the client */
	struct semaphore files_sem;
	/** 
	 * TODO: This reference is actually only an alias to a single element of list "peers" of task structure. Having this alias is causing race like problems
	 * and is not really well readable, so would be better to get rid of it
	 */
	struct proxyfs_peer_t *server;
};
#endif // _PROXYFS_CLIENT_H_PROTECTED
#endif // PROXYFS_CLIENT_PROTECTED



/********************** PROTECTED METHODS AND DATA ******************************/

#ifdef PROXYFS_CLIENT_PRIVATE
#ifndef _PROXYFS_CLIENT_H_PRIVATE
#define _PROXYFS_CLIENT_H_PRIVATE

/** \<\<private\>\> Proxyfs client constructor */
struct proxyfs_client_task* proxyfs_client_new(void);

/** \<\<private\>\> Main client loop */
void proxyfs_client_loop(struct proxyfs_client_task *self);

/** \<\<private\>\> Send open message for queued files */
void proxyfs_client_open_files(struct proxyfs_client_task *self);

/** \<\<private\>\> Handle task specific received message */
static void proxyfs_client_handle_msg(struct proxyfs_task *self, struct proxyfs_peer_t *peer);

/** \<\<private\>\> Free task resources */
static void proxyfs_client_free(struct proxyfs_task* self);

/** \<\<private\>\> Handling of dead peer */
static void proxyfs_client_handle_dead_peer(struct proxyfs_task* self, struct proxyfs_peer_t* peer);

/** <\<\private\>\> Structure with pointers to virtual methods */
struct proxyfs_task_ops client_ops = {
	/** Handle received message */
	.handle_msg = proxyfs_client_handle_msg,
	.handle_peer_dead = proxyfs_client_handle_dead_peer,
	/** Free task resources */
	.free = proxyfs_client_free,
};

#endif // _PROXYFS_CLIENT_H_PRIVATE
#endif // PROXYFS_CLIENT_PRIVATE

/**
 * @}
 */

