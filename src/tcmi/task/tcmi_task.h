/**
 * @file tcmi_task.h - common functionality for TCMI shadow and guest tasks.
 *                      
 * 
 *
 *
 * Date: 04/25/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_task.h,v 1.10 2009-01-20 14:23:10 andrep1 Exp $
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
#ifndef _TCMI_TASK_H
#define _TCMI_TASK_H

#include <asm/atomic.h>
#include <linux/types.h>
#include <linux/completion.h>
#include <clondike/tcmi/tcmi_struct.h>

#include "tcmi_method_wrapper.h"

#include <tcmi/ctlfs/tcmi_ctlfs.h>
#include <tcmi/ctlfs/tcmi_ctlfs_dir.h>
#include <tcmi/ctlfs/tcmi_ctlfs_file.h>
#include <tcmi/ctlfs/tcmi_ctlfs_symlink.h>

#include <kkc/kkc.h>
#include <tcmi/lib/tcmi_queue.h>
#include <tcmi/lib/tcmi_slotvec.h>

#include <tcmi/comm/tcmi_msg.h>

#include <arch/current/make_syscall.h>

#include <tcmi/lib/util.h>
#include <linux/sched.h>

struct tcmi_migman;
struct tcmi_npm_params;

/** @defgroup tcmi_task_class tcmi_task class 
 * 
 * @ingroup tcmi_task_group
 *
 * TCMI task is an abstraction for any process in the system that
 * participates in migration. This \<\<template\>\> class gathers
 * common functionality for shadow tasks that represent a migrated
 * process on CCN as well as for guest tasks that represent the
 * migrated process on PEN.
 *
 * TCMI tasks are controlled from user space via migration component.
 * Each task is associated with its migration manager. This ensures
 * that all messages addressed to a particular task are also delivered
 * to its message queue or transactions slotvector.  A TCMI task is
 * given a communication socket (KKC socket) upon its
 * instantiation. It is the communication channel that it will use for
 * sending messages out.  Note, that message delivery is handled
 * exclusively by the migration manager. This makes it very easy to
 * move a task among TCMI migration managers.
 *
 * The main purpose of this class is execution of methods in the context
 * of the shadow and guest process resp. These methods are submitted by the
 * migration component into the method queue and include:
 * - request to migrate a process to a PEN - includes PPM_P, PPM_V, NPM
 * - request to migrate a process back to CCN - includes PPM_P, PPM_V, NPM
 * - request to process any incoming TCMI messages from the message queue.
 * - request to terminate
 *
 * Methods in the queue are processed via a 'method pump'. Each method
 * returns a status that is used by the controlling component to make
 * a decision what to do further. Status return codes have following
 * meanings:
 *
 * - KEEP_PUMPING - the method pump should keep going as long as there
 * are methods to process
 *
 * - MOVE_ME - stop pumping and tell the task controlling component to
 * move it to a different migration manager (relevant for shadow tasks
 * on CCN)
 *
 * - EXECVE_FAILED_KILL_ME - stop pumping and tell the task
 * controlling component to kill this task. This status denotes the
 * situation when the task tried issuing execve(typically when loading
 * a new checkpoint or processing a non-preemptive migration request)
 * and it failed. This status is needed as it also instructs the method
 * pump not to discard the method wrapper as it has already been done
 * by (tcmi_task_execve() method internally)
 * 
 * - KILL_ME - stop pumping and tell the task controlling component to
 * kill this task (issue do_exit()) and remove it from its migration
 * manager
 *
 * - LET_ME_GO -stop pumping and tell the task controlling component
 * leave the migration mode, leave the task as is.
 *
 * - REMOVE_AND_LET_ME_GO -stop pumping and tell the task controlling
 * component to remove the task from its current migration manager,
 * destroy the tcmi_task and and let the regular task go. For example,
 * this status is returned by task_exit() method as it is called from
 * the intercepted exit system call. Once the TCMI task is detached
 * from its process the exit system call resumes and performs the
 * actual exit.
 *
 * In addition, the task also holds the filename along with argv and
 * envp arrays that will be executed. It is important to keep track of
 * this data as they are required by the execve (e.g. when migrating a
 * a task by executing it's checkpoint image). These resources
 * eventually need to be released once the execve has been issued or
 * when the task is destroyed. There is functionality that makes arguments
 * and environment management easy.
 *
 * Abbreviations:
 * - PPM_P - preemptive process migration using physical checkpoint image
 * - PPM_V - preemptive process migration using virtual checkpoint image
 * - NPM - non-preemptive process migration (using execve hook)
 * 
 * @{
 */

