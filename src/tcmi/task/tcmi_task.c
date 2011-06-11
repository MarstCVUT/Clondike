/**
 * @file tcmi_task.c - common functionality for TCMI shadow and guest tasks.
 *                      
 * 
 *
 *
 * Date: 04/25/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_task.c,v 1.11 2009-01-20 14:23:10 andrep1 Exp $
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

/* needed, so that execve is visible */
int errno = 0;
#define __KERNEL_SYSCALLS__
#include <linux/unistd.h> 
#include <asm/uaccess.h>
#include <linux/kmod.h>

#include <linux/hardirq.h>
#include <linux/syscalls.h>


#define TCMI_TASK_PRIVATE
#include "tcmi_task.h"
#include <tcmi/manager/tcmi_migman.h>

#include "tcmi_taskhelper.h"

/** TODO: Move this somewhere to arch */
#if defined(__x86_64__)
/* static inline _syscall3(int, execve, const char *, filename, char *const *, argv, char *const *, envp); */

int clondike_execve(const char *filename, char *const argv[], char *const envp[]);

static inline int copyof_kernel_execve(const char *filename, char *const argv[], char *const envp[]) {
	return clondike_execve(filename, argv, envp);
}

#endif

/* static inline _syscall3(int, execve, const char *, filename, char *const *, argv, char *const *, envp); */

/*inline int kernel_execve(const char *filename, char *const argv[], char *const envp[])
{
	mm_segment_t fs = get_fs();
	int ret;

	WARN_ON(segment_eq(fs, USER_DS));
	ret = execve(filename, (char **)argv, (char **)envp);
	if (ret)
		ret = errno;

	return ret;
}*/


/** 2^8 elements in the transaction process hash */
#define TRANSACTION_ORDER 8

/** Initializes the task. 
 * Task initialization requires:
 * - setting PID's (remote PID is set to 0 it will be updated
 * once the migration is finished)
 * - getting an extra reference to the communication socket
 * - getting an extra reference to the associated migration manager
 * - initializing method and message queues
 * - create transaction slot vector
 * - creating directory in migproc using the local PID as its name
 * - creating symlink to the migration manager directory
 * - creating remote PID TCMI ctlfs file
 * - default exit code is set to 0, this might change when
 * the guest task terminates and sends back its exit code.
 * - setting all execve context to NULL (file, argv, envp)
 * - initialize the completion synchronization element, so that
 * the task instantiator (usually the migration component) can be 
 * notified when the task has been picked up by the associated process.
 * Also reset the picked_up flag. This is used for atomic check if the task
 * might have been pickedup short after the timeout.
 * - setting operations for the specific task
 *
 * @param *self - this task instance
 * @param local_pid - local PID on CCN or PEN
 * @param migman - migration manager that maintains this task (can be null)
 * @param *sock - socket used for communication with guest task on PEN
 * or shadow task on CCN resp.
 * @param *d_migproc - directory where ctlfs entries of migrated
 * processes reside
 * @param *d_migman - TCMI ctlfs directory of the migration manager
 * responsible for this task
 * @param *ops - task specific operations
 * @return 0 upon success
 */
int tcmi_task_init(struct tcmi_task *self, pid_t local_pid, struct tcmi_migman* migman,
		   struct kkc_sock *sock, 
		   struct tcmi_ctlfs_entry *d_migproc, struct tcmi_ctlfs_entry *d_migman,
		   struct tcmi_task_ops *ops)
{
	int err = -EINVAL;

	self->local_pid = local_pid;
	self->migman = tcmi_migman_get(migman);
	self->remote_pid = 0;
	self->sock = kkc_sock_get(sock);

	self->execve_count = 0;
	self->peer_lost = 0;

	self->d_migman_root = d_migman;
	self->f_remote_pid_rev = NULL;
	tcmi_queue_init(&self->method_queue);
	tcmi_queue_init(&self->msg_queue);

	if (!ops) {
		mdbg(ERR3, "Missing task operations!");
		goto exit0;
	}

	if (!(self->transactions = tcmi_slotvec_hnew(TRANSACTION_ORDER))) {
		mdbg(ERR3, "Can't create transactions slotvector!");
		err = -ENOMEM;
		goto exit0;
	}

	if (!(self->d_task = 
	      tcmi_ctlfs_dir_new(d_migproc, TCMI_PERMS_DIR, "%d", local_pid))) {
		mdbg(ERR3, "Can't create migproc entry for %d", local_pid);
		goto exit1;
	}

