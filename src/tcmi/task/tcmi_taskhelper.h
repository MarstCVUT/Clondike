/**
 * @file tcmi_taskhelper.h - TCMI task helper, supports task manipulation
 *                      
 * 
 *
 *
 * Date: 04/28/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_taskhelper.h,v 1.13 2008/01/17 14:36:44 stavam2 Exp $
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
#ifndef _TCMI_TASKHELPER_H
#define _TCMI_TASKHELPER_H

#include <linux/jiffies.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fdtable.h>
#include <linux/sched.h>
#include <asm/current.h>

#include <clondike/tcmi/tcmi_struct.h>

#include <tcmi/ckpt/tcmi_ckptcom.h>
#include <tcmi/ckpt/tcmi_ckpt.h>
#include "tcmi_task.h"
#include <tcmi/lib/util.h>

/** @defgroup tcmi_taskhelper_class tcmi_taskhelper class 
 * 
 * @ingroup tcmi_task_group
 * 
 * This \<\<singleton\>\> class implements platform specific methods
 * that make handling TCMI tasks easier. The functionality includes:
 * - method to switch a task into migration mode
 * - method to submit a specific method to a TCMI task and have it process
 * the method
 * - synchronization with a task entering the migration mode for the
 * - OS specific basic signal handling
 * 
 * @{
 */

/**
 * Runs migration mode handler of the current process.
 *
 * @return 0 upon success
 */
static inline int tcmi_taskhelper_run_mig_mode_handler(void)
{
	if (!current->tcmi.mig_mode_handler) {
		mdbg(ERR3, "Missing migration mode handler for current process!!");
		goto exit0;
	}
	current->tcmi.mig_mode_handler();

	return 0;
	/* error handling */
 exit0:
	return -EINVAL;
}


/** 
 * \<\<public\>\> Switches the task into migration mode.  It is
 * necessary to lookup the regular kernel process that the TCMI task
 * is attached to and enforce its migration mode.  Essentially, it
 * sends SIG_UNUSED signal to the process. The signal handler hook(see
 * tcmi_taskhelper_do_mig_mode()) takes care of running the migration
 * mode handler.
 *
 * @param *t_task - pointer to the TCMI task that is to be preempted into migration mode
 */
static inline void tcmi_taskhelper_enter_mig_mode(struct tcmi_task *t_task)
{
	struct task_struct* l_task = tcmi_task_to_task_struct(t_task);
	
	if ( !l_task ) {
 		minfo(ERR3, "Can't enter to mig_mode. Cannot find corresponding task_struct for PID: %d", 
		     tcmi_task_local_pid(t_task));		
		return;
	}
	
	/* switch the task to migration mode */
	force_sig(SIGUNUSED, l_task);
}
 

/** 
 * \<\<public\>\> Same as standard enter_mig_mode, but in this case we have already a reference to current task, so we do not
 * perform any locking
 */
static inline void tcmi_taskhelper_enter_mig_mode_exclusive(struct task_struct* l_task, struct tcmi_task *t_task)
{
	/* switch the task to migration mode */
	force_sig(SIGUNUSED, l_task);

	return;
}


/**
 * \<\<public\>\> Finds a TCMI task based on PID, in addition checks
 * whether the task has a valid migration mode handler assigned.
 *
 * @param pid - PID of a taks to be looked up
 * @return TCMI task or NULL
 */

static inline struct tcmi_task* tcmi_taskhelper_find_by_pid(pid_t pid)
{
	struct task_struct *l_task;
	struct tcmi_task *t_task = NULL;
	

	/* try finding the process by PID */
	read_lock_irq(&tasklist_lock);
	if (!(l_task = task_find_by_pid(pid))) {
		mdbg(ERR3, "Can't find, no such process %d", pid);
		goto exit0;
	}
	task_lock(l_task);
	/* check whether the process has a TCMI task and a valid handler */
	if (!(l_task->tcmi.mig_mode_handler && l_task->tcmi.tcmi_task)) {
		mdbg(ERR3, "Process %d has no a mig. mode handler"
		     "or TCMI task", pid);
		task_unlock(l_task);
		goto exit0;
	}
	/* get the extra task reference */
	t_task = tcmi_task_get(l_task->tcmi.tcmi_task);
	task_unlock(l_task);
	read_unlock_irq(&tasklist_lock);

	return t_task;
	/* error handling */
 exit0:
	read_unlock_irq(&tasklist_lock);
	return NULL;
}

