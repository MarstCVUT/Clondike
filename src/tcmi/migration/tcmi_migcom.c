/**
 * @file tcmi_migcom.c - TCMI migration component.
 *                      
 * 
 *
 *
 * Date: 05/05/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_migcom.c,v 1.13 2009-04-06 21:48:46 stavam2 Exp $
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

#include <tcmi/task/tcmi_taskhelper.h>
#include <tcmi/task/tcmi_shadowtask.h>
#include <tcmi/task/tcmi_guesttask.h>
#include <tcmi/lib/util.h>

#define TCMI_MIGCOM_PRIVATE
#include "tcmi_migcom.h"
#include <tcmi/manager/tcmi_penmigman.h>
#include <tcmi/migration/fs/fs_mounter_register.h>
#include <tcmi/migration/fs/fs_mounter.h>
#include "tcmi_npm_params.h"

#include <director/director.h>

#include <dbg.h>


static int mount_proxyfs(struct kkc_sock* sock) {	
	int err = 0, len;
	char buffer[KKC_MAX_ARCH_LENGTH + KKC_SOCK_MAX_ADDR_LENGTH + 2];	
	char* last_colon_char;
	struct cred* new_cred;

	new_cred = prepare_creds();
	if ( !new_cred )
		return -ENOMEM;

	// TODO: Make mount dir configurable?
	
	memset(buffer, 0, KKC_MAX_ARCH_LENGTH + KKC_SOCK_MAX_ADDR_LENGTH + 2);
	kkc_sock_getarchname(sock, buffer, KKC_MAX_ARCH_LENGTH);
	len = strlen(buffer);
	buffer[len] = ':';
	kkc_sock_getpeername(sock, buffer + len + 1, KKC_SOCK_MAX_ADDR_LENGTH);
	// TODO: Ugly ugly... works only for specific format of string and in addition port is hardcoded here
	last_colon_char = strrchr(buffer, ':');	
	sprintf(last_colon_char+1, "%d", 1112); 
	
	mdbg(INFO2, "Going to mount proxyfs on dev: %s fsuid: %d euid: %d csa: %d euid: %d uid: %d, suid: %d", buffer, current_fsuid(), current_euid(), capable(CAP_SYS_ADMIN), current_euid(), current_uid(), current_suid() );

	// TODO: Ohh man.. this is something terrible.. we should fiddle better with the fs rights, so that we do not need this
	// The problem is, that all works fine for newly created immigrated processes, since they have these capabilities, but
	// when we start using forked processes, they do not and they have a lot of problems with FS access, for example to proxy-fs			
	cap_raise (new_cred->cap_effective, CAP_SYS_ADMIN);
	cap_raise (new_cred->cap_effective, CAP_DAC_READ_SEARCH);
	cap_raise (new_cred->cap_effective, CAP_DAC_OVERRIDE);  
	cap_raise (new_cred->cap_effective, CAP_FOWNER);
	cap_raise (new_cred->cap_effective, CAP_FSETID);

	commit_creds(new_cred);

	err = do_mount(buffer, "/mnt/proxy", "proxyfs", 0, NULL);
	mdbg(INFO2, "After Proxyfs mount: %d", err);
	if ( err ) {
		minfo(ERR1, "Failed to mount filesystem: %d", err);
		goto exit0;
	}	

exit0:
	return err;
}

static const char proc_path[] = "/mnt/local/proc";
/** Mounts local machine procfs into some directory in chrooted env, so that it can be later accessed if required */
static int mount_local_procfs(struct kkc_sock* sock) {
	struct nameidata nd;
	int err;

	err = path_lookup(proc_path, LOOKUP_FOLLOW | LOOKUP_DIRECTORY, &nd);
	if ( err ) {
		minfo(ERR1, "Failed to lookup path %s: %d. Trying to create", proc_path, err);
		err = mk_dir(proc_path, 777);
		if (err) {
			goto exit0;
		}
	}

	err = do_mount("none", proc_path, "proc", 0, NULL);
	mdbg(INFO2, "Procfs mount result: %d", err);
	if ( err ) {
		minfo(ERR1, "Failed to mount filesystem: %d", err);
		goto exit0;
	}	

exit0:
	return err;
}