	if (!(self->s_migman = 
	      tcmi_ctlfs_symlink_new(self->d_task, d_migman, "migman"))) {
		mdbg(ERR3, "Can't create symlink to migration manager for %d", local_pid);
		goto exit2;
	}

	if (!(self->f_remote_pid = 
	      tcmi_ctlfs_intfile_new(self->d_task, TCMI_PERMS_FILE_R,
				     self, tcmi_task_show_remote_pid, NULL,
				     sizeof(pid_t), "remote-pid"))) {
		mdbg(ERR3, "Can't create remote PID file entry for %d", local_pid);
		goto exit3;
	}

	if ( migman && tcmi_migman_add_task(migman, self) ) {
		mdbg(ERR3, "Can't add task to migman tasks struct: lpid %d", local_pid);
		goto exit4;
	}

	self->exit_code = 0;
	atomic_set(&self->ref_count, 1);

	self->ckpt_pathname = NULL;
	self->argv = NULL;
	self->envp = NULL;

	init_completion(&self->picked_up);
	self->picked_up_flag = 0;
	self->ops = ops;

	minfo(INFO4, "Initialized TCMI task PID=%d, %p", local_pid, self);
	return 0;

	/* error handling */
 exit4:
	tcmi_ctlfs_entry_put(self->f_remote_pid);
 exit3:
	tcmi_ctlfs_entry_put(self->s_migman);
 exit2:
	tcmi_ctlfs_entry_put(self->d_task);
 exit1:
	tcmi_slotvec_put(self->transactions);
 exit0:
	kkc_sock_put(self->sock);
	tcmi_migman_put(self->migman);
	return err;
}


/** 
 * \<\<public\>\> Releases the instance. 
 * - calls custom free method for a specific task
 * - destroys all ctlfs files, directories, symlinks
 * - destroys the transaction slot vector
 * - empties the message queue
 * - empties the method queue
 * - releases the execve context (execve filename, argv, envp) 
 *
 * @param *self - pointer to this task instance
 */
void tcmi_task_put(struct tcmi_task *self)
{
	/* Used for destruction of messages and methods. */
	struct tcmi_msg *m;
	struct tcmi_method_wrapper *w;

	if (self && atomic_dec_and_test(&self->ref_count)) {
		mdbg(INFO4, "Destroying TCMI task PID=%d, %p", 
		     tcmi_task_local_pid(self), self);
		/* instance specific free method */
		if (self->ops->free)
			self->ops->free(self);
		kkc_sock_put(self->sock);
		/* destroy all ctlfs entries */
		tcmi_ctlfs_file_unregister(self->f_remote_pid);
		tcmi_ctlfs_entry_put(self->f_remote_pid);
		tcmi_ctlfs_entry_put(self->s_migman);
		tcmi_ctlfs_entry_put(self->d_task);
		tcmi_ctlfs_entry_put(self->f_remote_pid_rev);

		if (!tcmi_slotvec_empty(self->transactions))
			mdbg(WARN2, "Task %d, destroying non-empty transactions slotvector!!",
			     self->local_pid);
		tcmi_slotvec_put(self->transactions);

		/* destroy remaining messages in the queue. */
		while (!tcmi_queue_empty(&self->msg_queue)) {
			tcmi_queue_remove_entry(&self->msg_queue, m, node);
			mdbg(INFO3, "Task %d, destroying message %x",
			     tcmi_task_local_pid(self), tcmi_msg_id(m));
			tcmi_msg_put(m);
		}
		/* destroy remaining methods in the queue. */
		while (!tcmi_queue_empty(&self->method_queue)) {
			tcmi_queue_remove_entry(&self->method_queue, w, node);
			mdbg(INFO3, "Task %d, destroying method wrapper %p",
			     tcmi_task_local_pid(self), w);
			tcmi_method_wrapper_put(w);
		}
		minfo(INFO3, "Releasing execve context");
		tcmi_task_release_execve_context(self);
		minfo(INFO3, "Done..");
		tcmi_migman_remove_task(self->migman, self);
		minfo(INFO3, "Removed from migman tasks done");
		tcmi_migman_put(self->migman);
		kfree(self);
	}
}



/** 
 * \<\<public\>\> Submits a method for execution. 
 * Creates a new method wrapper with the specified data and delegates
 * all work to tcmi_task_add_wrapper
 *
 * @param *self - pointer to this task instance
 * @param *method - pointer to the method that is to be submitted
 * @param *data - argument for the submitted method
 * @param size - size of the argument in bytes
 */
