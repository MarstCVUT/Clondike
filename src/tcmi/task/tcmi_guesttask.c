/**
 * @file tcmi_guesttask.c - TCMI guesttask, migrated process abstraction on PEN
 *                      
 * 
 *
 *
 * Date: 04/26/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_guesttask.c,v 1.13 2009-04-06 21:48:46 stavam2 Exp $
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

#include <tcmi/comm/tcmi_messages_dsc.h>
#include <tcmi/migration/tcmi_migcom.h>
#include <tcmi/manager/tcmi_migman.h>

#include "tcmi_taskhelper.h"

#define TCMI_GUESTTASK_PRIVATE
#include "tcmi_guesttask.h"

#include <proxyfs/proxyfs_helper.h>

#include <director/director.h>

/** 
 * \<\<public\>\> Instance constructor.
 * - allocates a new instance.
 * - delegates its initialization to the super class.
 *
 * @param local_pid - local PID on PEN
 * @param migman - The migration manager that maintains this task
 * @param *sock - socket used for communication with shadow task on CCN
 * @param *d_migproc - directory where ctlfs entries of migrated
 * processes reside
 * @param *d_migman - TCMI ctlfs directory of the migration manager
 * responsible for this task
 * @return 0 upon success
 */
struct tcmi_task* tcmi_guesttask_new(pid_t local_pid, struct tcmi_migman* migman,
				    struct kkc_sock *sock, 
				    struct tcmi_ctlfs_entry *d_migproc, 
				    struct tcmi_ctlfs_entry *d_migman)
{
	struct tcmi_guesttask *task;
	minfo(INFO2, "Creating new TCMI guest task");
	if (!(task = TCMI_GUESTTASK(kmalloc(sizeof(struct tcmi_guesttask), 
						   GFP_ATOMIC)))) {
		mdbg(ERR3, "Can't allocate memory for TCMI ppm guest task");
		goto exit0;
	}
	if (tcmi_task_init(TCMI_TASK(task), local_pid, migman, sock, 
			   d_migproc, d_migman, &guesttask_ops) < 0) {
		mdbg(ERR3, "TCMI ppm guest task initialization failed!");
		goto exit1;
	}

	return TCMI_TASK(task);

	/* error handling */
 exit1:
	kfree(task);
 exit0:
	return NULL;
}

/** @addtogroup tcmi_guesttask_class
 *
 * @{
 */

/** 
 * \<\<private\>\> Processes a message specified by the caller.
 * It handles:
 * - physical emigration request - this is special - as the guest
 * should turn eventually into a migrated process.
 *
 *
 * @param *self - pointer to this task instance
 * @param *m - message to be processed
 * @return depends on the type of message that has been processed
 */
static int tcmi_guesttask_process_msg(struct tcmi_task *self, struct tcmi_msg *m)
{
	int res = TCMI_TASK_KEEP_PUMPING;
	/* struct tcmi_guesttask *self_tsk = TCMI_GUESTTASK(self); */

memory_sanity_check("On process message");

	switch (tcmi_msg_id(m)) {
		/* task PPM phys. migration request */
	case TCMI_P_EMIGRATE_MSG_ID:
		res = tcmi_guesttask_process_p_emigrate_msg(self, m);
		break;
	case TCMI_PPM_P_MIGR_BACK_SHADOWREQ_PROCMSG_ID:
		/* dispose the message not needed anymore. */
		tcmi_msg_put(m);
		/* Is there some better way to invoke self migration home? If we call directly migrate back of this task the method queue won't be flushed as in the case of this call. */
		tcmi_migcom_migrate_home_ppm_p(tcmi_task_local_pid(self));
		break;
	default:
		mdbg(ERR3, "Unexpected message from the guest task: %x", tcmi_msg_id(m));
		break;
	}

memory_sanity_check("After process message");

	return res;
}

/** 
 * \<\<private\>\> Emigrates a task to a PEN.
 *
 * @param *self - pointer to this task instance
 * @return result of the task specific emigrate method or 
 * TCMI_TASK_KEEP_PUMPING
 */
