/**
 * @file tcmi_shadowtask.c - TCMI shadow task, migrated process abstraction on CCN
 *                      
 * 
 *
 *
 * Date: 04/26/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_shadowtask.c,v 1.17 2009-04-06 21:48:46 stavam2 Exp $
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
#include <tcmi/syscall/tcmi_shadow_rpc.h>
#include <tcmi/comm/tcmi_rpc_procmsg.h>
#include <tcmi/comm/tcmi_rpcresp_procmsg.h>
#include <tcmi/comm/tcmi_ppm_p_migr_back_shadowreq_procmsg.h>
#include <tcmi/manager/tcmi_migman.h>
#include <tcmi/migration/tcmi_npm_params.h>
#include "tcmi_taskhelper.h"

#define TCMI_SHADOWTASK_PRIVATE
#include "tcmi_shadowtask.h"
#include <asm/signal.h>
#include <proxyfs/proxyfs_server.h>

#include <director/director.h>

/** 
 * This is just a "assertion check" handler. It should never be calles since all signals
 * of the CCN should be processed in its main processing loop. 
 *
 * When the process is migrated away, this handler is registered as a handler of all its assocaited signals.
 * When the process is migrated back, its real handlers are restored.
 */
static void tcmi_shadowtask_signal_handler(int sig)
{
	mdbg(ERR3, "Shadow signal handler for signal %d called", sig);
}

static struct k_sigaction signal_action = {
	.sa = {
		.sa_handler = &tcmi_shadowtask_signal_handler,
		.sa_mask = {
			#if defined(__x86_64__) 
			.sig = { 0xffffFFFFffffFFFF }
			#else
			.sig = { 0xffffFFFF, 0xffffFFFF },
			#endif
		},
		.sa_flags = SA_SIGINFO,
		.sa_restorer = NULL,
	},
};


/** 
 * \<\<public\>\> Instance constructor.
 * - allocates a new instance.
 * - delegates its initialization to the super class.
 *
 * @param local_pid - local PID on CCN
 * @param migman - The migration manager that maintains this task
 * @param *sock - socket used for communication with guest task on PEN
 * @param *d_migproc - directory where ctlfs entries of migrated
 * processes reside
 * @param *d_migman - TCMI ctlfs directory of the migration manager
 * responsible for this task
 * @return 0 upon success
 */
struct tcmi_task* tcmi_shadowtask_new(pid_t local_pid, struct tcmi_migman* migman,
				      struct kkc_sock *sock, 
				      struct tcmi_ctlfs_entry *d_migproc, 
				      struct tcmi_ctlfs_entry *d_migman)
{
	struct tcmi_shadowtask *task;
	minfo(INFO2, "Creating new shadow task for local PID=%d", local_pid);
	if (!(task = TCMI_SHADOWTASK(kmalloc(sizeof(struct tcmi_shadowtask), 
					     GFP_ATOMIC)))) {
		minfo(ERR3, "Can't allocate memory for TCMI shadow task");
		goto exit0;
	}
	if (tcmi_task_init(TCMI_TASK(task), local_pid, migman, sock, d_migproc, 
			   d_migman, &shadowtask_ops) < 0) {
		minfo(ERR3, "TCMI shadow task initialization failed!");
		goto exit1;
	}
	return TCMI_TASK(task);

	/* error handling */
 exit1:
	kfree(task);
 exit0:
	return NULL;
}

/** @addtogroup tcmi_shadowtask_class
 *
 * @{
 */

/** 
 *  \<\<private\>\> Processes a message specified by the caller.
 *
 * @param *self - pointer to this task instance
 * @param *m - message to be processed
 * @return depends on the type of message that has been processed
 */