/** 
 * \<\<public\>\> Submits a specified method to a TCMI task identified by PID.
 * To accomplish the job in SMP safe manner we need to:
 * - get a valid reference to a TCMI task associated with particular PID
 * - submit the method - based on priority we can also choose to flush
 * the method queue
 * - switch the process to migration mode - again, to be safe, we
 * use the tcmi_taskhelper_enter_mig_mode()
 * - drop the task reference
 *
 * @param pid - process that is to be notified
 * @param *method - task method that is to be submitted
 * @param *data - argument for the submitted method
 * @param size - size of the argument in bytes
 * @param priority - when > 0, the method queue is also flushed
 * @return 0 upon success
 *
 * @todo Simplify the way the task is forced into migration mode, so
 * that a full lookup doesn't need to be performed again via
 * tcmi_taskhelper_enter_mig_mode()
 */
static inline int tcmi_taskhelper_notify_by_pid(pid_t pid, tcmi_method_t *method, 
						void *data, int size, int priority)
{
	
	struct tcmi_task *t_task = NULL;


	if (!(t_task = tcmi_taskhelper_find_by_pid(pid))) {
		mdbg(ERR3, "Can't find TCMI task PID %d", pid);
		goto exit0;
	}

	/* now we can safely operate on the task as we own 1 reference */
	if (priority > 0)
		tcmi_task_flush_and_submit_method(t_task, method, data, size);
	else
		tcmi_task_submit_method(t_task, method, data, size);

	/* using method is safe as we have no reference l_task */ 
	tcmi_taskhelper_enter_mig_mode(t_task);
	tcmi_task_put(t_task);

	return 0;

	/* error handling */
 exit0:
	return -EINVAL;
}

/** 
 * \<\<public\>\> Synchronizes with a task entering the migration mode
 * for the first time.  The thread associated with the task already has an
 * extra reference.  If the task doesn't get picked up on time, it
 * will be safely removed from under the thread.
 *
 * We don't need an extra reference to the kernel task struct as we
 * are holding task_list_lock -> the task struct can't be removed from
 * under us.
 * 
 * @param *t_task - pointer to the task instance that we are syncing with 
 * @param timeout - in ticks
 * @return 0 if the task has successfully entered the migration mode
 */
static inline int tcmi_taskhelper_wait_for_pick_up_timeout(struct tcmi_task *t_task,
							   unsigned long timeout)
{
	int err = 0;
	/* temporary storage for the task. */
	struct tcmi_task *tmp = NULL;
	/* regular linux task used for lookup*/
	struct task_struct *l_task;
	/* Wait for pickup first */
	if (!tcmi_task_wait_for_pick_up_timeout(t_task, timeout)) {
		/* try finding thread associated with the task by PID */
		read_lock_irq(&tasklist_lock);
		if (!(l_task = task_find_by_pid(tcmi_task_local_pid(t_task)))) {
			mdbg(ERR3, "Failed to wait - no such process %d", 
			     tcmi_task_local_pid(t_task));
			err = -ESRCH;
			read_unlock_irq(&tasklist_lock);
			goto exit0;
		}
		/* A bad inconsistency if tasks don't match - bug */
		BUG_ON(t_task != l_task->tcmi.tcmi_task);
		/* the task might still have been picked up in the
		 * meantime, we have to check again */
		task_lock(l_task);
		if (!tcmi_task_picked_up(l_task->tcmi.tcmi_task)) {
			minfo(INFO1, "Task local PID = %d still not picked up destroying..",
			      tcmi_task_local_pid(t_task));
			tmp = l_task->tcmi.tcmi_task;
			l_task->tcmi.mig_mode_handler = NULL;
			l_task->tcmi.tcmi_task = NULL;
			err = -EINVAL;
		}
		task_unlock(l_task);
		read_unlock_irq(&tasklist_lock);
		/* Release the task instance - safe if still NULL. We must release 1 reference as the thread that owned the task no longer owns it. */
		tcmi_task_put(tmp);
	}
	/* error handling */
 exit0:
	return err;
}