static int tcmi_guesttask_emigrate_p(struct tcmi_task *self)
{
	int res = TCMI_TASK_KEEP_PUMPING;
/*	struct tcmi_guesttask *self_tsk = TCMI_GUESTTASK(self);*/

	return res;
}

/**
 * Internal helper method for performing both npm/ppm physical migration back to ccn.
 *
 * @return 0 on success, error code otherwise
 */
static int tcmi_guesttask_migrateback_p(struct tcmi_task* self, struct tcmi_npm_params* npm_params) {
	struct tcmi_msg *req;

	mdbg(INFO2, "Process '%s' - guest local PID %d, migrating back (npm params: %p)", current->comm, tcmi_task_local_pid(self), npm_params);

	if (tcmi_taskhelper_checkpoint(self, npm_params) < 0) {
		mdbg(ERR3, "Failed to create a checkpoint");
		goto exit0;
	}
	tcmi_taskhelper_flushfiles();
	if (!(req = tcmi_ppm_p_migr_back_guestreq_procmsg_new_tx(tcmi_task_remote_pid(self), tcmi_task_ckpt_name(self)))) {
		mdbg(ERR3, "Error creating a migration back message");
		goto exit0;
	}
	if (tcmi_task_check_peer_lost(self, tcmi_task_send_anonymous_msg(self, req)) < 0) {
		mdbg(ERR3, "Failed to send message!!");
		goto exit1;
	}

	director_migrated_home(tcmi_task_local_pid(self));
	
	return 0;
 exit1:
	tcmi_msg_put(req);
 exit0:
	return -EFAULT;
}

/** 
 * \<\<private\>\> Migrates a task back to CCN using PPM with physical
 * checkpoint image. This requires:
 * - creating a new checkpoint of the current process (task helper job)
 * - flush open files of the current process (prevents write conflicts
 * after checkpoint restart)
 * - communicating the checkpoint name to the shadow
 * - terminating (via KILL_ME status)
 *
 * There are two modes how the migration back can be initiated:
 * -# PEN decides to migrate the task back - in that case, the
 * migration back message is sent as a request (trans_id is set
 * to invalid)
 * -# CCN decides to migrate the task back - the migration back
 * message is sent back as a response and as such is associated with a
 * transaction.
 * This method handles the first case.
 * @TODO: The second method is also handled here, but not exactly as described.. CCN send asynchronous request to migrate back and
 * then the guest initiates the processing of migration back.. we should likely rework it on transactional case described above
 *
 * @param *self - pointer to this task instance
 * @return TCMI_KILL_ME in either case (should it fail or not) as we want
 * the task away from the PEN.
 */
static int tcmi_guesttask_migrateback_ppm_p(struct tcmi_task *self)
{
	tcmi_guesttask_migrateback_p(self, NULL);
	// Migration back was requested so we kill in either case
	return TCMI_TASK_KILL_ME;
}

/**
 * Handles non-preemptive migration back of guest tasks
 */
int tcmi_guesttask_migrateback_npm(struct tcmi_task *self, struct tcmi_npm_params* npm_params)
{
	int res = tcmi_guesttask_migrateback_p(self, npm_params);

	if ( res ) {
		// npm migration has failed => continue with execution locally (we may try pre-emptive migration later and if that fails we kill the task)
		return TCMI_TASK_REMOVE_AND_LET_ME_GO;
	}

	// migration home succeeded => remove and kill guest task
	return TCMI_TASK_KILL_ME;
}


/** 
 * \<\<private\>\> Exit notification for the shadow on CCN.  This
 * method is scheduled by the exit system call hook that also invokes
 * the migration mode handler. It sends an exit process control
 * message to the shadow. No response is expected, the message content
 * is the exit code. The method terminates with
 * TCMI_TASK_REMOVE_AND_LET_ME_GO - explained below
 *
 * @return TCMI_TASK_REMOVE_AND_LET_ME_GO, so that the migration component
 * destroys this task. It doesn't itentionally return TCMI_TASK_KILL_ME
 * as it would cause another round of exit, this time using complete_and_exit()
 */