static int tcmi_shadowtask_process_msg(struct tcmi_task *self, struct tcmi_msg *m)
{
	int res = TCMI_TASK_KEEP_PUMPING;
	struct tcmi_msg *resp = NULL;
	pid_t remote_pid;
	int err;
	
	/* struct tcmi_shadowtask *self_tsk = TCMI_SHADOWTASK(self); */

	switch (tcmi_msg_id(m)) {
		case TCMI_RPC_PROCMSG_ID:
			mdbg(INFO1, "Receiced RPC message: %x", tcmi_msg_id(m));

			tcmi_rpc_call2(tcmi_shadow_rpc, tcmi_rpc_procmsg_num(TCMI_RPC_PROCMSG(m)), (long)m, (long)(&resp));

			if ( resp == NULL ) {
				minfo(ERR3, "RPC#%d didn't returned rpc response message", 
						tcmi_rpc_procmsg_num(TCMI_RPC_PROCMSG(m)));
				break;
			}

			if ( tcmi_task_check_peer_lost(self, (err = tcmi_task_send_anonymous_msg(self, resp)) ) ){
				minfo(ERR3, "Error sending RPC response message %d", err);
				
				if ( err == -ERESTARTSYS ) {
				    minfo(INFO3, "Scheduling message for resubmission");  
				    /* Protect response reference, retransmission method will be responsible for releasing of the msg */
				    tcmi_msg_get(resp);
				    tcmi_task_submit_method(self, tcmi_task_send_message, resp, sizeof(struct tcmi_msg));
				}
			}

			tcmi_msg_put( resp );

			break;
		/* exit request */
		case TCMI_EXIT_PROCMSG_ID:
			mdbg(INFO2, "Process '%s' local PID=%d, remote PID=%d terminating, "
					"code: %ld, anounced by guest",
					current->comm, tcmi_task_local_pid(self), 
					tcmi_task_remote_pid(self),
					(long)tcmi_exit_procmsg_code(TCMI_EXIT_PROCMSG(m)));
			tcmi_task_set_exit_code(self, tcmi_exit_procmsg_code(TCMI_EXIT_PROCMSG(m)));
			res = TCMI_TASK_KILL_ME;
			break;
		/* vfork done on guest side */
		case TCMI_VFORK_DONE_PROCMSG_ID:
			mdbg(INFO2, "Process '%s' local PID=%d, remote PID=%d vfork done",
					current->comm, tcmi_task_local_pid(self), 
					tcmi_task_remote_pid(self));
			tcmi_shadowtask_vfork_done(self);
			break;
			/* migrate back request */
		case TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG_ID:
			res = tcmi_shadowtask_process_ppm_p_migr_back_guestreq_procmsg(self, m);
			break;
		case TCMI_GUEST_STARTED_PROCMSG_ID: // We can get here only in double-fork
			remote_pid = tcmi_guest_started_procmsg_guest_pid(TCMI_GUEST_STARTED_PROCMSG(m));
			mdbg(INFO3, "Fork confirmation from guesttask - task migrated, local PID %d, guest PID %d", tcmi_task_local_pid(self), remote_pid);
			tcmi_task_set_remote_pid(self, remote_pid);
			break;
		default:
			mdbg(ERR3, "Unexpected message from the guest task: %x", tcmi_msg_id(m));
			break;
	}

	return res;
}

static void tcmi_shadowtask_vfork_done(struct tcmi_task *self) {
	struct completion *vfork_done = current->vfork_done;
	if ( current->vfork_done ) {
		current->vfork_done = NULL;
		complete(vfork_done);				
	}
}

/** Internal helper method that can perform both NPM and PPM physical emigration */
static int tcmi_shadowtask_emigrate_p(struct tcmi_task *self, struct tcmi_npm_params* npm_params) {
	struct tcmi_msg *req, *resp;
	char* exec_name;
	u64 beg_time, end_time;
	
	beg_time = cpu_clock(smp_processor_id());
	
	if ( npm_params ) {
		// For NPM, the exec name is name of the file being executed
		exec_name = npm_params->file_name;
	} else {
		// TODO: Detect this from current in case of PPM.. how? ;)	
		exec_name = current->comm;
	}

	mdbg(INFO2, "Process '%s' - local PID %d, emigrating",
	     current->comm, tcmi_task_local_pid(self));

	if (tcmi_taskhelper_checkpoint(self, npm_params) < 0) {
		mdbg(ERR3, "Failed to create a checkpoint");
		goto exit0;
	}

	if (!(req = tcmi_p_emigrate_msg_new_tx(tcmi_task_transactions(self), 
						   tcmi_task_local_pid(self), 
						   exec_name,
						   tcmi_task_ckpt_name(self),
						   current_euid(),
						   current_egid(),
						   current_fsuid(),
						   current_fsgid()))) {
		mdbg(ERR3, "Error creating an emigration message");
		goto exit0;
	}
	if ( tcmi_task_check_peer_lost(self, tcmi_task_send_and_receive_msg(self, req, &resp)) < 0) {
		mdbg(ERR3, "Failed to send message!!");
		goto exit1;
	}
	if (tcmi_shadowtask_verify_migration(self, resp) < 0) {
		mdbg(ERR3, "Failed to migrate the task '%s'", current->comm);
		goto exit2;
	}

	end_time = cpu_clock(smp_processor_id());
	mdbg(INFO3, "Emigration (npm: %d) took '%llu' ms.'", npm_params != NULL, (end_time - beg_time) / 1000000);
	printk("Emigration (npm: %d) took '%llu' ms.\n'", npm_params != NULL, (end_time - beg_time) / 1000000);
	
	/* 
	   Files are flushed only after we have confirmation that the migration have succeeded.
 	   Otherwise we keep them open so that we can eventually continue execution of the current
           process.
         */
	tcmi_taskhelper_flushfiles();
	tcmi_taskhelper_catch_all_signals(&signal_action);

	mdbg(INFO3, "Submitting process_msg");
	tcmi_task_submit_method(self, tcmi_task_process_msg, NULL, 0);
	/* release the message */
	tcmi_msg_put(req);
	tcmi_msg_put(resp);

	// In case the emigration succeeded, we are responsible for freeing of npm params, as the caller won't ever get chance
	if ( npm_params )
		vfree(npm_params);

	return TCMI_TASK_KEEP_PUMPING;
	/* error handling */
 exit2:
	tcmi_msg_put(resp);
 exit1:
	tcmi_msg_put(req);
 exit0:
	/* either checkpointing,communication or restart has failed. In either case we simply continue execution of the current task */
	return TCMI_TASK_REMOVE_AND_LET_ME_GO;
};