void tcmi_task_submit_method(struct tcmi_task *self, tcmi_method_t *method, 
			     void *data, int size)
{
	struct tcmi_method_wrapper *wr;

	if (!(wr = tcmi_method_wrapper_new(method, data, size))) {
		mdbg(ERR3, "Can't create method wrapper");
		goto exit0;
	}

	tcmi_task_add_wrapper(self, wr);
 exit0:
	return;
}

/** 
 * \<\<public\>\> Flushes all pending methods in the queue submits a method for execution. 
 * This operation must be atomic - under a queue lock, all methods are
 * removed and a new wrapper is added to the method queue.
 *
 * @param *self - pointer to this task instance
 * @param *method - pointer to the method that is to be submitted
 * @param *data - argument for the submitted method
 * @param size - size of the argument in bytes
 */
void tcmi_task_flush_and_submit_method(struct tcmi_task *self, tcmi_method_t *method, 
				       void *data, int size)
{
	struct tcmi_method_wrapper *wr, *old_w;
	if (!(wr = tcmi_method_wrapper_new(method, data, size))) {
		mdbg(ERR3, "Can't create method wrapper");
		goto exit0;
	}

	tcmi_queue_lock(&self->method_queue);
	/* flush everything in the queue. */
	while (!tcmi_queue_empty(&self->method_queue)) {
		__tcmi_queue_remove_entry(&self->method_queue, old_w, node);
		tcmi_method_wrapper_put(old_w);
	}
	/* append the requested method into the queue. */
	__tcmi_queue_add(&self->method_queue, &wr->node);

	tcmi_queue_unlock(&self->method_queue);
	mdbg(INFO3, "Flushed method queue and submitted new method in atomic=%d", in_atomic());
 exit0:
	return;
}


/** 
 * \<\<public\>\> Processes all methods in the queue.  This method is
 * used as a method pump, it keeps on unwrapping methods from the
 * queue and executing them until one of them returns a results
 * different from TCMI_TASK_KEEP_PUMPING. This result is communicated
 * to the component that controls this particular task (usually the
 * migration component). Every iteration, the signals are also
 * handled. This might also influence the method pump and terminate
 * it.
 *
 * The method wrapper is released once the method is executed only if
 * there was no execve problem. To explain this: some methods perform
 * an execve operation - in which case, the execution context changes
 * immediately. Therefore, these methods have to discard the method
 * wrapper on their own. Should the execve fail, they have already
 * discarded the wrapper and thus have on control over it. In that
 * case, the returned error is TCMI_TASK_EXECVE_FAILED_KILL_ME and we
 * have to prevent releasing this method wrapper.
 *
 * The specific task method that had been executed had a
 * chance to get an extra reference and resubmit the method wrapper
 * into the method queue.
 *
 * @param *self - pointer to this task instance
 * @return result of the last executed method
 */
int tcmi_task_process_methods(struct tcmi_task *self)
{
	int res = TCMI_TASK_KEEP_PUMPING;
	struct tcmi_method_wrapper *w;

	while ((!tcmi_queue_empty(&self->method_queue)) && 
	       (res == TCMI_TASK_KEEP_PUMPING)) {
		/* handle any pending signals */
		if ((res = tcmi_taskhelper_do_signals(self)) != TCMI_TASK_KEEP_PUMPING)
			break;

		/* get next method to proces */
		tcmi_queue_remove_entry(&self->method_queue, w, node);
		/* unwrap & destroy the method wrapper excluding the execve failure */
		if ((res = tcmi_method_wrapper_call(w, self)) != 
		    TCMI_TASK_EXECVE_FAILED_KILL_ME) {			
			mdbg(INFO3, "Execve have NOT failed.");
			tcmi_method_wrapper_put(w);
		} else {
			mdbg(INFO3, "Execve have failed.");
		}
	}

	mdbg(INFO3, "Process methods finished with res %d", res);

	return res;
}