static int tcmi_task_mount_proxyfs(void *self, struct tcmi_method_wrapper *wr) {
	int res;
	struct kkc_sock* sock = *(struct kkc_sock**)tcmi_method_wrapper_data(wr);
	if ( (res =mount_proxyfs(sock)) ) {
		minfo(ERR1, "Failed to mount proxy filesystem: %d", res);
		// Kill current process.. do not exit via kill me as it does not invoke exit hook
		force_sig(SIGKILL, current);
	}
	return TCMI_TASK_KEEP_PUMPING;
}

/** 
 * \<\<public\>\> Migrates a task from a CCN to a PEN. 
 * This requires:
 * - instantiating a new shadow task - requires migration manager ID,
 * communication socket, migproc directory(where the shadow will
 * present its information) and root directory of the migration
 * manager(for a symbolic link)
 * - submitting emigration method
 * - attaching the shadow task to the target process that is
 * to be migrated
 * - forcing the target process into migration mode
 * - checking if the process has properly picked up the shadow task
 *
 * This method exclusively uses preemptive process migration using
 * physical checkpoint image.
 *
 * @param pid - task that is to be migrated
 * @param *migman - migration manager that will provide the communication
 * channel for the migrated task.
 * @return 0 upon success;
 */
int tcmi_migcom_emigrate_ccn_ppm_p(pid_t pid, struct tcmi_migman *migman)
{
	int err = 0;
	struct tcmi_task *shadow;

	/* create a new PPM shadow task for physical ckpt image  */
	if (!(shadow = 
	      tcmi_shadowtask_new(pid, migman, 
				  tcmi_migman_sock(migman), 
				  tcmi_migman_migproc_dir(migman), 
				  tcmi_migman_root(migman)))) {
		minfo(ERR3, "Error creating a shadow task");
		goto exit0;
	}

	/* submit the emigrate method */
	mdbg(INFO3, "Submitting emigrate_ppm_p");
	tcmi_task_submit_method(shadow, tcmi_task_emigrate_ppm_p, NULL, 0);

	/* attaches the shadow to its thread. */
	if (tcmi_taskhelper_attach(shadow, tcmi_migcom_mig_mode_handler) < 0) {
		mdbg(ERR3, "Failed attaching shadow PID=%d to its thread", 
		     tcmi_task_local_pid(shadow));
		goto exit1;
	}

	/* switch the task to migration mode */	
	tcmi_taskhelper_enter_mig_mode(shadow);

	/* wait for pickup */
	if (tcmi_taskhelper_wait_for_pick_up_timeout(shadow, 2*HZ) < 0) {
		mdbg(INFO1, "Shadow not picked up: %p", shadow);
		director_emigration_failed(pid);
	} else {
		mdbg(INFO1, "Shadow successfully picked up: %p", shadow);
	}

	/* release the shadow instance reference (Reference is now held by the associated task_struct - it got it in attach method)*/
	tcmi_task_put(shadow);

	return 0;

	/* error handling */
 exit1:
	tcmi_task_put(shadow);
 exit0:
	return err;

}

/** \<\<public\>\> Migrates non-preemptively a task from a CCN to a PEN */
int tcmi_migcom_emigrate_ccn_npm(pid_t pid, struct tcmi_migman *migman, struct pt_regs* regs, struct tcmi_npm_params* npm_params) {
	int err = 0;
	struct tcmi_task *shadow;

	/* create a new PPM shadow task for physical ckpt image  */
	if (!(shadow = 
	      tcmi_shadowtask_new(pid, migman, 
				  tcmi_migman_sock(migman), 
				  tcmi_migman_migproc_dir(migman), 
				  tcmi_migman_root(migman)))) {
		minfo(ERR3, "Error creating a shadow task");
		goto exit0;
	}
	/* submit the emigrate method */
	mdbg(INFO3, "Submitting emigrate_ppm_p");
	tcmi_task_submit_method(shadow, tcmi_task_emigrate_npm, &npm_params, sizeof(void*));

	/* attaches the shadow to its thread. */
	if (tcmi_taskhelper_attach(shadow, tcmi_migcom_mig_mode_handler) < 0) {
		mdbg(ERR3, "Failed attaching shadow PID=%d to its thread", 
		     tcmi_task_local_pid(shadow));
		goto exit1;
	}


	/* release the shadow instance reference (Reference is now held by the associated task_struct - it got it in attach method)*/
	tcmi_task_put(shadow);
	/* Task now holds its reference to a migman so we can release it */
	tcmi_migman_put(migman);

	/* We are already in migmode so we can directly execute handler here */
	tcmi_taskhelper_do_mig_mode(regs);
	//tcmi_migcom_mig_mode_handler();
			
	// We get here only if npm migration failed, othewise either of the following happens:
	// - task finishes remotely, so the handler exists
	// - task migrates back, so it must execve to a new checkpoint => we still do not get here
	mdbg(INFO3, "Got after npm migration handler start");
	
	director_emigration_failed(pid);

	// Ok, the emigration failed. Task is going to release its reference to migman and we've release the reference we got
	// from the manager.. we have to retake this reference so that the release made by manager has matching get
	// TODO: This is incorrect.. task is already freed and has freed its reference! It may happen, that the migman is already freed!
	// How to solve this nicely? .. ;)
	tcmi_migman_get(migman);
	return 0;

	/* error handling */
 exit1:
	tcmi_task_put(shadow);
 exit0:
	return err;

};