/**
 * \<\<public\>\> Checks if there are any pending signals and handles
 * them.  For each pending signal, the task is requested to process it
 * as long as the signal is not SIGUNUSED - which indicates an attempt
 * to switch to migration mode, eventhough we are already in migration
 * mode. This signal is ignored.
 *
 * We keep processing the signals as long as there are any pending left
 * and the task method returns TCMI_TASK_KEEP_PUMPING.
 *
 * The result of the last call of the task method is communicated back.
 *
 * @param *t_task - pointer to this task instance 
 * @return result of the task specific signal handling method or
 * TCMI_TASK_KEEP_PUMPING
 */
static inline int tcmi_taskhelper_do_signals(struct tcmi_task *t_task)
{
	int res = TCMI_TASK_KEEP_PUMPING;
	siginfo_t info;
	unsigned long signr = 0;
	sigset_t *mask = &current->blocked;

	mdbg(INFO2, "Do signals being processed.");

	/* check pending signal */
	if (signal_pending(current)) {
		mdbg(INFO2, "Has signal pending");
		/* for all signals */
		while (res == TCMI_TASK_KEEP_PUMPING) {
			/* dequeue signal */
			spin_lock_irq(&current->sighand->siglock);
			signr = dequeue_signal(current, mask, &info);
			spin_unlock_irq(&current->sighand->siglock);
			/* no more signals in queue */
			if (!signr)
				break;
			mdbg(INFO2, "Dequeued signal number: %lu..", signr);
			if (signr != SIGUNUSED)
				res = tcmi_task_do_signal(t_task, signr, &info);
		}
	}

	mdbg(INFO2, "Signal handling finished. Result: %d", res);

	return res;
}

/**
 * \<\<public\>\> Flushes all pending signals of the task. Usually
 * needed right after establishing a new shadow process.
 *
 * @param *t_task - pointer to this task instance 
 */
static inline void tcmi_taskhelper_flush_signals(struct tcmi_task *t_task)
{
	flush_signals(current);
}

/** 
 * \<\<public\>\> Attaches the TCMI task to its kernel task.
 * This requires:
 * - looking up the target thread (denoted by task's local PID)
 * - checking if there has been assigned no handler/task assigned yet
 * - storing the specified handler+task inside the kernel task(process)
 *
 * The target kernel task will have an extra reference to the task.
 *
 * @param *t_task - pointer to the TCMI task that is to be attached
 * @param *mig_mode_handler - migration mode handler specific to the
 * task.
 * @return 0 upon success.
 */
static inline int tcmi_taskhelper_attach(struct tcmi_task *t_task, 
					 mig_mode_handler_t *mig_mode_handler)
{
	int err = -EINVAL;
	struct task_struct *l_task;

	/* try finding the target process for our TCMI task by PID */
	read_lock_irq(&tasklist_lock);
	if (!(l_task = task_find_by_pid(tcmi_task_local_pid(t_task)))) {
		mdbg(ERR3, "No such process %d", tcmi_task_local_pid(t_task));
		err = -ESRCH;
		goto exit0;
	}
	/* synchronize with the migration mode handler that might run in paralle */
	task_lock(l_task);
	if (l_task->tcmi.mig_mode_handler || l_task->tcmi.tcmi_task) {
		mdbg(ERR3, "Kernel task %d already has a mig. mode handler - %p "
		     "or TCMI task - %p assigned, bailing out",
		      tcmi_task_local_pid(t_task), l_task->tcmi.mig_mode_handler,
		     l_task->tcmi.tcmi_task);
		task_unlock(l_task);
		goto exit0;
	}
	l_task->tcmi.mig_mode_handler = mig_mode_handler;
	/* extra reference on TCMI task for the target process/its
	 * migration component resp. */
	l_task->tcmi.tcmi_task = tcmi_task_get(t_task); 
	l_task->tcmi.task_type = tcmi_task_get_type(t_task);
	task_unlock(l_task);

	read_unlock_irq(&tasklist_lock);

	return 0;

	/* error handling */
 exit0:
	read_unlock_irq(&tasklist_lock);
	return err;
}


/** 
 * \<\<public\>\> Same as standard attach, but here we have a reference to a real task and we are
 * exclusive owner of this reference and the task is not yet running => no locking required.
 * 
 *
 * @param *l_task - pointer to a kernel task struct, to which we are attaching
 * @param *t_task - pointer to the TCMI task that is to be attached
 * @param *mig_mode_handler - migration mode handler specific to the
 * task.
 * @return 0 upon success.
 */
