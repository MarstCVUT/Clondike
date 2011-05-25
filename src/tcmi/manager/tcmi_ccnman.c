/**
 * @file tcmi_ccnman.c - Implementation of TCMI cluster core node
 *                       manager - a class that controls the cluster 
 *                       core node
 *                      
 * 
 *
 *
 * Date: 04/06/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ccnman.c,v 1.9 2009-04-06 21:48:46 stavam2 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2005  Jan Capek
 * 
 * TCMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * TCMI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include <linux/kthread.h>

#include <tcmi/migration/tcmi_migcom.h>
#include <tcmi/ctlfs/tcmi_ctlfs.h>
#include <tcmi/lib/tcmi_sock.h>
#include "tcmi_ccnmigman.h"

#include <kkc/kkc.h>

#define TCMI_CCNMAN_PRIVATE
#include "tcmi_ccnman.h"
//#include <tcmi/migration/fs/fs_mount_params.h>

#include <dbg.h>

#include <director/director.h>

/** maximum number of listenings */
#define MAX_LISTENINGS 2

/**
 * \<\<public\>\> Singleton instance initializer.
 * The initialization is accomplished exactly in this order:
 * - allocate new slot vector for listenings 
 * - start listening thread
 * - delegate rest of initialization to TCMI generic manager
 *
 * @param *root - root directory of the filesystem, where all entries
 * are to be created
 * @return 0 upon success
 */
int tcmi_ccnman_init(struct tcmi_ctlfs_entry *root)
{
	self.mount_params.mount_type[0] = '\0';
	self.mount_params.mount_options[0] = '\0';
	self.mount_params.mount_device[0] = '\0';
	minfo(INFO1, "Initializing TCMI CCN manager");
	if (!(self.listenings = tcmi_slotvec_new(MAX_LISTENINGS))) {
		mdbg(ERR3, "Failed to create listenings slot vector");
		goto exit0;
	}
	if (tcmi_ccnman_start_thread() < 0) {
		mdbg(ERR3, "Failed to start listening thread");
		goto exit1; 
	}
	if (tcmi_man_init(TCMI_MAN(&self), root, &ccnman_ops, "ccn") < 0) {
		mdbg(ERR3, "TCMI CCN manager - generic init failed");
		goto exit2;
	}
	return 0;
		
	/* error handling */

 exit2:
	tcmi_ccnman_stop_thread();
 exit1:
	tcmi_slotvec_put(self.listenings);
 exit0:
	return -EINVAL;
}

/** 
 * \<\<public\>\> The manager is shutdown exactly in this order:
 *
 * - stop the listening thread, the thread doesn't need to worry about
 * termination of any listenings-it will be done further
 * - perform a generic shutdown (this destroys all
 * files and stops migration etc.). This step preceeds stop listening
 * as we need to be sure, that no control files are active (user
 * might try adding a new listening interface)
 * - stop listening on all interfaces
 * - destroy the listenings slotvector (should be empty by now)
 */
void tcmi_ccnman_shutdown(void)
{
	int stop_listen = 1;
	minfo(INFO1, "Shutting down the TCMI CCN manager");

	tcmi_ccnman_stop_thread();
	tcmi_man_shutdown(TCMI_MAN(&self));
	tcmi_ccnman_stop_listen_all(NULL, &stop_listen);
	tcmi_slotvec_put(self.listenings);
}


/** @addtogroup tcmi_ccnman_class
 *
 * @{
 */


/** 
 * \<\<private\>\> Creates the additional directories as described in \link
 * tcmi_ccnman_class here \endlink. This method is called back
 * by the \link tcmi_man_class TCMI manager generic class\endlink.
 *
 * Currently, there is only one additional directory that stores listenings.
 * @return 0 upon success
 */