static int tcmi_guesttask_exit(struct tcmi_task *self, long code)
{
	struct tcmi_msg *m;
	
	/** Sync proxyfs files only if peer is alive, else this call makes no sense */
	if ( !self->peer_lost )
	    proxyfs_sync_files_on_exit();

	mdbg(INFO2, "Stub process '%s' local PID=%d terminating", 
	      current->comm, current->pid);
	if (!(m = tcmi_exit_procmsg_new_tx(tcmi_task_remote_pid(self), code))) {
		mdbg(ERR3, "Can't create error message");
		goto exit0;
	}
	/* send it as anonymous since no response is expected. */
	tcmi_task_send_anonymous_msg(self, m);
	/* dispose the message not needed anymore. */
	tcmi_msg_put(m);
 exit0:
	return TCMI_TASK_REMOVE_AND_LET_ME_GO;
}

/**
 * \<\<private\>\> Processes a specified signal.  Currently only
 * SIGKILL signal is handled that causes the task to terminate
 * returned status for the method pump - TCMI_TASK_KILL_ME.  Final
 * solution should do a full signal handling and communicate with the
 * shadow too.
 *
 * @param *self - pointer to this task instance 
 * @param signr - signal that is to be processed
 * @param *info - info for signal to be processed
 *
 * @return TCMI_TASK_KEEP_PUMPING on any signal but SIGKILL
 */
static int tcmi_guesttask_do_signal(struct tcmi_task *self, unsigned long signr, siginfo_t *info)
{
	int res = TCMI_TASK_KEEP_PUMPING;

	mdbg(INFO2, "Stub is processing signal %lu", signr);
	if (signr == SIGKILL || signr == SIGQUIT || signr == SIGINT) {
		tcmi_task_set_exit_code(self, signr);
		// We have to return keep pumping, because following do_exit call will invoke exit hook and it should be handled by the method pump!
		res = TCMI_TASK_KEEP_PUMPING;
	}

	return res;
}


/**
 * \<\<private\>\> Processes the physical emigrate process message (both NPM and PPM)
 *
 * The remote PID is extracted from the emigration message and stored
 * in the guest task. The checkpoint name is also extracted and passed
 * to the task helper to schedule a restart. This also sets the
 * checkpoint image as the most recent checkpoint of the task.
 *
 * A response is sent back that will contain the guest task local
 * PID. This PID is needed for further communication between the
 * shadow and guest tasks.
 *
 * The restart is only scheduled and will be processed upon next
 * iteration of the method pump. 
 *
 * @param *self - this guest task instance
 * @param *m - Physical emigration message that will be proceseed
 * @return TCMI_TASK_KEEP_PUMPING if the restart has been successfully
 * scheduled
 */
static int tcmi_guesttask_process_p_emigrate_msg(struct tcmi_task *self, 
						    struct tcmi_msg *m)
{
	struct tcmi_msg *resp;
	pid_t remote_pid;
	char *ckpt_name;

	/* extract the remote PID */
	remote_pid = tcmi_p_emigrate_msg_reply_pid(TCMI_P_EMIGRATE_MSG(m));
	tcmi_task_set_remote_pid(self, remote_pid);
	/* extract the checkpoint name */
	ckpt_name =  tcmi_p_emigrate_msg_ckpt_name(TCMI_P_EMIGRATE_MSG(m));
		
	mdbg(INFO2, "Processing emigration request, remote PID=%d, checkpoint: '%s'", tcmi_task_remote_pid(self), ckpt_name);

memory_sanity_check("On processing");

	/* schedules process restart from a checkpoint image */
	if (tcmi_taskhelper_restart(self, ckpt_name) < 0) {
		/* report a problem */
		if (!(resp = tcmi_err_procmsg_new_tx(TCMI_GUEST_STARTED_PROCMSG_ID, 
						     tcmi_msg_req_id(m), -ENOEXEC,
						     tcmi_task_remote_pid(self)))) {
						    
			mdbg(ERR3, "Cannot create a guest error response!!");
			goto exit0;
		} 
		memory_sanity_check("After restart");
	}	
	/* restart successfully scheduled, respond with our local PID */
	else {
		if (!(resp = tcmi_guest_started_procmsg_new_tx(tcmi_msg_req_id(m), 
							      tcmi_task_remote_pid(self), 
							      tcmi_task_local_pid(self)))) {
			mdbg(ERR3, "Cannot create a guest response!!");
			goto exit0;
		}
		memory_sanity_check("After restart 2");
	}
	/* preliminary announce - the guest has been started (execve still might fail) */
	tcmi_task_check_peer_lost(self, tcmi_task_send_anonymous_msg(self, resp));
	tcmi_msg_put(resp);
	
	
	return TCMI_TASK_KEEP_PUMPING;
		