static inline int tcmi_taskhelper_attach_exclusive(struct task_struct* l_task, struct tcmi_task *t_task, 
					 mig_mode_handler_t *mig_mode_handler)
{
	int err = -EINVAL;

	if (l_task->tcmi.mig_mode_handler || l_task->tcmi.tcmi_task) {
		mdbg(ERR3, "Kernel task %d already has a mig. mode handler - %p "
		     "or TCMI task - %p assigned, bailing out",
		      tcmi_task_local_pid(t_task), l_task->tcmi.mig_mode_handler,
		     l_task->tcmi.tcmi_task);
		goto exit0;
	}
	l_task->tcmi.mig_mode_handler = mig_mode_handler;
	/* extra reference on TCMI task for the target process/its
	 * migration component resp. */
	l_task->tcmi.tcmi_task = tcmi_task_get(t_task); 
	l_task->tcmi.task_type = tcmi_task_get_type(t_task);

	return 0;

	/* error handling */
 exit0:
	return err;
}


/** 
 * \<\<private\>\> Dettaches the TCMI task from a specified process -
 * lock free version. This method is intended for internal use only.
 * It resets all TCMI related data in the specified process and
 * returns the TCMI task pointer. It is user responsibility
 * to ensure proper locking.
 *
 * @return TCMI task that has been detached.
 */
static inline struct tcmi_task* __tcmi_taskhelper_detach(struct task_struct *l_task)
{
	struct tcmi_task *tmp;

	tmp = l_task->tcmi.tcmi_task;
	l_task->tcmi.tcmi_task = NULL;
	l_task->tcmi.mig_mode_handler = NULL;
	if ( l_task->tcmi.task_type == shadow ) 
		l_task->tcmi.task_type = shadow_detached;

	return tmp;
}

/** 
 * \<\<public\>\> Dettaches the TCMI task from its process The user
 * specifies only kernel task, that is to be handled.  This method is
 * typically used from migration mode handler -
 * 
 *
 * Dettaching the task includes also resetting the migration mode
 * handler to NULL. This work is delegated to
 * __tcmi_taskhelper_detach().
 *
 * @return TCMI task that has been detached.
 */
static inline struct tcmi_task* tcmi_taskhelper_detach(void)
{
	struct tcmi_task *tmp;

	read_lock_irq(&tasklist_lock);
	task_lock(current);

	
	tmp = __tcmi_taskhelper_detach(current);

	task_unlock(current);
	read_unlock_irq(&tasklist_lock);

	return tmp;
}

/** 
 * \<\<public\>\> Dettaches the TCMI task from its process.  The
 * process is looked up based on PID and the task is removed from the
 * task_struct.
 *
 * @param pid - PID of the process whose TCMI task is to be detached
 * @return TCMI task that has been detached.
 */
static inline struct tcmi_task* tcmi_taskhelper_detach_by_pid(pid_t pid)
{
	struct task_struct *l_task;
	struct tcmi_task *t_task;

	/* try finding the process by PID */
	read_lock_irq(&tasklist_lock);
	if (!(l_task = task_find_by_pid(pid))) {
		mdbg(ERR3, "Can't notify, no such process %d", pid);
		goto exit0;
	}
	task_lock(l_task);
	t_task = __tcmi_taskhelper_detach(l_task);
	task_unlock(l_task);

	read_unlock_irq(&tasklist_lock);

	return t_task;

	/* error handling */
 exit0:
	read_unlock_irq(&tasklist_lock);
	return NULL;
}


/** 
 * \<\<public\>\> This method is intended for migration mode handler,
 * it should be the very first check it performs. We verify that the
 * current process has a valid TCMI task assigned. Also, if this is
 * the first time entering the migration mode, the task has to be
 * 'picked up'. This is a synchronization mechanism that is available
 * for the TCMI task instantiator (usually the migration component).
 *
 * @return a valid TCMI task that is attached to the current kernel
 * process or NULL;
 */
static inline struct tcmi_task* tcmi_taskhelper_sanity_check(void)
{
	struct tcmi_task *t_task;

	task_lock(current);
	/* safes a few dereferences */
	t_task = current->tcmi.tcmi_task;
	/* check that the TCMI task exists */
	if (!t_task) {
		mdbg(ERR3, "Kernel task '%s' PID=%d has no TCMI task assigned", 
		     current->comm, current->pid);
		goto exit0;
	}
	/* pickup the task when entering the migration mode for the first time */
	if (!tcmi_task_picked_up(t_task)) {
		mdbg(INFO2, "TCMI task %p PID=%d first time in mig. mode, picking up", 
		     t_task, tcmi_task_local_pid(t_task));
		tcmi_task_pick_up(t_task);
	}
	task_unlock(current);

	return t_task;
	/* error handling */
 exit0:
	task_unlock(current);
	return NULL;
}

