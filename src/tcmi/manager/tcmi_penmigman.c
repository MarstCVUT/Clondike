/**
 * @file tcmi_penmigman.c - Implementation of TCMI cluster core node migration
 *                          manager - a class that controls task migration on PEN
 *                      
 * 
 *
 *
 * Date: 04/20/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_penmigman.c,v 1.9 2009-04-06 21:48:46 stavam2 Exp $
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

#include <tcmi/migration/tcmi_migcom.h>
#include <tcmi/lib/tcmi_slotvec.h>
#include <tcmi/task/tcmi_task.h>
#include <linux/sched.h>

#define TCMI_PENMIGMAN_PRIVATE
#include "tcmi_penmigman.h"

#include <tcmi/comm/tcmi_authenticate_msg.h>
#include <tcmi/comm/tcmi_authenticate_resp_msg.h>
#include <arch/current/regs.h>
#include <tcmi/lib/util.h>

#include <director/director.h>

#include <dbg.h>

/** Forward declaration */
static int tcmi_penmigman_migrate_all_home(void *obj, void *data);

/**
 * \<\<public\>\> TCMI PEN migration manager constructor.
 * The initialization is accomplished exactly in this order:
 * - create new instance
 * - delegates all remaining intialization work to the generic manager.
 *
 * @param *sock - socket where the new PEN is registering
 * @param pen_id - PEN identifier
 * @param *root - directory where the migration manager should create its
 * files and directories
 * @param *migproc - directory where the migrated process will have their info
 * @param namefmt - nameformat string for the main manager directory name (printf style)
 * @param ... - variable arguments
 * @return new TCMI PEN manager instance or NULL
 */