static int tcmi_ccnman_init_ctlfs_dirs(void)
{

	if (!(self.d_listening_on = 
	      tcmi_ctlfs_dir_new(tcmi_man_root(TCMI_MAN(&self)), TCMI_PERMS_DIR, "listening-on")))
		goto exit0;

	if (!(self.d_mounter = 
	      tcmi_ctlfs_dir_new(tcmi_man_root(TCMI_MAN(&self)), TCMI_PERMS_DIR, "mounter")))
		goto exit1;

	return 0;

	/* error handling */
 exit1:
	tcmi_ctlfs_entry_put(self.d_listening_on);
 exit0:
	return -EINVAL;

}
/** 
 * \<\<private\>\> Creates the static files 
 * described \link tcmi_ccnman_class here \endlink. 
 *
 * @return 0 upon success
 */
static int tcmi_ccnman_init_ctlfs_files(void)
{
	mdbg(INFO4, "Creating files");
	if (!(self.f_stop_listen_all = 
	      tcmi_ctlfs_intfile_new(tcmi_man_root(TCMI_MAN(&self)), TCMI_PERMS_FILE_W,
				     NULL, NULL, tcmi_ccnman_stop_listen_all,
				     sizeof(int), "stop-listen-all")))
		goto exit0;

	if (!(self.f_stop_listen_one = 
	      tcmi_ctlfs_intfile_new(tcmi_man_root(TCMI_MAN(&self)), TCMI_PERMS_FILE_W,
				     NULL, NULL, tcmi_ccnman_stop_listen_one,
				     sizeof(int), "stop-listen-one")))
		goto exit1;

	if (!(self.f_listen = 
	      tcmi_ctlfs_strfile_new(tcmi_man_root(TCMI_MAN(&self)), TCMI_PERMS_FILE_W,
				     NULL, NULL, tcmi_ccnman_listen,
				     KKC_MAX_WHERE_LENGTH, "listen")))
		goto exit1;

	if (!(self.f_mount = 
	      tcmi_ctlfs_strfile_new(self.d_mounter, TCMI_PERMS_FILE_W,
				     NULL, NULL, tcmi_man_set_mount,
				     TCMI_FS_MOUNT_LENGTH, "fs-mount")))
		goto exit1;

	if (!(self.f_mount_device = 
	      tcmi_ctlfs_strfile_new(self.d_mounter, TCMI_PERMS_FILE_W,
				     NULL, NULL, tcmi_man_set_mount_device,
				     TCMI_FS_MOUNT_LENGTH, "fs-mount-device")))
		goto exit1;

	if (!(self.f_mount_options = 
	      tcmi_ctlfs_strfile_new(self.d_mounter, TCMI_PERMS_FILE_W,
				     NULL, NULL, tcmi_man_set_mount_options,
				     TCMI_FS_MOUNT_OPTIONS_LENGTH, "fs-mount-options")))
		goto exit1;

	return 0;

	/* error handling */
 exit1:
	tcmi_ccnman_stop_ctlfs_files();
 exit0:
	return -EINVAL;

}

/**
 * Adds a sleeper that represents a wait queue element for a newly
 * created socket into the internal sleepers list.  The sleeper needs
 * to be removed from the list prior to releasing the socket
 * reference.
 *
 * @param *sleeper - element to be added to the sleepers list
 */
static inline void tcmi_ccnman_add_sleeper(struct kkc_sock_sleeper *sleeper)
{
	spin_lock(&self.sleepers_lock);
	list_add(&sleeper->list, &self.sleepers);
	spin_unlock(&self.sleepers_lock);
}

/**
 * Searches the list of socket sleepers and removes all sleepers that
 * match the socket. 
 *
 * @param *sock - socket that is to be cleaned from all sleepers
 */
static inline void tcmi_ccnman_remove_sleepers(struct tcmi_sock *sock)
{
	struct kkc_sock *k_sock = tcmi_sock_kkc_sock_get(sock);
	/* backup element for traversing the list is needed since the
	 * sleeper might get removed */
	struct kkc_sock_sleeper *sleeper, *tmp;
	spin_lock(&self.sleepers_lock);
	/* Find all sleepers that match this socket */
	list_for_each_entry_safe(sleeper, tmp, &self.sleepers, list) {
		kkc_sock_sleeper_remove_match(sleeper, k_sock);
	}
	spin_unlock(&self.sleepers_lock);
}

/** 
 * \<\<private\>\> Releases all directories - called back by generic
 * TCMI manager
 *
 */