/** Helper struct to be passed to immigration statup thread */
struct  immigrate_startup_params {
	/** used to signal the thread the guest(or fs mount respectively) is ready */	
	struct completion guest_ready, fs_ready;
	/** Params for mounting of distributed fs */
	struct fs_mount_params* mount_params;
	/** Migman socket.. used to determine peers address so that we can mount its proxyfs */
	struct kkc_sock* sock;
	/** Credentials that can be used for fs mounting */
	int16_t fsuid, fsgid;
};

/** 
 * \<\<public\>\> Immigrates a task - handles accepting the task on
 * the PEN side.  This method immigrates a new task based on an
 * emigration message that a PEN has received as follows:
 *
 * - creates a new kernel thread that will wait until the new guest
 * process is instantiated (waits with timeout on guest_ready
 * completion)
 * - instantiates a new guest task. 
 * - delivers the migration message to it and makes a necessary setup,
 * so that the guest task processes the message as soon as it becomes
 * a regular migrated process
 * - attaches the guest to its new process(thread)
 * - announces that the guest instance is ready to the thread via completion
 * - waits for the thread to pickup the guest and become a migrated
 * process
 *
 * @param *m - a message based on which we are immigrating the task
 * @param *migman - migration manager that will provide the communication
 * channel for the migrated task.
 * migration information
 * @return 0 upon success
 */