/** 
 * \<\<public\>\> Called from method queue - Processes one message
 * from the message queue.
 *
 * - Waits for the message to arrive or a signal to wake us up.
 * - Upon message arrival, the message is processed (delegated to the
 * task specific method if any) and discarded.  
 * - Eventually, the method resubmits itself into the method queue
 * reusing the method wrapper iff the result of the message processing
 * is TCMI_TASK_KEEP_PUMPING.
 *
 * Optionally, this method can take an integer argument (argument to wrapper).
 * This argument specifies, whether we should be waiting for new messages, if there
 * is no message, or not. By default, this method waits for messages and in the
 * end it resubmits itself. If wait for messages is false, and there are no messages
 * this method returns.
 *
 * @param *self - pointer to this task instance
 * @param *wr - method wrapper used to store the method in the 
 * method queue
 * @return result of the last executed method
 */
int tcmi_task_process_msg(void *self, struct tcmi_method_wrapper *wr)
{
	int res = TCMI_TASK_KEEP_PUMPING;
	int err;
	struct tcmi_msg *m; /* message that is being processed */
	struct tcmi_task *self_tsk = TCMI_TASK(self);
	int wait_for_messages = 1;

	if ( tcmi_method_wrapper_data(wr) ) {
		wait_for_messages = *(int*)tcmi_method_wrapper_data(wr);
	}

	// No msgs and we do not want to wait?
	if ( !wait_for_messages && tcmi_queue_empty(&self_tsk->msg_queue) )
		return res;

	if ((err = tcmi_queue_wait_on_empty_interruptible(&self_tsk->msg_queue)) < 0) {
		mdbg(INFO3, "Signal arrived %d", err);
		goto exit0;
	}
	tcmi_queue_remove_entry(&self_tsk->msg_queue, m, node);
	/* process the message that has arrived */
	if (m) {
		mdbg(INFO3, "Processing message..%x", tcmi_msg_id(m));
		if (self_tsk->ops->process_msg)
			res = self_tsk->ops->process_msg(self_tsk, m);
		/* discard the mesage*/
		tcmi_msg_put(m);
	}

 exit0:
	/* resubmit the process_msg method. */
	if (res == TCMI_TASK_KEEP_PUMPING) {
		/* This is to have an extra reference for resubmitting */
		tcmi_method_wrapper_get(wr);
		/* resubmit the method. */
		tcmi_task_add_wrapper(self_tsk, wr);
	}
	return res;

}

/**
 * \<\<public\>\> Called from method queue - Sends a message to peer task.
 * The method is primarily designed for retransmission of messages in case the original transimision failed due to pending signals.
 *
 * @param *self - pointer to this task instance
 * @param *wr - method wrapper used to store the method in the 
 * method queue
 * @return result of the task specific emigrate method or 
 * TCMI_TASK_KEEP_PUMPING
 */
int tcmi_task_send_message(void *self, struct tcmi_method_wrapper *wr)
{
  	int res = TCMI_TASK_KEEP_PUMPING;
	int send_res;
	struct tcmi_task *self_tsk = TCMI_TASK(self);
	struct tcmi_msg *resp = *((struct tcmi_msg**)tcmi_method_wrapper_data(wr));
	
	//ERESTARTSYS
	send_res = tcmi_task_send_anonymous_msg(self_tsk, resp);
	if ( send_res == -ERESTARTSYS ) {
	  minfo(ERR3, "Signal arrived while sending message, will retransmit later.");
	  tcmi_msg_get(resp);
	  tcmi_task_submit_method(self, tcmi_task_send_message, resp, sizeof(struct tcmi_msg));	  
	} else if ( send_res != 0 ) {
	  minfo(ERR3, "Sending of message has failed with error: %d. The message will NOT be retransmitted.", send_res);
	} else {
	  mdbg(INFO3, "Message send");
	}
	
	tcmi_msg_put( resp );
	
	return res;
}

/** 
 * \<\<public\>\> Called from method queue - Emigrates a task to a PEN
 * - PPM w/ physical checkpoint.  All the work is delegated to the
 * task specific emigrate method.
 *
 * @param *self - pointer to this task instance
 * @param *wr - method wrapper used to store the method in the 
 * method queue
 * @return result of the task specific emigrate method or 
 * TCMI_TASK_KEEP_PUMPING
 */
int tcmi_task_emigrate_ppm_p(void *self, struct tcmi_method_wrapper *wr)
{
	int res = TCMI_TASK_KEEP_PUMPING;
	struct tcmi_task *self_tsk = TCMI_TASK(self);

	mdbg(INFO3, "Task - local pid %d, remote pid %d, emigrating",
	     tcmi_task_local_pid(self_tsk), tcmi_task_remote_pid(self_tsk));

	if (self_tsk->ops->emigrate_ppm_p)
		res = self_tsk->ops->emigrate_ppm_p(self_tsk);
	return res;
}