/** Compound structure that gathers necessary process information. */
struct tcmi_task {
	/** node for storage in the migration slot of the migration manager */
	tcmi_slot_node_t node;

	/** local PID on PEN, CCN resp. */
	pid_t local_pid;
	/** remote PID on PEN, CCN resp. */
	pid_t remote_pid;

	/* Identifier of a manager that this task belongs to.
	u_int32_t migman_id;
 	*/
	/** 
	 * Reference to a migration manager to which this task belongs to.
	 * TODO: This "back" reference perhaps couples the class to tighly to migman class. We in fact need just a slotvector of migman tasks so that this task can handle its addition/removal on creation/deletion.
	 */
	struct tcmi_migman* migman;

	/** communication socket. */
	struct kkc_sock *sock;

	/** queue of methods to be executed. */
	struct tcmi_queue method_queue;

	/** Message queue for transaction-less messages. */
	struct tcmi_queue msg_queue;
	/** Message transactions. */
	struct tcmi_slotvec *transactions;

	/** TCMI ctlfs - migproc directory entry. */
	struct tcmi_ctlfs_entry *d_task;
	/** TCMI ctlfs - symlink to the migration manager directory. */
	struct tcmi_ctlfs_entry *s_migman;
	/** TCMI ctlfs - contains remote PID. */
	struct tcmi_ctlfs_entry *f_remote_pid;

	/** */
	struct tcmi_ctlfs_entry *f_remote_pid_rev;

	/** */
	struct tcmi_ctlfs_entry *d_migman_root;

	/** Task exit code - used when terminating the task. */
	long exit_code;

	/** Instance reference counter. */
	atomic_t ref_count;

	/** Pointer to the context of a process when switched into user mode. */
	void *context;

	/** contains the pathname of the latest checkpoint file */
	char *ckpt_pathname;

	/** command line arguments for the checkpoint file (NULL terminated array of strings) */
	char **argv;
	/** environment (NULL terminated array of strings) */
	char **envp;
	/** How many time was execve called on this task, since it became controlled by the tcmi */
	int execve_count;

	/** Data for proxyfs filesystem */
	void *proxyfs_data;

	/** Used for signalling when the task is picked up by the associated process */
	struct completion picked_up;
	/** This flag is set when the task has been picked up,
	 * indicates the completion is finished. It is used when
	 * performing synchronized waiting on pickup with timeout.
	 */
	int picked_up_flag;
	
	/** Flag used to inform the task that its peer is lost and it should terminate */
	volatile int peer_lost;

	/** Operations specific to shadow/guest tasks resp. */
	struct tcmi_task_ops *ops;
};