static void tcmi_ccnman_stop_ctlfs_dirs(void)
{
	mdbg(INFO3, "Destroying  TCMI CCN manager ctlfs directories");
	tcmi_ctlfs_entry_put(self.d_listening_on);
	tcmi_ctlfs_entry_put(self.d_mounter);
}

/** 
 *\<\<private\>\>  Unregisters and releases all control files.
 */
static void tcmi_ccnman_stop_ctlfs_files(void)
{
	mdbg(INFO3, "Destroying  TCMI CCN manager ctlfs files");

	tcmi_ctlfs_file_unregister(self.f_listen);
	tcmi_ctlfs_entry_put(self.f_listen);

	tcmi_ctlfs_file_unregister(self.f_stop_listen_one);
	tcmi_ctlfs_entry_put(self.f_stop_listen_one);

	tcmi_ctlfs_file_unregister(self.f_stop_listen_all);
	tcmi_ctlfs_entry_put(self.f_stop_listen_all);

	tcmi_ctlfs_file_unregister(self.f_mount);
	tcmi_ctlfs_entry_put(self.f_mount);	

	tcmi_ctlfs_file_unregister(self.f_mount_device);
	tcmi_ctlfs_entry_put(self.f_mount_device);	

	tcmi_ctlfs_file_unregister(self.f_mount_options);
	tcmi_ctlfs_entry_put(self.f_mount_options);	

}

/**
 * \<\<private\>\> Instantiates a new listening.
 * Following steps need to be done
 * - reserve an empty slot in the slot vector
 * - create a new KKC socket, that listens on specified interface
 * - create a new KKC socket sleeper associated with the listening
 * thread and add it to the sleepers list.
 * - create a new TCMI socket that will present the KKC socket in
 * TCMI ctlfs
 * - insert the TCMI socket into that reserved slot
 * - release the KKC socket - it is not needed anymore as the TCMI
 * socket retains its own reference.
 *
 * @param *obj - pointer to an object - NULL is expected as
 * TCMI CCN manager is a singleton class.
 * @param *str - listening string - encodes the interface on which the
 * new listening is to be started. It is architecture dependent
 * and passed further on KKC to parse it.
 * @return 0 upon success
 */
static int tcmi_ccnman_listen(void *obj, void *str)
{
	struct tcmi_slot *slot;
	struct kkc_sock *k_sock;
	struct tcmi_sock *t_sock;
	/* storage for the new socket sleeper element */
	struct kkc_sock_sleeper *sleeper;
	char *listen_str = (char *) str;
	int err = -EINVAL;

	mdbg(INFO3, "Listen request on %s", listen_str);
	/* Reserve an empty slot */
	if (!(slot = tcmi_slotvec_reserve_empty(self.listenings))) {
		mdbg(ERR3, "Failed allocating an empty slot for listening '%s'", 
		     listen_str);
		goto exit0;
	}
	/* create KKC socket */
	if ((err = kkc_listen(&k_sock, listen_str))) {
		mdbg(ERR3, "Failed creating KKC listening on '%s'", listen_str);
		goto exit1;
	}
	if (!(sleeper = kkc_sock_sleeper_add(k_sock, self.thread))) {
		mdbg(ERR3, "Failed to create a new socket sleeper");
		goto exit2;
	}

	if (!(t_sock = tcmi_sock_new(self.d_listening_on, k_sock, "listen-%02d", 
				     tcmi_slot_index(slot)))) {
		mdbg(ERR3, "Failed creating TCMI socket on '%s'", listen_str);
		goto exit3;
	}
	/* append the sleeper to the list */
	tcmi_ccnman_add_sleeper(sleeper);
	tcmi_slot_insert(slot, tcmi_sock_node(t_sock));
	/* KKC socket not needed anymore as it is stored internally by
	 * TCMI socket */
	kkc_sock_put(k_sock);
	
	return 0;
	/* error handling */
 exit3:
	kkc_sock_sleeper_remove(sleeper);
 exit2:
	kkc_sock_put(k_sock);
 exit1:
	tcmi_slot_move_unused(slot); 
 exit0:
	return err;
}