int tcmi_migcom_immigrate(struct tcmi_msg *m, struct tcmi_migman *migman)
{
	int err = -EINVAL;
	int pid;
	int wait_for_msg = 0;

	struct fs_mounter* mounter;

	struct immigrate_startup_params startup_params;
	struct tcmi_task *guest;
	struct tcmi_penmigman* penmigman = TCMI_PENMIGMAN(migman);
	struct tcmi_p_emigrate_msg* msg = TCMI_P_EMIGRATE_MSG(m);
	int accept = 0;


	int call_res = director_immigration_request(tcmi_migman_slot_index(migman), tcmi_p_emigrate_msg_euid(msg), tcmi_p_emigrate_msg_exec_name(msg), &accept);
	if ( call_res == 0 ) {
		if ( !accept ) {
			mdbg(INFO2, "Immigration rejected by director.");
			// Rejected immigration
			return -EINVAL;
		}
	}


	init_completion(&startup_params.guest_ready);
	init_completion(&startup_params.fs_ready);
	startup_params.mount_params = tcmi_penmigman_get_mount_params(penmigman);
	startup_params.sock = tcmi_migman_sock(migman);
	startup_params.fsuid = tcmi_p_emigrate_msg_fsuid(msg);
	startup_params.fsgid = tcmi_p_emigrate_msg_fsgid(msg);

	// Global part of mount
	mounter = get_new_mounter(startup_params.mount_params);	
	if ( mounter && mounter->global_mount ) {
		/* Some file system mounter is registered => perform the mount */
		mdbg(INFO2, "Doing global mount");
		err = mounter->global_mount(mounter, startup_params.fsuid, startup_params.fsgid);
		if ( err ) {
			free_mounter(mounter);
			goto exit0;
		}
	}
	free_mounter(mounter);


	/* new thread is started with a private namespace so that the mounts in that thread are not global */
	if ((pid = kernel_thread(tcmi_migcom_migrated_task, &startup_params, CLONE_NEWNS)) < 0) {
		mdbg(ERR3, "Cannot start kernel thread for guest process %d", pid);
		goto exit0;
	}

	/* before we proceed with restarting, we have to wait till the file system mount is ready (if it is performed) */
	if ((err = wait_for_completion_interruptible(&startup_params.fs_ready))) {
		minfo(ERR3, "Received signal fs not ready");
		goto exit0;
	}
	
	if (!(guest = tcmi_guesttask_new(pid, migman, 
					tcmi_migman_sock(migman), 
					tcmi_migman_migproc_dir(migman), 
					tcmi_migman_root(migman)))) {
		mdbg(ERR3, "Error creating a guest task");
		goto exit0;
	}
	/* have the guest task deliver the message to itself, extra message reference is for the guest.*/
	if (tcmi_task_deliver_msg(guest, tcmi_msg_get(m)) < 0) {
		mdbg(ERR3, "Error delivering the message to the guest process");
		tcmi_msg_put(m);
	}
	/* The message will be processed first. Submitted version does not wait for other messages,
	so that it does not block potential processing of other methods later. */
	mdbg(INFO3, "Submitting process_msg");
	tcmi_task_submit_method(guest, tcmi_task_process_msg, &wait_for_msg, sizeof(int));

	/* attaches the guest to its thread. */
	if (tcmi_taskhelper_attach(guest, tcmi_migcom_mig_mode_handler) < 0) {
		minfo(ERR3, "Failed attaching guest PID=%d to its thread", 
		      tcmi_task_local_pid(guest));
		goto exit1;
	}

	/* signal the new thread that the guest is ready. */
	complete(&startup_params.guest_ready);

	/* wait for pickup */
	if (tcmi_taskhelper_wait_for_pick_up_timeout(guest, 2*HZ) < 0) {
		mdbg(ERR1, "Guest not picked up: %p!!!!", guest);
		goto exit1;
	}
	mdbg(INFO2, "Guest successfully picked up: %p", guest);
	director_immigration_confirmed(tcmi_migman_slot_index(migman), tcmi_p_emigrate_msg_euid(msg), tcmi_p_emigrate_msg_exec_name(msg), pid, tcmi_p_emigrate_msg_reply_pid(msg));	
	tcmi_task_put(guest);
	return 0;

	/* error handling */
 exit1:
	tcmi_task_put(guest);
 exit0:
	return err;

}


/** 
 * \<\<public\>\> Migrates a task back to its home node using
 * preemptive migration with physical checkpoint file. The approach is
 * the same on CCN and PEN:
 *
 * - send a notification to a process with a given PID.
 * - the priority is set to 1 which causes all method from the
 * TCMI task to be flushed from its method queue.
 *
 * @param pid - task that is to be migrated
 * @return 0 upon success;
 */
int tcmi_migcom_migrate_home_ppm_p(pid_t pid)
{
	/* set priority for the method - causes all methods to be flushed */
	int prio = 1;
	mdbg(INFO4, "request to migrate home pid %d", pid);
	return tcmi_taskhelper_notify_by_pid(pid, tcmi_task_migrateback_ppm_p, 
					     NULL, 0, prio);
}


/** @addtogroup tcmi_migcom_class
 *
 * @{
 */

/** 
 * \<\<private\>\> This method represents a kernel thread that will
 * eventually become the 'immigrated' task. It does following:
 * 
 * - tries to mount a distributed filesystem, if required (note, that this must be done by this thread as we are mounting into its private namespace)
 * - mounts a proxyfs into a private namespace
 * - waits until signalled, that the guest is ready
 * - runs its migration mode handler (this picks up the task).
 *
 * @param *data - pointer to startup parameters
 * @return shouldn't return as the execve will overlay current
 * execution image.
 * @todo process identity should be set according to the
 * the user configuration (tcmi_confman)
 */