/** 
 * \<\<public\>\> Called from method queue - Migrates a task back to
 * CCN - PPM w/ physical checkpoint.  All the work is delegated to the
 * task specific emigrate method.
 *
 * @param *self - pointer to this task instance
 * @param *wr - method wrapper used to store the method in the 
 * method queue
 * @return result of the task specific migrate back method or 
 * TCMI_TASK_KEEP_PUMPING
 */
int tcmi_task_migrateback_ppm_p(void *self, struct tcmi_method_wrapper *wr)
{
	int res = TCMI_TASK_KEEP_PUMPING;
	struct tcmi_task *self_tsk = TCMI_TASK(self);

	mdbg(INFO3, "Task - local pid %d, remote pid %d, migrating back",
	     tcmi_task_local_pid(self_tsk), tcmi_task_remote_pid(self_tsk));

	if (self_tsk->ops->migrateback_ppm_p)
		res = self_tsk->ops->migrateback_ppm_p(self_tsk);
	return res;
}

/** 
 * \<\<public\>\> Called from method queue - Emigrates a task to a PEN
 * - PPM w/ virtual checkpoint.  All the work is delegated to the task
 * specific emigrate method.
 *
 * @param *self - pointer to this task instance
 * @param *wr - method wrapper used to store the method in the 
 * method queue
 * @return result of the task specific emigrate method or 
 * TCMI_TASK_KEEP_PUMPING
 */
int tcmi_task_emigrate_ppm_v(void *self, struct tcmi_method_wrapper *wr)
{
	int res = TCMI_TASK_KEEP_PUMPING;
	struct tcmi_task *self_tsk = TCMI_TASK(self);

	mdbg(INFO3, "Task - local pid %d, remote pid %d, emigrating",
	     tcmi_task_local_pid(self_tsk), tcmi_task_remote_pid(self_tsk));

	if (self_tsk->ops->emigrate_ppm_v)
		res = self_tsk->ops->emigrate_ppm_v(self_tsk);
	return res;
}

/** 
 * \<\<public\>\> Called from method queue - Migrates a task back to
 * CCN - PPM w/ virtual checkpoint.  All the work is delegated to the
 * task specific emigrate method.
 *
 * @param *self - pointer to this task instance
 * @param *wr - method wrapper used to store the method in the 
 * method queue
 * @return result of the task specific migrate back method or 
 * TCMI_TASK_KEEP_PUMPING
 */
int tcmi_task_migrateback_ppm_v(void *self, struct tcmi_method_wrapper *wr)
{
	int res = TCMI_TASK_KEEP_PUMPING;
	struct tcmi_task *self_tsk = TCMI_TASK(self);

	mdbg(INFO3, "Task - local pid %d, remote pid %d, migrating back",
	     tcmi_task_local_pid(self_tsk), tcmi_task_remote_pid(self_tsk));

	if (self_tsk->ops->migrateback_ppm_v)
		res = self_tsk->ops->migrateback_ppm_v(self_tsk);
	return res;
}

/** 
 * \<\<public\>\> Called from method queue - Emigrates a task to a PEN
 * - NPM.  All the work is delegated to the task specific emigrate
 * method.
 *
 * @param *self - pointer to this task instance
 * @param *wr - method wrapper used to store the method in the 
 * method queue
 * @return result of the task specific emigrate method or 
 * TCMI_TASK_KEEP_PUMPING
 */
int tcmi_task_emigrate_npm(void *self, struct tcmi_method_wrapper *wr)
{
	int res = TCMI_TASK_KEEP_PUMPING;
	struct tcmi_task *self_tsk = TCMI_TASK(self);
	struct tcmi_npm_params* npm_params = *((struct tcmi_npm_params**)tcmi_method_wrapper_data(wr));

	mdbg(INFO3, "Task - local pid %d, remote pid %d, emigrating",
	     tcmi_task_local_pid(self_tsk), tcmi_task_remote_pid(self_tsk));

	if (self_tsk->ops->emigrate_npm)
		res = self_tsk->ops->emigrate_npm(self_tsk, npm_params);
	return res;
}

/** 
 * \<\<public\>\> Called from method queue - Migrates a task back to
 * CCN - NPM.  work is delegated to the task specific emigrate method.
 *
 * @param *self - pointer to this task instance
 * @param *wr - method wrapper used to store the method in the 
 * method queue
 * @return result of the task specific migrate back method or 
 * TCMI_TASK_KEEP_PUMPING
 */