/**
 * \<\<private\>\> Iterates through all active listenings and notifies
 * each instance to quit. The slot vector needs to be locked while
 * iterating through the listenings.
 *
 * @param *obj - pointer to an object - NULL is expected as
 * TCMI CCN manager is a singleton class.
 * @param *data - if the value that it points to is >0, the action is 
 * taken
 * @return 0 - when all listenings have been terminated
 */
static int tcmi_ccnman_stop_listen_all(void *obj, void *data)
{
	int stop = *((int *)data);
	/* temporary storage for the slots while traversing the vector */
	struct tcmi_sock *listening;
	mdbg(INFO3, "Stop listening - all interfaces, action = %d", stop);
	

	if (stop) {
		while (!tcmi_slotvec_empty(self.listenings)) {

			tcmi_slotvec_lock(self.listenings);
			tcmi_slotvec_remove_one_entry(self.listenings, listening, node);
			tcmi_slotvec_unlock(self.listenings);

			if (listening) {
				tcmi_ccnman_remove_sleepers(listening);
				tcmi_sock_put(listening);
			}
		}

	}
	return 0;
}

/**
 * \<\<private\>\> Destroys a particular listening specified by the
 * user.  The listening is first looked up in the slotvector and
 * removed. After unlocking the slotvector we can destroy it.
 *
 * @param *obj - pointer to an object - NULL is expected as
 * TCMI CCN manager is a singleton class.
 * @param *data - slot number where the listening that is to 
 * destroyed resides.
 * @return 0 - when all listenings have been terminated
 */
static int tcmi_ccnman_stop_listen_one(void *obj, void *data)
{
	int slot_num = *((int *)data);
	/* temporary storage for the slot where the listening resides */
	struct tcmi_slot *slot;
	/* temporary storage for the slots while traversing the vector */
	struct tcmi_sock *listening = NULL;

	

	tcmi_slotvec_lock(self.listenings);
	slot = tcmi_slotvec_at(self.listenings, slot_num);
	if (!slot) {
		mdbg(ERR3, "Can't stop listening - invalid number! %d", slot_num);
		goto exit0;
	}
	/* there is always only one listening per slot */
	tcmi_slot_remove_first(listening, slot, node);
	/* Slot might exist, but it might still be empty*/
	if (!listening) {
		mdbg(ERR3, "Can't stop listening - no such listening (%d)", slot_num);
		goto exit0;
	}
	tcmi_slotvec_unlock(self.listenings);
	tcmi_ccnman_remove_sleepers(listening);
	tcmi_sock_put(listening);

	mdbg(INFO3, "Stopped listening - %d", slot_num);	
	return 0;
	/* error handling */
 exit0:
	tcmi_slotvec_unlock(self.listenings);
	return -EINVAL;
}

/** 
 * \<\<private\>\> Write method to set fs mount type
 *
 * @param *obj - this manager instance
 * @param *data - string with name of the mount type
 * @return 0 upon success
 */
static int tcmi_man_set_mount(void* obj, void* data)
{
	char *type_str = (char *)data;
	strncpy(self.mount_params.mount_type, type_str, TCMI_FS_MOUNT_LENGTH);
	return 0;
}

/** 
 * \<\<private\>\> Write method to set fs mount device
 *
 * @param *obj - this manager instance
 * @param *data - string with the mount device name
 * @return 0 upon success
 */
static int tcmi_man_set_mount_device(void* obj, void* data)
{
	char *device_str = (char *)data;
	strncpy(self.mount_params.mount_device, device_str, TCMI_FS_MOUNT_DEVICE_LENGTH);
	return 0;
}

/** 
 * \<\<private\>\> Write method to set fs mount options
 *
 * @param *obj - this manager instance
 * @param *data - string with the mount options
 * @return 0 upon success
 */
static int tcmi_man_set_mount_options(void* obj, void* data)
{
	char *options_str = (char *)data;
	strncpy(self.mount_params.mount_options, options_str, TCMI_FS_MOUNT_OPTIONS_LENGTH);
	return 0;
}