/**
 * \<\<public\>\> This method is registered by CCN or PEN component as
 * the SIGUNUSED signal handler. It is responsible for storing the
 * current process user mode context into the shadow/guest task resp.
 *
 * @param *regs - the current process context (registers)
 */
static inline void tcmi_taskhelper_do_mig_mode(struct pt_regs *regs)
{
	if (current->tcmi.mig_mode_handler && current->tcmi.tcmi_task) {
		mdbg(INFO3, "Migration mode active for %p '%s'", current, current->comm);
		tcmi_task_set_context(current->tcmi.tcmi_task, regs);
		current->tcmi.mig_mode_handler();
	}
}




/**
 * \<\<public\>\> current process notification - submission of a method.
 * The notification requires:
 * - checks for a valid TCMI task in current process
 * - submits the method based on priority
 * - launches the migration mode handler - we need to reassign
 * the process context as this notification is typically used
 * from system call hooks.
 *
 * @param *method - task method that is to be submitted
 * @param *data - argument for the submitted method
 * @param size - size of the argument in bytes
 * @param priority - when > 0, the method queue is also flushed
 * @return 0 upon success
 */
static inline int tcmi_taskhelper_notify_current(tcmi_method_t *method, 
						 void *data, int size, int priority)
{
	struct tcmi_task *t_task;

	mdbg(INFO3, "Notify current %p", current);
	if (!(t_task = current->tcmi.tcmi_task)) {
		goto exit0;
	}

	/* now we can safely operate on the task as we own 1 reference */
	if (priority > 0)
		tcmi_task_flush_and_submit_method(t_task, method, data, size);
	else
		tcmi_task_submit_method(t_task, method, data, size);

	tcmi_taskhelper_do_mig_mode(tcmi_task_context(t_task));

	return 0;

	/* error handling */
 exit0:
	return -EINVAL;
}

/** 
 * \<\<public\>\> Creates a new checkpoint of the process.  It might
 * be a good idea to delete the old checkpoint too. This method has to
 * be called from the context of the process that is to be
 * checkpointed.
 *
 * - The checkpoint name is composed of the process name stored in
 * current->comm pid and current value of jiffies.  Once the
 * checkpoint name is complete, it is stored in the task instance(so
 * that it can be accessed later on).
 *
 * - a new checkpoint file is open(created)
 *
 * - file object along with task context is passed onto the
 * checkpointing component.
 *
 * 
 * @param *t_task - pointer to the TCMI task that is to be checkpointed.
 * @params *npm_params - Pointer to non-preemtive migration params (or null in case of preemptive migration)
 * @return 0 upon success
 */
static inline int tcmi_taskhelper_checkpoint(struct tcmi_task *t_task, struct tcmi_npm_params* npm_params)
{
	/* page for the filepathname */
	unsigned long page;
	char *pathname = NULL;
	struct file *file;
	char *argv[] = { NULL };
	char *envp[] = { NULL };

	/* resolve the path name. */
	if (!(page = __get_free_page(GFP_ATOMIC))) {
		mdbg(ERR3, "Can't allocate page for file pathname!");
		goto exit0;
	}
	pathname = (char*) page;
	snprintf(pathname, PAGE_SIZE, "/home/clondike/%s.%d.%lu", current->comm, 
		 current->pid, jiffies);
	mdbg(INFO3, "Composed checkpoint name: '%s' Npm params: %p", pathname, npm_params);
		
	/* setup execve arguments - this is just to store checkpoint name in the instance */
	if (tcmi_task_prepare_execve(t_task, pathname, argv, envp) < 0) {
		mdbg(ERR3, "Failed preparing execve of the checkpoint '%s'", pathname);
		goto exit1;
	}

	/* Ask the checkpointing component for a checkpoint */
	file = filp_open(pathname, 
			 O_CREAT | O_RDWR  | O_NOFOLLOW | O_LARGEFILE | O_TRUNC, 0700);
	if (IS_ERR(file)) {
		mdbg(ERR3, "Failed to create a checkpoint file for pid %d. Ckptname = '%s'. Error: %ld", current->pid,
		     pathname, PTR_ERR(file));
		goto exit1;
	}

	/* create the checkpoint, task context contains pointer to process registers */
	if ( npm_params ) {
		// Non-preemptive checkpoint
		if (tcmi_ckptcom_checkpoint_npm(file, tcmi_task_context(t_task), npm_params) < 0) {
			mdbg(ERR3, "Failed npm checkpointing process PID %d ckptname = '%s'", current->pid,
			pathname);
			goto exit2;
		}
	} else {
		// Preemptive checkpoint
		if (tcmi_ckptcom_checkpoint_ppm(file, tcmi_task_context(t_task), 1) < 0) {
			mdbg(ERR3, "Failed ppm checkpointing process PID %d ckptname = '%s'", current->pid,
			pathname);
			goto exit2;
		}
	}

	filp_close(file, NULL);	

	free_page(page);
	return 0;

	/* error handling */
 exit2:
	filp_close(file, NULL);	
 exit1:
	free_page(page);
 exit0:
	return -EINVAL;
}