/** 
 * \<\<private\>\> Emigrates a task to a PEN.
 * - create a new checkpoint
 * - flush open files of the current process (prevents write conflicts
 * after checkpoint restart)
 * - creates a new emigration message specifying latest checkpoint name
 * and shadow task PID
 * - send the message waiting for a response
 * - if the guest has been succesfully started submit process_msg method into
 * the method queue and quit (migration verification). The submitted method
 * will keep on processing any message that will arrive on the queue.
 *
 * @param *self - pointer to this task instance
 * @return TCMI_TASK_KEEP_PUMPING, if the shadow has received a positive 
 * response and the task has been successfully migrated.
 *
 * @todo - after creating a checkpoint and preparing the emigration
 * message there a few things that need to be done:
 *
 * - some component should scan all open files and close all regular
 * files as they will be accessed from the network filesystem.
 * - character, block devices, pipes, sockets should stay open so that
 * remote operations can be forwarded locally.
 * - the current execution image should be flushed as it is not needed
 * anymore
 * 
 */
static int tcmi_shadowtask_emigrate_ppm_p(struct tcmi_task *self)
{
	return tcmi_shadowtask_emigrate_p(self, NULL);
}

static int tcmi_shadowtask_emigrate_npm(struct tcmi_task *self, struct tcmi_npm_params* npm_params)
{
	return tcmi_shadowtask_emigrate_p(self, npm_params);
}


/** 
 * \<\<private\>\> Migrates a task back to CCN. This is done simply by sending
 * a migrate back request to the associated ghost task, which will handle its
 * migration home the same way as if it was requested on PEN
 *
 * @param *self - pointer to this task instance
 * @return result of the task specific migrate back method or 
 * TCMI_TASK_KEEP_PUMPING
 */
static int tcmi_shadowtask_migrateback_ppm_p(struct tcmi_task *self)
{
	int res = TCMI_TASK_KEEP_PUMPING;
	struct tcmi_msg *msg;
	/* struct tcmi_shadowtask *self_tsk = TCMI_SHADOWTASK(self); */

	mdbg(INFO2, "Process '%s' - local PID %d, remote PID %d, migrate home requested",
	     current->comm, tcmi_task_local_pid(self), tcmi_task_remote_pid(self));

	if ( !(msg = tcmi_ppm_p_migr_back_shadowreq_procmsg_new_tx(tcmi_task_remote_pid(self))) ) {
		mdbg(ERR3, "Error creating a migrate back message");
		goto exit0;
	}

	if ( tcmi_task_check_peer_lost(self, tcmi_task_send_anonymous_msg(self, msg)) < 0) {
		mdbg(ERR3, "Failed to send message!!");
		goto exit1;
	}
exit1:
	tcmi_msg_put(msg);
exit0:	
	return res;
}


/** 
 * \<\<private\>\> Execve notification when merging shadow with
 * migrating process on CCN.  This method is called right before
 * issuing the actual execve on the checkpoint image of the migrated
 * task. Since the task will be merged with the original process, it
 * needs to be remove from the TCMI. All that needs to be done is
 * scheduling a tcmi_task_exit() method. The migration mode handler
 * takes care of the rest.
 * 
 * @param *self - pointer to this task instance 
 * @return 0 upon success
 */
static int tcmi_shadowtask_execve(struct tcmi_task *self)
{
	tcmi_task_flush_and_submit_method(self, tcmi_task_exit, NULL, 0);
	/* migration mode is entered when the actual execve resumes to
	 * user mode */
	tcmi_taskhelper_enter_mig_mode(self);
	return 0;
}

/**
 * \<\<private\>\> Processes a specified signal.  
 * All signals are forwared to the guest task.
 *
 * @param *self - pointer to this task instance 
 * @param signr - signal that is to be processed
 * @param *info - info for signal to be processed
 *
 * @return TCMI_TASK_KEEP_PUMPING on any signal but second or more SIGKILL
 */