/** TCMI task operations that support polymorphism */
struct tcmi_task_ops {
	/** Processes a message that has arrived on the message queue. */
	int (*process_msg)(struct tcmi_task*, struct tcmi_msg*);
	/** Emigrates a task to a PEN - PPM w/ physical checkpoint. */
	int (*emigrate_ppm_p)(struct tcmi_task*);
	/** Migrates a task back to CCN - PPM w/ physical checkpoint. */
	int (*migrateback_ppm_p)(struct tcmi_task*);
	/** Emigrates a task to a PEN - PPM w/ virtual checkpoint. */
	int (*emigrate_ppm_v)(struct tcmi_task*);
	/** Migrates a task back to CCN - PPM w/ virtual checkpoint. */
	int (*migrateback_ppm_v)(struct tcmi_task*);
	/** Emigrates a task to a PEN - NPM. */
	int (*emigrate_npm)(struct tcmi_task*, struct tcmi_npm_params*);
	/** Migrates a task back to CCN - NPM. */
	int (*migrateback_npm)(struct tcmi_task*, struct tcmi_npm_params*);
	/** Used upon task exit. */
	int (*exit)(struct tcmi_task*, long);
	/** Instance specific execve. */
	int (*execve)(struct tcmi_task*);
	/** Handles a specific signal. */
	int (*do_signal)(struct tcmi_task *self, unsigned long signr, siginfo_t *info);
	/** Releases the instance specific resources. */
	void (*free)(struct tcmi_task*);
	/** Return type of task */
	enum tcmi_task_type (*get_type)(void);
};

/** Describes the status returned by task methods that are executed
 * from the method queue. See the \link tcmi_task_class task \endlink
 * detailed description for explanation of each individual return
 * status.
 */
typedef enum {
	TCMI_TASK_KEEP_PUMPING,
	TCMI_TASK_MOVE_ME,
	TCMI_TASK_EXECVE_FAILED_KILL_ME,
	TCMI_TASK_KILL_ME,
	TCMI_TASK_LET_ME_GO,
	TCMI_TASK_REMOVE_AND_LET_ME_GO
} tcmi_task_method_status_t;

/** Casts to the task instance. */
#define TCMI_TASK(t) ((struct tcmi_task*)t)

/** Initializes the task. */
extern int tcmi_task_init(struct tcmi_task *self, pid_t local_pid, struct tcmi_migman* migman, 
			  struct kkc_sock *sock, 
			  struct tcmi_ctlfs_entry *d_migproc, struct tcmi_ctlfs_entry *d_migman,
			  struct tcmi_task_ops *ops);

/** 
 * \<\<public\>\> Instance accessor, increments the reference counter.
 *
 * @param *self - pointer to this task instance
 * @return tcmi_task instance
 */
static inline struct tcmi_task* tcmi_task_get(struct tcmi_task *self)
{
	if (self) {
		atomic_inc(&self->ref_count);
	}
	return self;
}

/** \<\<public\>\> Releases the instance. */
extern void tcmi_task_put(struct tcmi_task *self);

/** \<\<public\>\> Submits a method for execution. */
extern void tcmi_task_submit_method(struct tcmi_task *self, tcmi_method_t *method, 
				    void *data, int size);

/** \<\<public\>\> Prior to submitting, atomically flushes the method queue. */
extern void tcmi_task_flush_and_submit_method(struct tcmi_task *self, tcmi_method_t *method, 
					      void *data, int size);

/** \<\<public\>\> Processes all methods in the queue. */
extern int tcmi_task_process_methods(struct tcmi_task *self);

/** \<\<public\>\> Processes message in the message queue. */
extern int tcmi_task_process_msg(void *self, struct tcmi_method_wrapper *wr);

/** \<\<public\>\> Sends a message to peer. */
extern int tcmi_task_send_message(void *self, struct tcmi_method_wrapper *wr);

/** \<\<public\>\> Emigrates a task to a PEN - PPM w/ physical checkpoint. */
extern int tcmi_task_emigrate_ppm_p(void *self, struct tcmi_method_wrapper *wr);

/** \<\<public\>\> Migrates a task back to CCN - PPM w/ physical checkpoint. */
extern int tcmi_task_migrateback_ppm_p(void *self, struct tcmi_method_wrapper *wr);


/** \<\<public\>\> Emigrates a task to a PEN - PPM w/ virtual checkpoint. */
extern int tcmi_task_emigrate_ppm_v(void *self, struct tcmi_method_wrapper *wr);