/** 
 * \<\<public\>\> Schedules a task restart from a given checkpoint
 * image. This requires:
 *
 * - preparing execve of the checkpoint (the task stores internally
 * the execve name string, so that it can be released upon task destruction)
 * - submitting execve method - it will be picked up by the method pump
 * in its next iteration. We WANT to flush all methods from the queue as
 * execve we is of high importance and want to make sure it runs next.
 *
 * @param *t_task - pointer to the TCMI task whose image will be overlayed
 * with the checkpoint image
 * @param ckpt_name - checkpoint image name.
 * @return 0 upon success
 */
static inline int tcmi_taskhelper_restart(struct tcmi_task *t_task, char *ckpt_name)
{
	char *argv[] = { NULL };
	char *envp[] = { NULL };
	/* setup execve arguments*/
	if (tcmi_task_prepare_execve(t_task, ckpt_name, argv, envp) < 0) { 
		mdbg(ERR3, "Failed preparing execve of the checkpoint '%s'", ckpt_name);
		goto exit0;
	}
	/* schedule task execve as the next method - requires flushing
	 * all methods in the queue */
	mdbg(INFO3, "Submitting execve");
	tcmi_task_flush_and_submit_method(t_task, tcmi_task_execve, NULL, 0);
	return 0;

	/* error handling */
 exit0:
	return -EINVAL;
}

/**
 * \<\<public\>\> Flushing files is required prior to issuing a
 * migrate request. It is necessary to close all regular files that
 * the process has open as those files will be accessed again by the
 * restarted checkpoint. This prevents write conflicts.
 *
 *
 */
static inline void tcmi_taskhelper_flushfiles(void)
{
	int fd;
	struct file *file;
	struct inode *inode;
	unsigned int old_flags = current->flags & PF_EXITING;
	
	/* We do a special trick here. In order to preserve proxy files during the migration, we need to tell the other end not to close the files.
	   It does not close the files in case the clsoe was called in context of exit, as this file is going to be closed on process exit.
	   So we pretend we are exitting here so that the server things it should not close the file */ 
	current->flags |= PF_EXITING;
	
	/* Check each open file */
	tcmi_ckpt_foreach_openfile(file, fd) {
		inode = file->f_dentry->d_inode;
		if (S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) || S_ISCHR(inode->i_mode) || S_ISFIFO(inode->i_mode) ) {
			mdbg(INFO4, "Closing file fd=%d, type=%0o '%s'", fd, file->f_flags, 
			     file->f_dentry->d_name.name);
			sys_close(fd);
		}
	}
		
	current->flags = old_flags;
}

/** \<\<public\>\> Set action for all signals accept SIGKILL and SIGSTOP 
 * @param *action - struct k_sigaction, which will be set to all signals
 */
static inline void tcmi_taskhelper_catch_all_signals(struct k_sigaction *action)
{
	int i;
	for( i = 1; i < _NSIG; i++ ){
		if( i != SIGKILL && i != SIGSTOP )
			do_sigaction( i, action, NULL );
	}

}
/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_TASKHELPER_PRIVATE


#endif /* TCMI_TASKHELPER_PRIVATE */


/**
 * @}
 */

#endif /* _TCMI_TASKHELPER_H */