int tcmi_task_migrateback_npm(void *self, struct tcmi_method_wrapper *wr)
{
	int res = TCMI_TASK_KEEP_PUMPING;
	struct tcmi_task *self_tsk = TCMI_TASK(self);
	struct tcmi_npm_params* npm_params = tcmi_method_wrapper_data(wr);	

	mdbg(INFO3, "Task - local pid %d, remote pid %d, migrating back",
	     tcmi_task_local_pid(self_tsk), tcmi_task_remote_pid(self_tsk));

	if (self_tsk->ops->migrateback_npm)
		res = self_tsk->ops->migrateback_npm(self_tsk, npm_params);
	return res;
}

/** 
 * \<\<public\>\> Called from method queue - Exits a task.  This
 * method is primarily intended terminate the main method processing
 * loop of the task. The task is then removed from its migration
 * manager and destroyed. Task specific method has the chance to
 * communicate this to its counter part. E.g. a guest task sends an
 * exit code to the associated shadow process on the CCN.
 *
 * The method wrapper might contain an exit code. If so, it is communicated
 * to the specific task method. The default exit code is 0.
 *
 * @param *self - pointer to this task instance
 * @param *wr - method wrapper that stores the requested exit code.
 * @return result of the task specific exit method or 
 * TCMI_TASK_REMOVE_AND_LET_ME_GO
 */
int tcmi_task_exit(void *self, struct tcmi_method_wrapper *wr)
{
	int res = TCMI_TASK_REMOVE_AND_LET_ME_GO;
	struct tcmi_task *self_tsk = TCMI_TASK(self);
	long *pcode, code;
	minfo(INFO3, "Doing exit");
	/* extract the exit code if any. */
	pcode = (long*)tcmi_method_wrapper_data(wr);
	code = (pcode ? *pcode : 0);

	minfo(INFO3, "Executing custom exit method");
	if (self_tsk->ops->exit)
		res = self_tsk->ops->exit(self_tsk, code);
	minfo(INFO3, "Executed custom exit method");

	return res;
}


/**
 * \<\<private\>\> Class method - copies an array of strings terminated by NULL
 * string (standard argv/envp format) into the destination
 * array. Destination array are released and new memory for the
 * strings is allocated. The idea how the array in the task
 * will be organized is sketched below.
 *
 * @param ***dst - pointer(*) to the destination array(**) (should be
 * &self->argv or &self->envp
 * @param **src - source array of strings
 * @return 0 upon success
 */