/** \<\<public\>\> Migrates a task back to CCN - PPM w/ virtual checkpoint. */
extern int tcmi_task_migrateback_ppm_v(void *self, struct tcmi_method_wrapper *wr);


/** \<\<public\>\> Emigrates a task to a PEN - NPM. */
extern int tcmi_task_emigrate_npm(void *self, struct tcmi_method_wrapper *wr);

/** \<\<public\>\> Migrates a task back to CCN - NPM. */
extern int tcmi_task_migrateback_npm(void *self, struct tcmi_method_wrapper *wr);

/** \<\<public\>\> Exits a task. */
extern int tcmi_task_exit(void *self, struct tcmi_method_wrapper *wr);

/** \<\<public\>\> Prepares execution of a file given the arrays of arguments and
 * environment.*/
extern int tcmi_task_prepare_execve(struct tcmi_task *self, char *file, 
				    char **argv, char **envp);

/** \<\<public\>\> Executes the file that has been previously setup for execution. */
extern int tcmi_task_execve(void *self, struct tcmi_method_wrapper *wr);

/** \<\<public\>\> Delivers a specified message. */
extern int tcmi_task_deliver_msg(struct tcmi_task *self, struct tcmi_msg *m);

/**
 * \<\<public\>\> Transactions accessor - needed when tasks construct
 * their own messages.
 * 
 * @param *self - pointer to this task instance
 * @return transactions slot vector.
 */
static inline struct tcmi_slotvec* tcmi_task_transactions(struct tcmi_task *self)
{
	return self->transactions;
}


/**
 * \<\<public\>\> Exit code accessor.
 * 
 * @param *self - pointer to this task instance
 * @return task exit code.
 */
static inline long tcmi_task_exit_code(struct tcmi_task *self)
{
	return self->exit_code;
}

/**
 * \<\<public\>\> Exit code setter.
 * 
 * @param *self - pointer to this task instance
 * @param code - exit code that is to be set for the task
 */
static inline void tcmi_task_set_exit_code(struct tcmi_task *self, long code)
{
	self->exit_code = code;
}

/**
 * \<\<public\>\> Process context accessor.
 * 
 * @param *self - pointer to this task instance
 * @return process context.
 */
static inline void* tcmi_task_context(struct tcmi_task *self)
{
	return self->context;
}

/**
 * \<\<public\>\> Process context setter.
 * 
 * @param *self - pointer to this task instance
 * @param *context - pointer to the context stored in the task
 */
static inline void tcmi_task_set_context(struct tcmi_task *self, void *context)
{
	self->context = context;
}

/**
 * \<\<public\>\> Task checkpoint pathname accessor.
 * 
 * @param *self - pointer to this task instance
 * @returns the most recent checkpoint pathname.
 */
static inline char* tcmi_task_ckpt_name(struct tcmi_task *self)
{
	return self->ckpt_pathname;
}

/**
 * \<\<public\>\> Local PID accessor.
 * 
 * @param *self - pointer to this task instance
 * @return local PID of the task
 */
static inline pid_t tcmi_task_local_pid(struct tcmi_task *self)
{
	return self->local_pid;
}

/**
 * \<\<public\>\> Remote PID accessor.
 * 
 * @param *self - pointer to this task instance
 * @return remote PID of the task
 */
static inline pid_t tcmi_task_remote_pid(struct tcmi_task *self)
{
	return self->remote_pid;
}

/**
 * \<\<public\>\> Local PID setter.
 * 
 * @param *self - pointer to this task instance
 * @param local_pid - new local PID of the task
 */
static inline void tcmi_task_set_local_pid(struct tcmi_task *self, pid_t local_pid)
{
	self->local_pid = local_pid;
}

/** */
static int tcmi_task_show_remote_pid_rev(void *obj, void *data)
{
	struct tcmi_task *self = TCMI_TASK(obj);
	int * local_pid = (int*) data;
	*local_pid = tcmi_task_local_pid(self);
	return 0;
}