/**
 * \<\<private\>\> This method is called back by the generic TCMI manager.
 * It stops listening on all interfaces.
 *
 */
static void tcmi_ccnman_stop(void)
{
	int stop_listen = 1;
	tcmi_ccnman_stop_listen_all(NULL, &stop_listen);
}

/**
 * \<\<private\>\> Starts the listening thread. The kernel thread is
 * started by using kernel services. No additional actions are needed.
 *
 * @return 0 upon sucessful thread creation 
 */
static int tcmi_ccnman_start_thread(void)
{
	mdbg(INFO3, "Starting listening thread");

	self.thread = kthread_run(tcmi_ccnman_thread, NULL, "tcmi_ccnmand");
	
	if (IS_ERR(self.thread)) {
		mdbg(ERR2, "Failed to create a listening thread!");
		goto exit0;
	}
	mdbg(INFO3, "Created listening thread %p", self.thread);
	return 0;

	/* error handling */
 exit0:
	return -EINVAL;
}

/**
 * \<\<private\>\> The TCMI CCN manager listening thread is
 * terminated. The thread state is changed to 'DONE', so that the
 * thread notices, that the manager is about to be shutdown and
 * terminates.
 */
static void tcmi_ccnman_stop_thread(void)
{
	if (self.thread) {
		mdbg(INFO3, "Stopping listening thread");
		kthread_stop(self.thread);
	}
}

/**
 * \<\<private\>\> This is the actual listening thread.  It performs
 * an infinite loop which cycles through all listenings checking for
 * an incoming connection. This task is not trivial as we are racing
 * with other threads from user space, that add and remove
 * listenings. Also it is highly desirable to process a listening and
 * not hold the slot vector lock.
 * 
 * is equal to following pseudocode:
 \verbatim
 while(!signalled to quit) {
   for each slot (s) {
     take first listening (l) from (s)
     if (s) is empty -> report serious error, break;
     obtain the KKC socket and process it.
     if (l) has been removed from (s) in the
       meantime (unhashed) -> break;
 }
 \endverbatim
 *
 * @param *data - a mandatory parameter, not used by our thread as it
 * can access all tcmi_ccnman related data from the singleton instance
 * @return 0 upon successful thread termination.
 */
static int tcmi_ccnman_thread(void *data)
{
	struct tcmi_sock *listening = NULL;
	struct tcmi_sock *tmp = NULL;
	struct tcmi_slot *slot;
	mdbg(INFO3, "Listening thread up and running %p", current);

	while (!kthread_should_stop()) {
		/* iterate safely through the slot vector */
		tcmi_slotvec_lock(self.listenings);
		tcmi_slotvec_for_each_used_slot(slot, self.listenings) {
			/* release the socket from previous iteration */
			tcmi_sock_put(tmp);
			/* retrieve the first object in the slot */
			tcmi_slot_find_first(listening, slot, node);
			if (!tcmi_sock_get(listening)) {
				mdbg(ERR3, "Slot vector inconsistency, empty slot in used list!!");
				break;
			}
			tcmi_slotvec_unlock(self.listenings);
			/* now the critical region is over, and we can
			 * process the listening without any need to
			 * lock the slot vector since we own */
			tcmi_ccnman_process_sock(tcmi_sock_kkc_sock_get(listening));
			tmp = listening; /* tmp is released in the next iteration */
			/* set critical region for next iteration */
			tcmi_slotvec_lock(self.listenings);
			if (tcmi_slot_node_unhashed(tcmi_sock_node(tmp))) {
				mdbg(INFO3, "Socket has been unhashed in the meantime local: '%s', remote: '%s'", 
				     kkc_sock_getsockname2(tcmi_sock_kkc_sock_get(tmp)), 
				     kkc_sock_getpeername2(tcmi_sock_kkc_sock_get(tmp)));
				break;
			}
		}
		tcmi_slotvec_unlock(self.listenings);
		/* release the socket from the last iteration and
		 * reset tmp to prevent another put in next main loop
		 * iteration */
		tcmi_sock_put(tmp);
		tmp = NULL;

		/* for each listening if incoming create mig man */
		mdbg(INFO3, "Listening thread going to sleep");
		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
		mdbg(INFO3, "Listening thread woken up");
	}
	mdbg(INFO3, "Listening thread terminating");
	return 0;
}