struct tcmi_migman* tcmi_penmigman_new(struct kkc_sock *sock, u_int32_t pen_id, struct tcmi_slot* manager_slot,
				       struct tcmi_ctlfs_entry *root,
				       struct tcmi_ctlfs_entry *migproc,
				       const char namefmt[], ...)
{
	struct tcmi_penmigman *migman;
	va_list args;

	minfo(INFO2, "Creating new TCMI PEN migration manager");
	if (!(migman = TCMI_PENMIGMAN(kmalloc(sizeof(struct tcmi_penmigman), 
					      GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate memory for CCM migration manager");
		goto exit0;
	}
	va_start(args, namefmt);
	if (tcmi_migman_init(TCMI_MIGMAN(migman), sock, 0, pen_id, UNKNOWN, manager_slot, root, migproc,
			     &penmigman_ops, namefmt, args) < 0) {
		mdbg(ERR3, "TCMI PEN migman initializtion failed!");
		va_end(args);
		goto exit1;
	}
	va_end(args);
	
	return TCMI_MIGMAN(migman);

	/* error handling */
 exit1:
	kfree(migman);
 exit0:
	return NULL;
}

/**
 * \<\<public\>\> Authenticates at the CCN, expecting either a response
 *(accept/refused)
 *
 * @param *self - pointer to this migration manager instance
 * @param auth_data_size - optional authentication data string length
 * @param auth_data - optional authentication data string, or NULL
 * @return 0 if successfully, -EINVAL otherwise
 */
int tcmi_penmigman_auth_ccn(struct tcmi_penmigman *self, int auth_data_length, char *auth_data)
{
	/* authentication request and response*/
	struct tcmi_msg *req, *resp;

	/* Build the request */ 
	if ( !(req = tcmi_authenticate_msg_new_tx(tcmi_migman_transactions(TCMI_MIGMAN(self)), TCMI_MIGMAN(self)->pen_id, ARCH_CURRENT, auth_data, auth_data_length)) ) { 
		mdbg(ERR3, "Error creating the authentication request");
		goto exit0;
	}
	if (tcmi_msg_send_and_receive(req, tcmi_migman_sock(TCMI_MIGMAN(self)), 
				      &resp) < 0) {
		minfo(ERR3, "Failed to send the authentication request!!");
		goto exit1;
	}
	if (!resp) {
		minfo(ERR3, "No authentication response from CCN - nothing arrived");
		goto exit1;
	}
	mdbg(INFO3, "Authentication response  is now to be handled");
	tcmi_penmigman_process_msg(TCMI_MIGMAN(self), resp);

	/* fall through is ok, the response message has been discarded
	 * by process_msg */
 exit1:
	/* release the message */
	tcmi_msg_put(req);
 exit0:
	return (tcmi_migman_state(TCMI_MIGMAN(self)) == TCMI_MIGMAN_CONNECTED ? 
		0 : -EINVAL); 
}

/** @addtogroup tcmi_penmigman_class
 *
 * @{
 */



/** 
 * \<\<private\>\> Creates the static files 
 * described \link tcmi_penmigman_class here \endlink. 
 *
 * @param *self - pointer to this migration manager instance
 * @return 0 upon success
 */
static int tcmi_penmigman_init_ctlfs_files(struct tcmi_migman *self)
{
	struct tcmi_penmigman *self_pen = TCMI_PENMIGMAN(self);
	mdbg(INFO4, "Creating TCMI ctlfs files - PEN migman");

	if (!(self_pen->f_mighome_all = 
	      tcmi_ctlfs_intfile_new(self->d_migman, TCMI_PERMS_FILE_W,
				     self, NULL, tcmi_penmigman_migrate_all_home,
				    sizeof(int),"migrate-home-all")))
		goto exit0;

	return 0;

exit0:
	return -EINVAL;
}


/** 
 *\<\<private\>\>  Unregisters and releases all control files.
 *
 * @param *self - pointer to this migration manager instance
 */
static void tcmi_penmigman_stop_ctlfs_files(struct tcmi_migman *self)
{
	struct tcmi_penmigman *self_pen = TCMI_PENMIGMAN(self);
	mdbg(INFO3, "Destroying  TCMI ctlfs files - PEN migman");

	tcmi_ctlfs_file_unregister(self_pen->f_mighome_all);
	tcmi_ctlfs_entry_put(self_pen->f_mighome_all);

}

/** 
 * \<\<private\>\> Emigrates all contained tasks back to home node
 * The method is asynchrounous. Migrate home requests are issued to all contained tasks,
 * but we do not wait till the migration back is performed.
 *
 * @param *obj - pointer to this migration manager instance
 * @param *data - not used
 */
static int tcmi_penmigman_migrate_all_home(void *obj, void *data) {
	struct tcmi_task* task;
	struct tcmi_slot *slot;
	tcmi_slot_node_t* node;
	
	struct tcmi_migman *self  = TCMI_MIGMAN(obj);

	mdbg(INFO3, "Penmigman migrate all home requested");
	/* we protect the iteration so that migration backs do not interfere with the iteration */
	tcmi_slotvec_lock(self->tasks);
	/* Iterate over all managed tasks */
	tcmi_slotvec_for_each_used_slot(slot, self->tasks) {
		tcmi_slot_for_each(node, slot) {
			task = tcmi_slot_entry(node, struct tcmi_task, node);
			mdbg(INFO3, "Sending migrate home request to task: local_pid=%d", tcmi_task_local_pid(task));
			tcmi_migcom_migrate_home_ppm_p(tcmi_task_local_pid(task));
		};
	};
	tcmi_slotvec_unlock(self->tasks);
	mdbg(INFO3, "Penmigman migrate all home done");

	return 0;
}

static inline void tcmi_penmigman_free(struct tcmi_migman *self) {
	/* struct tcmi_penmigman *self_ppn  = TCMI_PENMIGMAN(self); */
	
	/** 
		TODO: We cannot do it so simply as this method won't get called if there are tasks pointing to the migman.. either tasks should not keep reference to migman
		or we better we have to introduce some 'stop' method, that is invoked before the free is performed. This stop will migrate everything back and so we wouldn't need
		to migrate back here, but we'd do that in the stop method
	 */
	//tcmi_penmigman_migrate_all_home(self, NULL);
	/* TODO: Wait till all migrations are finished? */
}

/** 
 * \<\<private\>\> Called on stop request
 * 
 * Emigrates all contained tasks back to home node (asynchronously - only emigration requests are issued in context of this method)
 *
 * @param *self - pointer to this migration manager instance
 */
static void tcmi_penmigman_stop(struct tcmi_migman *self, int remote_requested) {
    tcmi_penmigman_migrate_all_home(self, NULL);
    
    director_node_disconnected(tcmi_migman_slot_index(self), 0, remote_requested);
}

/** \<\<public\>\> Send dignal to process
 * @param *self - pointer to this penmigman instance 
 * @param pid - signal target pid
 * @param *info - info for signal to be send
 */
static void tcmi_penmigman_send_signal(struct tcmi_migman *self, int pid, siginfo_t *info)
{
	int sig;
	struct task_struct *task;

	rcu_read_lock();
	task = task_find_by_pid(pid);
	rcu_read_unlock();
	sig = info->si_signo;
	mdbg(INFO2, "Sending signal %d to task %p (PID %d)", sig, task, pid);
	if(task)
		send_sig_info(sig, info, task);
}


/**
 * \<\<private\>\> Processes a TCMI message m. 
 * The message is disposed afterwords.
 *
 * @param *self - pointer to this migration manager instance
 * @param *m - pointer to a message to be processed
 */
static void tcmi_penmigman_process_msg(struct tcmi_migman *self, struct tcmi_msg *m)
{
	struct tcmi_authenticate_resp_msg *auth_msg;
	struct tcmi_generic_user_msg *user_msg;
	struct tcmi_msg *resp;
	struct tcmi_penmigman *self_pen  = TCMI_PENMIGMAN(self);
	int err;

	mdbg(INFO4, "Processing message (%p)", m);
	
	if ( !m )
	    return;

	switch(tcmi_msg_id(m)) {
		case TCMI_AUTHENTICATE_RESP_MSG_ID:
			auth_msg = TCMI_AUTHENTICATE_RESP_MSG(m);
	
			if ( auth_msg->result_code == 0) {
				minfo(INFO1, "Authentication at CCN successful..");
				self->peer_arch_type = tcmi_authenticate_resp_msg_arch(auth_msg);
				self->ccn_id = tcmi_authenticate_resp_msg_ccn_id(auth_msg);
				self_pen->mount_params = *tcmi_authenticate_resp_msg_mount_params(auth_msg);
				minfo(INFO1, "received mount_params - type: %s device: %s options: %s", 
						self_pen->mount_params.mount_type, self_pen->mount_params.mount_device,
						self_pen->mount_params.mount_options);
				tcmi_migman_set_state(self, TCMI_MIGMAN_CONNECTED);
			} else {
				minfo(INFO1, "Authentication at CCN failed with error=%d..", auth_msg->result_code);			
			}
			break;
		case TCMI_MSG_FLG_SET_ERR(TCMI_AUTHENTICATE_RESP_MSG_ID):
			minfo(INFO1, "Authentication at CCN failed, error received..");
			break;
		case TCMI_SIGNAL_MSG_ID:
			tcmi_penmigman_send_signal(self, TCMI_SIGNAL_MSG(m)->pid,
					&TCMI_SIGNAL_MSG(m)->info);
			break;
		case TCMI_P_EMIGRATE_MSG_ID:
			minfo(INFO1, "Physical emigrate message has arrived..");
			if (tcmi_migcom_immigrate(m, self) < 0) {
				minfo(ERR3, "Error immigrating process");
												
				if (!(resp = tcmi_err_procmsg_new_tx(TCMI_GUEST_STARTED_PROCMSG_ID, tcmi_msg_req_id(m), -EINVAL, tcmi_p_emigrate_msg_reply_pid(TCMI_P_EMIGRATE_MSG(m))))) {
				      minfo(ERR3, "Failed to send error response");
				} else {
				      tcmi_msg_send_anonymous(resp, tcmi_migman_sock(self));				      
				}
			}
			break;
		case TCMI_GENERIC_USER_MSG_ID:
			user_msg = TCMI_GENERIC_USER_MSG(m);
			minfo(INFO1, "User message arrived in pen.");
	
			err = director_generic_user_message_recv(tcmi_generic_user_msg_node_id(user_msg), 0, tcmi_migman_slot_index(self), tcmi_generic_user_msg_user_data_size(user_msg), tcmi_generic_user_msg_user_data(user_msg));
			if ( err ) {
				mdbg(ERR3, "Error in processing user message %d", err);
			}
			break;
		default:
			minfo(INFO1, "Unexpected message ID %x, no handler available.", 
			tcmi_msg_id(m));
			break;
	}

	/* release the message */
	tcmi_msg_put(m);

}


/** Migration manager operations that support polymorphism */
static struct tcmi_migman_ops penmigman_ops = {
	.init_ctlfs_files = tcmi_penmigman_init_ctlfs_files, 
	.stop_ctlfs_files = tcmi_penmigman_stop_ctlfs_files,
//	.free = tcmi_penmigman_free,
	.stop = tcmi_penmigman_stop,
	.process_msg = tcmi_penmigman_process_msg,
};

/**
 * @}
 */