/**
 * \<\<public\>\> Remote PID setter.
 * 
 * @param *self - pointer to this task instance
 * @param remote_pid - new remote PID of the task
 */
static inline void tcmi_task_set_remote_pid(struct tcmi_task *self, pid_t remote_pid)
{
	self->remote_pid = remote_pid;
	self->f_remote_pid_rev = tcmi_ctlfs_intfile_new(self->d_migman_root, TCMI_PERMS_FILE_R,
                                     self, tcmi_task_show_remote_pid_rev, NULL,
                                     sizeof(pid_t),"%d" , self->remote_pid);

}

/**
 * \<\<public\>\> Accessor of id of the migration manager associated with this task.
 */
extern u_int32_t tcmi_task_migman_id(struct tcmi_task *self);

/**
 * \<\<public\>\> Accessor of vector slot index of the migration manager associated with this task.
 */
extern u_int tcmi_task_migman_slot_index(struct tcmi_task *self);

/**
 * \<\<public\>\> Waits until the task is picked up, by the associated
 * process.  Returns also when the timer expires. This method is meant
 * to be used by the task helper, that provides \link
 * tcmi_taskhelper.h::tcmi_taskhelper_wait_for_pick_up_timeout()
 * tcmi_taskhelper_wait_for_pickup_timeout() \endlink.
 *
 * @param *self - pointer to this task instance
 * @param timeout - time out limit in ticks to wait for pickup
 * @return 0 when successfully completed
 */
static inline unsigned long tcmi_task_wait_for_pick_up_timeout(struct tcmi_task *self, 
							       unsigned long timeout)
{
	return wait_for_completion_interruptible_timeout(&self->picked_up, timeout);
}

/**
 * \<\<public\>\> Signals, the tcmi task has been pickedup by the
 * associated process.n
 *
 * @param *self - pointer to this task instance
 * @return 0 when successfully completed
 */
static inline void tcmi_task_pick_up(struct tcmi_task *self)

{
	complete(&self->picked_up);
	self->picked_up_flag = 1;
}

/**
 * \<\<public\>\> Checks whether the task has been picked up.
 *
 * @param *self - pointer to this task instance
 * @return true when it has already been picked up
 */
static inline int tcmi_task_picked_up(struct tcmi_task *self)

{
	return self->picked_up_flag > 0;
}

/** 
 * \<\<public\>\> Sends a specified message.  The message is sent as
 * anonymous, so that any potential responses will be delivered to the
 * message queue.
 *
 * @param *self - pointer to this task instance
 * @param *m - message to be sent.
 * @return 0 when the message has been successfully sent
 */
static inline int tcmi_task_send_anonymous_msg(struct tcmi_task *self, struct tcmi_msg *m)
{
	return tcmi_msg_send_anonymous(m, self->sock);
}

static inline int tcmi_task_get_execve_count(struct tcmi_task *self)

{
	return self->execve_count;
}

static inline void tcmi_task_inc_execve_count(struct tcmi_task *self)

{
	self->execve_count++;
}

/** Returns a task_struct associated with the tcmi_task */
static inline struct task_struct* tcmi_task_to_task_struct(struct tcmi_task *self) {     
	struct task_struct *task;
	
	/* try finding the target kernel task for our TCMI task by PID */
	read_lock_irq(&tasklist_lock);
	if (!(task = task_find_by_pid(tcmi_task_local_pid(self)))) {
		mdbg(ERR3, "Can't find corresponding task_struct for PID: %d", 
		     tcmi_task_local_pid(self));		
	}	
	read_unlock_irq(&tasklist_lock);
  
	return task;
}


/** 
 * \<\<public\>\> Sends a specified message and waits for a response.
 * All work is delegate to tcmi_msg.c::tcmi_msg_send_and_receive()
 * method
 *
 * @param *self - pointer to this task instance
 * @param *req - request message to be sent.
 * @param **resp - output parameter, stores the response message or NULL
 * @return 0 when the message has been sent.
 */