	/* error handling */
 exit0:
	return TCMI_TASK_KILL_ME;
}

int tcmi_guesttask_post_fork(struct tcmi_task* self, struct tcmi_task* child, long fork_result, pid_t remote_child_pid) {
	struct tcmi_msg *m;

	if ( fork_result < 0 ) {
		// Notify CCN about fork-failed
		// We're reusing exit msg here, but it may be better to introduce a specific event for this, right?
		if (!(m = tcmi_exit_procmsg_new_tx(remote_child_pid, fork_result))) {
			mdbg(ERR3, "Can't create failed-fork exit message");
			goto exit0;
		}
	} else {
		BUG_ON(child == NULL); // If fork succeeded, child cannot be null, right?

		tcmi_task_set_remote_pid(child, remote_child_pid);		

		if (!(m = tcmi_guest_started_procmsg_new_tx(TCMI_TRANSACTION_INVAL_ID, 
							      tcmi_task_remote_pid(child), 
							      tcmi_task_local_pid(child)))) {
			mdbg(ERR3, "Cannot create a guest response in post-fork!");
			goto exit0;
		}		
	}

	tcmi_task_send_anonymous_msg(self, m);
	tcmi_msg_put(m);

	return 0;

exit0:
	// TODO: Some better failure handling? Shall we stop this task, if we've failed to inform CCN that it is running? CCN does not know it's ID in this case...
	return -EINVAL;
}

/** \<\<public\>\> Used to set proper tid after a new process was forked.. it must be run in that process context */
int tcmi_guesttask_post_fork_set_tid(void *self, struct tcmi_method_wrapper *wr) {
	int res = TCMI_TASK_KEEP_PUMPING;
	struct tcmi_task *self_tsk = TCMI_TASK(self);

	mdbg(INFO3, "Task - remote pid %d being set after fork",
	     tcmi_task_remote_pid(self_tsk));

        if (current->set_child_tid) {
		mdbg(INFO3, "Tid set required");	     	
                put_user(tcmi_task_remote_pid(self_tsk), current->set_child_tid);
	}

	if ( thread_group_leader(current) ) { // Proxyfs file names changing is required only for group leaders
		// TODO: Make a separate method for this, it does not belong here
		// Modifies proxyfs file references to references specific to this task
		if ( proxyfs_clone_file_names(tcmi_task_remote_pid(self_tsk)) ) {
			mdbg(INFO3, "Task proxy fs clonning has failed. Pid %d", tcmi_task_remote_pid(self_tsk));
			// We cannot simply return KILL_ME here.. this would remove current TCMI task and kill the process,
			// but the exit hook won't be called!
			force_sig(SIGKILL, current);
		}
	}

	return res;
}


/** Return type of task. Only for setting task type to kernel task_struct. Polymorphism 
 * cannot be used in kernel, bacause tcmi_task is defined outside of kernel. */
static enum tcmi_task_type tcmi_guesttask_get_type(void){
	return guest;
}

/** TCMI task operations that support polymorphism */
static struct tcmi_task_ops guesttask_ops = {
	.process_msg = tcmi_guesttask_process_msg,
	.emigrate_ppm_p = tcmi_guesttask_emigrate_p,	
	.migrateback_ppm_p = tcmi_guesttask_migrateback_ppm_p,
	.migrateback_npm = tcmi_guesttask_migrateback_npm,
	.exit = tcmi_guesttask_exit,
	.get_type = tcmi_guesttask_get_type,
	.do_signal = tcmi_guesttask_do_signal
};

/**
 * @}
 */