static int tcmi_migcom_migrated_task(void *data)
{
	int err = -EINVAL;
	struct immigrate_startup_params* startup_params = (struct immigrate_startup_params*) data;
	struct completion *guest_ready = &startup_params->guest_ready;
	struct completion *fs_ready = &startup_params->fs_ready;
	struct fs_mounter* mounter;
	struct cred* new_cred;
	
	mdbg(INFO2, "New potential guest thread started. Pid: %d", current->pid);

	new_cred = prepare_creds();
	if ( !new_cred )
		return -ENOMEM;

	/* set process credentials */
	new_cred->uid = new_cred->euid = new_cred->suid = new_cred->fsuid = 
	new_cred->gid = new_cred->egid = new_cred->sgid = new_cred->fsgid = 999;

	commit_creds(new_cred);

	
//	current_uid() = current_euid() = current_suid() = current_fsuid() =
//		current_gid() = current_egid() = current_sgid() =
//		current_fsgid() = 999;
	/* This part simulates when the process didn't pickup
	   the task on time 
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(5*HZ); 
	minfo(INFO1, "Task has woken up %p", current);
	*/

	// TODO: Close ALL fs.. are there some others? ;)
	sys_close(0);
	sys_close(1);
	sys_close(2);

	mdbg(INFO2, "Creating mounter: %s", startup_params->mount_params->mount_type);
	mounter = get_new_mounter(startup_params->mount_params);	
	mdbg(INFO2, "Created mounter: %p", mounter);
	if ( mounter ) {
		/* Some file system mounter is registered => perform the mount */
		err = mounter->private_mount(mounter, startup_params->fsuid, startup_params->fsgid);
		if ( !err )
			mdbg(INFO2, "Private Mount succeeded");
		free_mounter(mounter);
		mdbg(INFO2, "Freed mounter");
		if ( err ) {
			/* We must notify anyway to prevent blocking */
			complete(fs_ready);	
			goto exit0;
		}		
	}
	
	err = mount_proxyfs(startup_params->sock);
	if ( !err )
		err = mount_local_procfs(startup_params->sock);

	if ( err ) {
		// We must notify anyway to prevent blocking
		complete(fs_ready);	
		goto exit0;
	}

	/* Notify about file system mount finished */
	complete(fs_ready);

	mdbg(INFO2, "Starting new thread for migrating task..");
	if ((err = wait_for_completion_interruptible(guest_ready))) {
		minfo(INFO1, "Received signal guest not ready");
		goto exit0;
	}
	mdbg(INFO3, "Guest task is ready by now..");

	/* migration mode handler picks up the task  */
	if (tcmi_taskhelper_run_mig_mode_handler() < 0) {
		mdbg(ERR3, "Failed to run migration mode handler");
		goto exit0;
	}

	return 0;

	/* error handling */
 exit0:

	return err;
}

/** \<\<public\>\> Fork of guest. Called in parent process context */
int tcmi_migcom_guest_fork(struct task_struct* parent, struct task_struct* child, struct tcmi_migman* migman) {
	struct tcmi_task *guest;
	struct kkc_sock* sock;	

	mdbg(INFO3, "Forking guest process => Making a new child guest as well.");
	// TODO: We have to mount proxyfs&clondike fs in the child context.. schedule method to do that here
	if (!(guest = tcmi_guesttask_new(child->pid, migman, 
					tcmi_migman_sock(migman), 
					tcmi_migman_migproc_dir(migman), 
					tcmi_migman_root(migman)))) {
		mdbg(ERR3, "Error creating a guest task on fork");
		goto exit0;
	}

	sock = tcmi_migman_sock(migman); 
	tcmi_task_submit_method(guest, tcmi_task_mount_proxyfs, &sock, sizeof(struct kkc_sock*));	
	// Submit method, that will set tid of task into a user context.. it has to be done from that process context!	
	tcmi_task_submit_method(guest, tcmi_guesttask_post_fork_set_tid, NULL, 0);

	/* attaches the shadow to its thread. */
	if (tcmi_taskhelper_attach_exclusive(child, guest, tcmi_migcom_mig_mode_handler) < 0) {
		mdbg(ERR3, "Failed attaching guest PID=%d to its thread", 
		     tcmi_task_local_pid(guest));
		goto exit1;
	}	

	/* release the the instance reference (Reference is now held by the associated task_struct - it got it in attach method)*/
	tcmi_task_put(guest);

	/* switch the task to migration mode */	
	tcmi_taskhelper_enter_mig_mode_exclusive(child, guest);

	return 0;
exit1:
	tcmi_task_put(guest);
exit0:
	return -EINVAL;
}