static inline int tcmi_task_copy_strings(char ***dst, char **src)
{
	/* auxiliary storage when computing argv/envp length's */
	char **tmp_src = src;
	/* refers to current string being processed */
	char *src_data;
	/* used when copying the strings over */ 
	char **new_src;
	/* number of elements and total size of all elements in bytes */
	unsigned int src_len = 0, src_size = 0;


	/* calculate the number of elements and total array size, so
	 * that we can allocate it all at once */
	for(;*tmp_src != NULL;src_len++, src_size += strlen(*tmp_src) + 1, tmp_src++);
	/* account for the terminating NULL */
	src_len++;
	
	mdbg(INFO3, "src count %d, src size %d", src_len, src_size);

	if (!(new_src = *dst = (char**)kmalloc((sizeof(char*) * src_len) + src_size, GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate memory for src array!!");
		goto exit0;
	}

	/* copy the src array over */
	/* compute the start of the data section for src */
	src_data = (char*) *dst + (sizeof(char*) * src_len);
	for (tmp_src = src; *tmp_src != NULL; tmp_src++, new_src++) {
		mdbg(INFO4, "Copying '%s', length %lu", *tmp_src, (unsigned long)strlen(*tmp_src));
		strcpy(src_data, *tmp_src);
		*new_src = src_data;
		src_data += strlen(*tmp_src) + 1;
	}
	*new_src = NULL; /* each array terminated by NULL as required by execve */
	return 0;

	/* error handling */
 exit0:
	return -EINVAL;
}

/**
 * \<\<public\>\> Prepares execution of a file given the arrays of arguments and
 * environment.  Allocates memory for all execve parameters and copies
 * the parameters into the instance.  The main purpose of this method
 * is setup everything for execve. After using this method, the caller
 * is able to release all memory resources and issue
 * tcmi_task_execve().
 *
 * To reduce the overhead when releasing the argv/envp arrays of the task,
 * the layout of each array is as follows. All strings are put
 * at the end of the array.
 *
 \verbatim
 +-------------+
 | pointer  0  |--+
 +-------------+  |
 | pointer  1  |--------------------+
 +-------------+  |                 |
 .             .  |                 |
 .             .  |                 |
 +-------------+  |                 |
 | pointer  n  |---------------------------------------------+
 +-------------+  |                 |                        |
 | NULL        |  |                 |                        |
 +-------------+--V-----------------V------------------------V----------+
 | "string 0 goes up here\0", "string 1 here\0", ..., "string n here\0" |
 +----------------------------------------------------------------------+

 \endverbatim 
 * This code can be used to printout the argv/envp arrays for testing.
 \verbatim
	int i;
	mdbg(INFO3, "Environ test");
	for(i = 0; self->argv[i] != NULL; i++)
		mdbg(INFO3, "ARGV[%d]='%s'", i, self->argv[i]);

	for(i = 0; self->envp[i] != NULL; i++)
		mdbg(INFO3, "ENVP[%d]='%s'", i, self->envp[i]); 

 \endverbatim
 *
 * @param *self - pointer to this task instance
 * @param *file
 * @param **argv - arguments list terminated by NULL
 * @param **envp - environment strings terminated by NULL
 * @return 0 upon success, otherwise -ENOEXEC
 */
int tcmi_task_prepare_execve(struct tcmi_task *self, char *file, char **argv, char **envp)
{
	/* release the old context first. */
	tcmi_task_release_execve_context(self);

	if (!(self->ckpt_pathname = (char*)kmalloc(strlen(file) + 1, GFP_ATOMIC))) {
		mdbg(ERR3, "Can't allocate memory for execve file '%s'", file);
		goto exit0;
	}
	strcpy(self->ckpt_pathname, file);
	if (tcmi_task_copy_strings(&self->argv, argv) < 0) {
		mdbg(ERR3, "Failed copying ARGV array for file '%s'", file);
		goto exit1;
	}
	if (tcmi_task_copy_strings(&self->envp, envp) < 0) {
		mdbg(ERR3, "Failed copying ENVP array for file '%s'", file);
		goto exit2;
	}
	mdbg(INFO4, "Allocated TCMI task execve context(%s, %p, %p) PID=%d, %p", 
	     self->ckpt_pathname, self->argv, self->envp, tcmi_task_local_pid(self), self);
	return 0;

	/* error handling */
 exit2:
	kfree(self->argv);
	self->argv = NULL;
 exit1:
	kfree(self->ckpt_pathname);
	self->ckpt_pathname = NULL;
 exit0:
	return -ENOEXEC;
}

/**
 * \<\<public\>\> Executes the file that has been previously setup for
 * execution.  Prior to performing the exec, it is necessary to
 * discard the method wrapper as the method pump won't resume anymore
 * in this context. Also the instance specific execve method is
 * called. This allows a particular TCMI task to get ready for an
 * upcoming execve.
 *
 * @param *self - pointer to this task instance
 * @param *wr - method wrapper that carried this method in the queue.
 * @return - doesn't return upon succesful execve, otherwise 
 * TCMI_TASK_EXECVE_FAILED_KILL_ME to inform the method pump to quit
 * without discarding the method wrapper as we have already done so.
 */
int tcmi_task_execve(void *self, struct tcmi_method_wrapper *wr)
{
	int err = 0;
	mm_segment_t oldfs;
	struct tcmi_task *self_tsk = TCMI_TASK(self);

memory_sanity_check("Sanity check on execve");
	
	mdbg(INFO2, "Performing exec on '%s' Args: %p ([0] -> %p) Envp: %p ([0] -> %p) in atomic=%d", self_tsk->ckpt_pathname, self_tsk->argv, self_tsk->argv[0], self_tsk->envp, self_tsk->envp[0], in_atomic());
	/* discard the method wrapper prior to doing anything */
	tcmi_method_wrapper_put(wr);
	/* instance specific execve notification */
	if (self_tsk->ops->execve)
		err = self_tsk->ops->execve(self_tsk);
	if (err < 0) {
		mdbg(ERR3, "Task specific execve failed!");
		goto exit0;
	}
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	//mm_segment_t fs = get_fs();
memory_sanity_check("Sanity check on execve - just before the call");
	if ((err = copyof_kernel_execve(self_tsk->ckpt_pathname, self_tsk->argv,
	/* if ((err = execve(self_tsk->ckpt_pathname, self_tsk->argv, */
			  self_tsk->envp)) < 0) {
		minfo(ERR3, "Execve failed!!! %d, errno=%d", err, errno);
	}
	set_fs(oldfs);

	mdbg(INFO2, "Exec done!");

	/* error handling */
 exit0:
	return TCMI_TASK_EXECVE_FAILED_KILL_ME;
}

/** @addtogroup tcmi_task_class
 *
 * @{
 */
/** 
 * \<\<private\>\> Read method for the TCMI ctlfs - reports remote
 * PID.  For a shadow process, the remote PID is the identifier of the
 * guest process on PEN and vica versa.
 *
 * @param *obj - pointer to this task instance
 * @param *data - pointer where the PID is to be stored.
 * @return 0 upon success
 */
static int tcmi_task_show_remote_pid(void *obj, void *data)
{
	struct tcmi_task *self = TCMI_TASK(obj);
	int *remote_pid = (int*) data;
	
	*remote_pid = tcmi_task_remote_pid(self);

	return 0;
}


/** 
 * \<\<private\>\> Releases the execve context (execve file, argv,
 * envp) Since the argv/envp have allocated a linear chunks of memory
 * (including all strings) we need to issue only 1 kfree/vfree for
 * each and don't need to scan the array releasing each string
 * separate. Releasing is atomic as this method is also used when
 * preparing a new execve context (tcmi_task_prepare_execve()).
 *
 * @param *self - pointer to this task instance
 */
static void tcmi_task_release_execve_context(struct tcmi_task *self)
{
	char *tmp1, **tmp2;
	mdbg(INFO4, "Destroying TCMI task execve context(%p, %p, %p) PID=%d, %p", 
	     self->ckpt_pathname, self->argv, self->envp, tcmi_task_local_pid(self), self);
	tmp1 = self->ckpt_pathname;
	self->ckpt_pathname = NULL;
	kfree(tmp1);

	tmp2 = self->argv;
	self->argv = NULL;
	kfree(tmp2);

	
	tmp2 = self->envp;
	self->envp = NULL;
	kfree(tmp2);
}

/** 
 * \<\<public\>\> Delivers a specified message.  Asks the message to
 * deliver itself into a message queue or associate itself with a
 * matching transaction. The result is communicated back.
 *
 * If the message is delivered sucessfully, an enter to the migration mode
 * is requested.
 *
 * @param *self - pointer to this task instance
 * @param *m - message to be delivered.
 * @return 0 when the message has been successfully delivered.
 */
int tcmi_task_deliver_msg(struct tcmi_task *self, struct tcmi_msg *m)
{
	int res = tcmi_msg_deliver(m, &self->msg_queue, self->transactions);
	if ( res ) {
		minfo(ERR3, "Failed to deliver the message: errno=%d", res);

		return res;
	}

	// Request migration mode only for proc messages with enforce migmode flag specified
	if (TCMI_MSG_GROUP(tcmi_msg_id(m)) == TCMI_MSG_GROUP_PROC && tcmi_procmsg_enforce_migmode(TCMI_PROCMSG(m)) ) {
		mdbg(INFO4, "Message successfully delivered, requesting migration mode for pid: %d", tcmi_task_local_pid(self));	
		// TODO: Nasty workaround.. this should likely by in a guest task specific code, since it makes no sense for shadow
		if ( tcmi_queue_empty(&self->method_queue) ) { 
			int wait_for_msg = 0;
			// If there is no method in method queue, there is nothing that would process the message, so we have to submit the processing method into the queue
			tcmi_task_submit_method(self, tcmi_task_process_msg, &wait_for_msg, sizeof(int));
		}
		tcmi_taskhelper_enter_mig_mode(self);
	}

	return 0;
}

/**
 * \<\<public\>\> Accessor of id of the migration manager associated with this task.
 * 
 * @param *self - pointer to this task instance
 * @return migration manager id
 */
u_int32_t tcmi_task_migman_id(struct tcmi_task *self)
{
	return self->migman ? tcmi_migman_id(self->migman) : -1;
}

/**
 * \<\<public\>\> Accessor of vector slot index of the migration manager associated with this task.
 */
u_int tcmi_task_migman_slot_index(struct tcmi_task *self) {
	return self->migman ? tcmi_migman_slot_index(self->migman) : -1;
}

/**
 * @}
 */