static inline int tcmi_task_send_and_receive_msg(struct tcmi_task *self, struct tcmi_msg *req,
						 struct tcmi_msg **resp)
{
	return tcmi_msg_send_and_receive(req, self->sock, resp);
}

/** 
 * Notifies task that it peer is lost and sends a kill signal to that task.
 * The method is used for termination of tasks with lost peers.
 */
static inline void tcmi_task_signal_peer_lost(struct tcmi_task *self) {
      self->peer_lost = 1;
      force_sig(SIGKILL, tcmi_task_to_task_struct(self));
}

/**
 * \<\<public\>\> Processes a specified signal.  The work is delegated
 * to the task specific method if one is specified. This allows the
 * shadow and guest processes to handle the signal differently. In
 * fact, they have to: - shadow process takes appropriate actions and
 * forwards the signal to the guest using signal forwarding
 * component(since the guest might not be in migration mode) - guest
 * process takes only appropriate actions and is able to communicate
 * with the shadow directly.
 *
 * @param *self - pointer to this task instance 
 * @param signr - signal that is to be processed
 * @param *info - info for signal to be processed
 *
 * @return result of the task specific signal handling method or
 * TCMI_TASK_KEEP_PUMPING
 */
static inline int tcmi_task_do_signal(struct tcmi_task *self, unsigned long signr, siginfo_t *info)
{
	int res = TCMI_TASK_KEEP_PUMPING;

	mdbg(INFO2, "Processing signal %lu", signr);
	
	if ( self->peer_lost && signr == SIGKILL ) {
		mdbg(INFO2, "Peer task is dead, killing current task");
		return TCMI_TASK_KILL_ME;
	}
	
	if (self->ops->do_signal)
		res = self->ops->do_signal(self, signr, info);

	return res;

}


/**
 * \<\<public>\> Get task type
 * @param *self - pointer to this task instance 
 *
 * @return (tcmi_task_type)shadow or (tcmi_task_type)guest (or unresolved if not defined).
 */
static inline enum tcmi_task_type tcmi_task_get_type(struct tcmi_task *self)
{
	enum tcmi_task_type res = unresolved;
	if (self->ops->get_type)
		res = self->ops->get_type();
	return res;
}

/**
 * \<\<public>\> Computes the hash value of the task (to be used in tasks slotvec of migman)
 * 
 * @param local_pid - local pid of the task
 * @param hashmask - hashmask limiting the maximum hash value.
 */
static inline u_int tcmi_task_hash(pid_t local_pid, u_int hashmask)
{
	return (local_pid & hashmask);
}

/**
 * Simple error checking method to be used after communication operations to detect lost peer. In case err indicates broken connection, peer is considered dead for us.
 *
 * @return Returns passed err param, so that it can be easily used for call chaining
 */
static inline int tcmi_task_check_peer_lost(struct tcmi_task *self, int err) {
      // For now, we react only on broken pipe, but we may react on more cases, right?
      if ( err == -EPIPE )
	  tcmi_task_signal_peer_lost(self);

      return err;
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_TASK_PRIVATE

/** Read method for the TCMI ctlfs - reports remote PID. */
static int tcmi_task_show_remote_pid(void *obj, void *data);

/** Releases the execve context (execve file, argv, envp). */
static void tcmi_task_release_execve_context(struct tcmi_task *self);

/** 
 * \<\<private\>\> Adds the filled in method wrapper into the method queue. 
 *
 * 
 * @param *self - pointer to this task instance
 * @param *wr - method wrapper to be added to the method queue.
 */
static inline void tcmi_task_add_wrapper(struct tcmi_task *self, struct tcmi_method_wrapper *wr)
{
	tcmi_queue_add(&self->method_queue, &wr->node);
}

#endif /* TCMI_TASK_PRIVATE */


/**
 * @}
 */

#endif /* _TCMI_TASK_H */