/** \<\<public\>\> Fork of shadow */
int tcmi_migcom_shadow_fork(struct task_struct* parent, struct task_struct* child, struct tcmi_migman* migman) {
	struct tcmi_task *shadow;	

	mdbg(INFO3, "Forking shadow process => Making a new child shadow as well.");

	if (!(shadow = 
	      tcmi_shadowtask_new(child->pid, migman, 
				  tcmi_migman_sock(migman), 
				  tcmi_migman_migproc_dir(migman), 
				  tcmi_migman_root(migman)))) {
		minfo(ERR3, "Error creating a shadow task for forked child");
		goto exit0;
	}
	
	// Submit process message so that the task is started in a main waiting loop
	// First expected message to get is either about successful start, or about start failure
	tcmi_task_submit_method(shadow, tcmi_task_process_msg, NULL, 0);

	/* attaches the shadow to its thread. */
	if (tcmi_taskhelper_attach_exclusive(child,shadow, tcmi_migcom_mig_mode_handler) < 0) {
		mdbg(ERR3, "Failed attaching shadow PID=%d to its thread", 
		     tcmi_task_local_pid(shadow));
		goto exit1;
	}

	/* release the the instance reference (Reference is now held by the associated task_struct - it got it in attach method)*/
	tcmi_task_put(shadow);


	/* switch the task to migration mode */	
	tcmi_taskhelper_enter_mig_mode_exclusive(child, shadow);

	return 0;
exit1:
	tcmi_task_put(shadow);
exit0:
	return -EINVAL;
}


/** 
 * \<\<private\>\> This handler is invoked when the sig_unused hook
 * finds a non-NULL entry in the clondike record in the task_struct.
 *
 * What needs to be done:
 * - check whether the process in migration mode has a valid TCMI task
 * - launch the method pump
 * - based on its return status:
 *   -# let's the task return to user mode
 *   -# detaches the TCMI task from the process and resumes user mode
 *   -# detaches the TCMI task from the process and kills the process
 */
static void tcmi_migcom_mig_mode_handler(void)
{
	struct tcmi_task *tmp, *task;
	int res;
	long exit_code;

	mdbg(INFO3, "Migration mode handler for task '%s' stack rp(%p) executing", 
	     current->comm, &res);


	if (!(task = tcmi_taskhelper_sanity_check())) {
		mdbg(ERR2, "Process %p PID=%d - invalid entry to migration mode",
		      current, current->pid);
		goto exit0;
	}

	/* keep moving the task between migration managers */
	while ((res = tcmi_task_process_methods(task)) ==
	       TCMI_TASK_MOVE_ME);
	/* check the result of the last method processed */
	switch (res) {
	case TCMI_TASK_KEEP_PUMPING:
		mdbg(INFO2, "KEEP PUMPING - but no methods left %d", res);
		break;
	case TCMI_TASK_LET_ME_GO:
		mdbg(INFO2, "LET ME GO - request %d", res);
		break;
		/* no need for special handling a fall through causes
		 * do_exit which invokes the migration handler again. */
	case TCMI_TASK_EXECVE_FAILED_KILL_ME:
		mdbg(INFO2, "EXECVE_FAILED KILL ME - request %d", res);
		break;
	case TCMI_TASK_KILL_ME:
		mdbg(INFO2, "KILL ME - request %d", res);
		tmp = tcmi_taskhelper_detach();
		/* get the exit code prior terminating. */
		exit_code = tcmi_task_exit_code(tmp);
		tcmi_task_put(tmp);
		mdbg(INFO2, "Put done, going to exit %ld", exit_code);
		complete_and_exit(NULL, exit_code);
		break;
	case TCMI_TASK_REMOVE_AND_LET_ME_GO:
		mdbg(INFO2, "REMOVE AND LET ME GO - request %d", res);
		tcmi_task_put(tcmi_taskhelper_detach());
		break;
	default:
		mdbg(ERR2, "Unexpected result %d.", res);
		break;
	}
	/* error handling */
 exit0:
	return;
}


/**
 * @}
 */