static int tcmi_shadowtask_do_signal(struct tcmi_task *self, unsigned long signr, siginfo_t *info)
{
	int res = TCMI_TASK_KEEP_PUMPING;
	struct tcmi_msg *msg;

	mdbg(INFO2, "Shadow is processing signal %lu", signr);
	
	msg = tcmi_signal_msg_new(tcmi_task_remote_pid(self), info);
	if( msg ){
		tcmi_task_check_peer_lost(self, tcmi_msg_send_anonymous(msg, tcmi_migman_sock(self->migman)));
		mdbg(INFO2, "Signal message send");
	}
		
	return res;
}

/**
 * \<\<private\>\> Verifies, that the task has been successfully migrated.
 * The verification is done based on response message the has to be
 * TCMI_GUEST_STARTED_PROCMSG and has to carry a non-zero guest PID.
 * If the verification is positive, the guest PID is stored inside
 * this shadow instance. It will be used for further communication
 * with the guest.
 *
 * @param *self - pointer to this task instance
 * @param *resp - pointer to the response to be processed
 * @return 0 if the task has been successfully migrated
 */
static int tcmi_shadowtask_verify_migration(struct tcmi_task *self, struct tcmi_msg *resp)
{
	int err = -EINVAL;
	pid_t remote_pid;

	if (!resp) {
		mdbg(ERR3, "No response from guest task has arrived..");
		goto exit0;
	}
	switch (tcmi_msg_id(resp)) {
		/* regular version */
	case TCMI_GUEST_STARTED_PROCMSG_ID:
		remote_pid = tcmi_guest_started_procmsg_guest_pid(TCMI_GUEST_STARTED_PROCMSG(resp));
		mdbg(INFO3, "Confirmation from guesttask - task migrated, local PID %d, guest PID %d", tcmi_task_local_pid(self), remote_pid);
		if ( remote_pid == -1 ) {
		    err = -EINVAL;
		} else {
		  tcmi_task_set_remote_pid(self, remote_pid);
		  err = 0;
		}
		break;
		/* error version */
	case TCMI_MSG_FLG_SET_ERR(TCMI_GUEST_STARTED_PROCMSG_ID):
		mdbg(ERR3, "Stub hasn't been started, migration failed..");
		break;
	default:
		mdbg(ERR3, "Unexpected response from the guest task: %x", tcmi_msg_id(resp));
		break;
	}
	/* error handling */
 exit0:
	return err;
}


/**
 * \<\<private\>\> Processes the PPM_P emigrate process message.
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
 * @param *m - PPM_P migrate back guest request
 * @return TASK_KEEP_PUMPING if the restart has been successfully
 * scheduled
 */
static int tcmi_shadowtask_process_ppm_p_migr_back_guestreq_procmsg(struct tcmi_task *self, 
								   struct tcmi_msg *m)
{
	char *ckpt_name;

	/* extract the checkpoint name */
	ckpt_name =  tcmi_ppm_p_migr_back_guestreq_procmsg_ckpt_name
		(TCMI_PPM_P_MIGR_BACK_GUESTREQ_PROCMSG(m));
		
	mdbg(INFO2, "Processing migrate back request, remote PID=%d, checkpoint: '%s'",
		     tcmi_task_remote_pid(self), ckpt_name);

	director_migrated_home(tcmi_task_local_pid(self));
	
	/* schedules process restart from a checkpoint image */
	if (tcmi_taskhelper_restart(self, ckpt_name) < 0) {
		mdbg(ERR3, "Cannot setup task restart!!");
		goto exit0;

	}
	return TCMI_TASK_KEEP_PUMPING;
		
	/* error handling */
 exit0:
	return TCMI_TASK_KILL_ME;
}

/** \<\<private\>\> Custom free method */
static void tcmi_shadowtask_free(struct tcmi_task* self)
{
}


/** Return type of task. Only for setting task type to kernel task_struct. Polymorphism 
 * cannot be used in kernel, bacause tcmi_task is defined outside of kernel. */
enum tcmi_task_type tcmi_shadowtask_get_type(void)
{
	return shadow;
}

/** TCMI task operations that support polymorphism */
static struct tcmi_task_ops shadowtask_ops = {
	.process_msg = tcmi_shadowtask_process_msg,
	.emigrate_ppm_p = tcmi_shadowtask_emigrate_ppm_p,
	.emigrate_npm = tcmi_shadowtask_emigrate_npm,
	.migrateback_ppm_p = tcmi_shadowtask_migrateback_ppm_p,
	.execve = tcmi_shadowtask_execve,
	.get_type = tcmi_shadowtask_get_type,
	.do_signal = tcmi_shadowtask_do_signal,
	.free = tcmi_shadowtask_free
};

/**
 * @}
 */