/** 
 * \<\<private\>\> Processes a selected socket and checks for incoming
 * connection. If an incoming connection is detected, spawns a new
 * migration manager passing it the connected socket. A successfully
 * created migration manager is added among other migration managers
 * into the slot vector.
 *
 * @param *sock - kkc socket that is to be processed
 */
static void tcmi_ccnman_process_sock(struct kkc_sock *sock)
{
	int err;
	struct kkc_sock *new_sock;
	struct tcmi_migman *migman;
	struct tcmi_slot *slot;	

	/* check if there is incoming connection */
	if ((err = kkc_sock_accept(sock, &new_sock, 
				   KKC_SOCK_NONBLOCK)) < 0) {
		mdbg(INFO3, "No connection, accept would block %d", err);
		goto exit0;
	}
	mdbg(INFO3, "Accepted incoming connection local: '%s', remote: '%s'", 
	     kkc_sock_getsockname2(new_sock), kkc_sock_getpeername2(new_sock));
	/* Reserve an empty slot */
	if (!(slot = tcmi_man_reserve_migmanslot(TCMI_MAN(&self)))) {
		mdbg(ERR3, "Failed allocating an empty slot for CCN migman"); 
		goto exit1;
	}

	/* Accept the first incoming connection */
	if (!(migman = tcmi_ccnmigman_new(new_sock, 
					  tcmi_man_id(TCMI_MAN(&self)), 
					  slot,
					  tcmi_man_nodes_dir(TCMI_MAN(&self)),
					  tcmi_man_migproc_dir(TCMI_MAN(&self)),
					  "%d", tcmi_slot_index(slot)))) {
		minfo(ERR1, "Failed to instantiate the CCN migration manager"); 
		goto exit2;
	}
	if ((err = tcmi_ccnmigman_auth_pen(TCMI_CCNMIGMAN(migman))) < 0) {
		minfo(ERR1, "Failed to authenticate the PEN! %d", err); 
		goto exit3;
	}

	tcmi_slot_insert(slot, tcmi_migman_node(migman));

	kkc_sock_put(new_sock);

	return;
/* error handling */
 exit3:
	tcmi_migman_put(migman);
 exit2:
	tcmi_man_release_migmanslot(TCMI_MAN(&self), slot); 
 exit1:
	kkc_sock_put(new_sock);
 exit0:
	return;
}

/** \<\<public\>\> Getter of mount params structure. */
struct fs_mount_params* tcmi_ccnman_get_mount_params(void) {
	return &self.mount_params;
}

/** Singleton instance pre-initialization */
static struct tcmi_ccnman self = {
	.listenings = NULL,
	.f_listen = NULL,
	.d_listening_on = NULL,	
	.f_stop_listen_all = NULL,
	.d_mounter = NULL,
	.f_mount = NULL,
	.f_mount_device = NULL,
	.f_mount_options = NULL,
	.thread = NULL,
	.sleepers = LIST_HEAD_INIT(self.sleepers),
	.sleepers_lock = SPIN_LOCK_UNLOCKED,
};


/** Migration manager operations that support polymorphism */
static struct tcmi_man_ops ccnman_ops = {
	.init_ctlfs_dirs = tcmi_ccnman_init_ctlfs_dirs,
	.stop_ctlfs_dirs = tcmi_ccnman_stop_ctlfs_dirs,
	.init_ctlfs_files = tcmi_ccnman_init_ctlfs_files, 
	.stop_ctlfs_files = tcmi_ccnman_stop_ctlfs_files,
	.stop = tcmi_ccnman_stop,
	.emigrate_ppm_p = tcmi_migcom_emigrate_ccn_ppm_p,
	.emigrate_npm = tcmi_migcom_emigrate_ccn_npm,
	.migrate_home_ppm_p = tcmi_migcom_migrate_home_ppm_p,
	.fork = tcmi_migcom_shadow_fork,
};

/**
 * @}
 */


