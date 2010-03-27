/**
 * @file tcmi_migman.c - Implementation of TCMI cluster core node migration
 *                          manager - a class that controls task migration on CCN
 *                      
 * 
 *
 *
 * Date: 04/19/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_migman.c,v 1.8 2009-01-20 14:23:03 andrep1 Exp $
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

#include <linux/types.h>
#include <linux/random.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/sched.h>

#include <tcmi/task/tcmi_taskhelper.h>
#include <tcmi/task/tcmi_task.h>

#define TCMI_MIGMAN_PRIVATE
#include "tcmi_migman.h"

#include <dbg.h>

/** 2^8 elements in the transaction process hash*/
#define TRANSACTION_ORDER 8
/** 2^9 elements in the process hash*/
#define TASK_ORDER 9
/** Max length of the state description */
#define TCMI_MIGMAN_STATE_LENGTH 40

/** TCMI migration manager initializer. */
/**
 * \<\<public\>\> TCMI migration manager initializer.
 * The initialization is accomplished exactly in this order:
 * - set ID's and initial state
 * - initialize message queue and the transaction vector
 * - choose a unique ID for the manager.
 * - store a reference to a mig proc directory - it's required when
 * immigrating a process.
 * - start TCMI communication component
 * - create TCMI ctlfs directories 
 * - create TCMI ctlfs control files 
 * - create a TCMI socket to represent the connection in filesystem
 * - run the message processing thread.
 *
 * @param *self - this migration manager instance
 * @param *sock - socket where the control connection between CCN/PEN is running
 * @param ccn_id - CCN identifier - valid when initializing a CCN mig. manager
 * @param pen_id - PEN identifier - valid when initializing a PEN mig. manager
 * @param peer_arch_type - Architecture of the peer node
 * @param *root - directory where the migration manager should create its
 * files and directories
 * @param *migproc - directory where the migrated process will have their info
 * @param *ops - instance specific operations
 * @param namefmt - nameformat string for the main manager directory name (printf style)
 * @param args - variable arguments list
 * @return 0 upon success
 */
int tcmi_migman_init(struct tcmi_migman *self, struct kkc_sock *sock, 
		     u_int32_t ccn_id, u_int32_t pen_id, 
		     enum arch_ids peer_arch_type,u_int slot_index,
		     struct tcmi_ctlfs_entry *root,
		     struct tcmi_ctlfs_entry *migproc,
		     struct tcmi_migman_ops *ops,
		     const char namefmt[], va_list args)
{
	int err = -EINVAL;

	minfo(INFO2, "Creating new TCMI CCN migration manager");
	self->state = TCMI_MIGMAN_INIT;
	self->ccn_id = ccn_id;
	self->pen_id = pen_id;
	self->slot_index = slot_index;
	self->peer_arch_type = peer_arch_type;
	atomic_set(&self->ref_count, 1);
	tcmi_queue_init(&self->msg_queue);

	get_random_bytes(&self->id, sizeof(u_int32_t));
	self->ops = ops;
	if (!ops) {
		mdbg(ERR3, "Missing migration manager operations!");
		goto exit0;
	}
	
	if (!ops->process_msg) {
		mdbg(ERR3, "Missing process message operation!");
		goto exit0;
	}

	if (!(self->transactions = tcmi_slotvec_hnew(TRANSACTION_ORDER))) {
		mdbg(ERR3, "Can't create transactions slotvector!");
		goto exit0;
	}
	
	if (!(self->tasks = tcmi_slotvec_hnew(TASK_ORDER))) {
		mdbg(ERR3, "Can't create tasks slotvector!");
		goto exit1;
	} 

	/* TCMI comm that accepts all types of messages */
	if (!(self->comm = tcmi_comm_new(sock, tcmi_migman_deliver_msg, self, 
					 TCMI_MSG_GROUP_ANY))) {
		mdbg(ERR3, "Failed to create TCMI comm component!");
		goto exit2;
	}

	if (tcmi_migman_init_ctlfs_dirs(self, root, migproc, namefmt, args) < 0) {
		mdbg(ERR3, "Failed to create ctlfs directories!");
		goto exit3;
	}

	mdbg(INFO4, "Creating symlink: self: %p root: %p sock %p",self, root, sock);
        if (tcmi_migman_create_symlink(self, root, sock) < 0){
                mdbg(ERR3, "Failed to create ctlfs symlink!");
                goto exit4;
        }

	if (tcmi_migman_init_ctlfs_files(self) < 0) {
		mdbg(ERR3, "Failed to create ctlfs files!");
		goto exit4;
	}
	if (!(self->sock = tcmi_sock_new(self->d_conns, sock, "ctrlconn"))) {
		mdbg(ERR3, "Failed to create a TCMI socket!");
		goto exit5;
	}

	if (tcmi_migman_start_thread(self) < 0) {
		mdbg(ERR3, "Failed to create message processing thread!");
		goto exit6;
	}
	
	return 0;

	/* error handling */
 exit6:
	tcmi_sock_put(self->sock);
 exit5:
	tcmi_migman_stop_ctlfs_files(self);
 exit4:
	tcmi_migman_stop_ctlfs_dirs(self);
 exit3:
	tcmi_comm_put(self->comm);
 exit2:
	tcmi_slotvec_put(self->tasks);
 exit1:
	tcmi_slotvec_put(self->transactions);
 exit0:
	return err;
}


/** 
 * \<\<public\>\> The manager is shutdown exactly in this order:
 * - calls the free method specific to a particular migration manager
 * - stop message receiving thread
 * - destroy TCMI socket
 * - unregister all ctlfs files as we don't want anymore requests 
 * from users.
 * - unregister all ctlfs directories
 * - release the communication component
 * - release the transaction slot vector - check for stale transactions
 * - release the shadows slot vector
 *
 * @param *self - pointer to this migration manager instance
 */
void tcmi_migman_put(struct tcmi_migman *self)
{
	if (self && atomic_dec_and_test(&self->ref_count)) {
		mdbg(INFO3, "Shutting down the TCMI migration manager %p", self);
		tcmi_migman_free(self);
		kfree(self);
	}

}

/** @addtogroup tcmi_migman_class
 *
 * @{
 */

/** 
 * \<\<private\>\> Free method is has two purposes:
 * -# when the last reference is released, it will free all
 *    instance resources, except for the instance itself.
 * -# calls the child class free method if defined
 */
static void tcmi_migman_free(struct tcmi_migman *self)
{
	/* terminate the processing thread first to prevent any
	 * message processing */
	tcmi_migman_stop_thread(self);
	/* also stop receiving all messages */
	tcmi_comm_put(self->comm);
	/* now we are safe and we can start cleaning the rest */
	if (self->ops->free)
		self->ops->free(self);
	tcmi_sock_put(self->sock);
	tcmi_migman_stop_ctlfs_files(self);
	tcmi_migman_stop_ctlfs_dirs(self);
	/* check for stale transactions.. */
	tcmi_slotvec_put(self->transactions);
	/* Should migrate all tasks back!! */
	tcmi_slotvec_put(self->tasks);
}


/** 
 * \<\<private\>\> Creates the directories as described in \link
 * tcmi_migman_class here \endlink. 
 *
 * @param *self - pointer to this migration manager instance
 * @param *root - root directory where to create TCMI ctlfs entries
 * @param *migproc - directory where the migrated process will have their info
 * @param namefmt - nameformat string for the main manager directory name (printf style)
 * @param args - arguments to the format string
 * @return 0 upon success
 */
static int tcmi_migman_init_ctlfs_dirs(struct tcmi_migman *self,
				       struct tcmi_ctlfs_entry *root,
				       struct tcmi_ctlfs_entry *migproc,
				       const char namefmt[], va_list args)

{
	mdbg(INFO4, "Creating directories");

	if (!(self->d_migman = tcmi_ctlfs_dir_vnew(root, TCMI_PERMS_DIR, namefmt, args)))
		goto exit0;

	if (!(self->d_conns = 
	      tcmi_ctlfs_dir_new(self->d_migman, TCMI_PERMS_DIR, "connections")))
		goto exit1;

	if (!(self->d_migproc = tcmi_ctlfs_entry_get(migproc))) {
		mdbg(ERR3, "Missing migproc directory for migration manager!");
		goto exit2;
	}
	if (self->ops->init_ctlfs_dirs && self->ops->init_ctlfs_dirs(self)) {
		mdbg(ERR3, "Failed to create specific ctlfs directories!");
		goto exit3;
	}

	return 0;

	/* error handling */
 exit3:
	tcmi_ctlfs_entry_put(self->d_migproc);
 exit2:
	tcmi_ctlfs_entry_put(self->d_conns);
 exit1:
	tcmi_ctlfs_entry_put(self->d_migman);
 exit0:
	return -EINVAL;

}


/** 
 * \<\<private\>\> Creates the static files described \link
 * tcmi_migman_class here \endlink.
 *
 * @param *self - pointer to this migration manager instance
 * @return 0 upon success
 */
static int tcmi_migman_init_ctlfs_files(struct tcmi_migman *self)
{
	mdbg(INFO4, "Creating files");

	if (!(self->f_state = 
	      tcmi_ctlfs_strfile_new(self->d_migman, TCMI_PERMS_FILE_R,
				     self, tcmi_migman_show_state, NULL,
				     TCMI_MIGMAN_STATE_LENGTH, "state")))
		goto exit0;

	if (self->ops->init_ctlfs_files && self->ops->init_ctlfs_files(self)) {
		mdbg(ERR3, "Failed to create specific ctlfs files!");
		goto exit1;
	}

	return 0;

	/* error handling */
 exit1:
	tcmi_ctlfs_file_unregister(self->f_state);
	tcmi_ctlfs_entry_put(self->f_state);
 exit0:
	return -EINVAL;

}


/** 
 * \<\<private\>\> Destroys all TCMI ctlfs directories
 *
 * @param *self - pointer to this migration manager instance
 */
static void tcmi_migman_stop_ctlfs_dirs(struct tcmi_migman *self)
{
	mdbg(INFO3, "Destroying  TCMI migmanager ctlfs directories");

	if (self->ops->stop_ctlfs_dirs) 
		self->ops->stop_ctlfs_dirs(self);

	tcmi_ctlfs_entry_put(self->d_migman);
	tcmi_ctlfs_entry_put(self->s_migman);
	tcmi_ctlfs_entry_put(self->d_conns);
	tcmi_ctlfs_entry_put(self->d_migproc);
}

/** 
 *\<\<private\>\>  Unregisters and releases all control files.
 *
 * @param *self - pointer to this migration manager instance
 */
static void tcmi_migman_stop_ctlfs_files(struct tcmi_migman *self)
{
	mdbg(INFO3, "Destroying  TCMI migmanager ctlfs files");

	if (self->ops->stop_ctlfs_files) 
		self->ops->stop_ctlfs_files(self);

	tcmi_ctlfs_file_unregister(self->f_state);
	tcmi_ctlfs_entry_put(self->f_state);
}

/** 
 * \<\<private\>\> Read method for the TCMI ctlfs - reports migration
 * manager state.
 *
 * @param *obj - this migration manager instance
 * @param *data - pointer to this migration manager instance
 * @return 0 upon success
 */
static int tcmi_migman_show_state(void *obj, void *data)
{
	struct tcmi_migman *self = TCMI_MIGMAN(obj);
	
	mdbg(INFO3, "state %d, maxcount %d", tcmi_migman_state(self), TCMI_MIGMAN_STATE_COUNT);
	if (tcmi_migman_state(self) < TCMI_MIGMAN_STATE_COUNT) {
		strncpy((char*)data, 
			tcmi_migman_states[tcmi_migman_state(self)], 
			TCMI_MIGMAN_STATE_LENGTH);
		mdbg(INFO3, "Displaying state %s", tcmi_migman_states[tcmi_migman_state(self)]);
	}
	return 0;
}

/**
 * \<\<private\>\> Starts the listening thread. The kernel thread is
 * started by using kernel services. No additional actions are needed.
 *
 * @param *self - pointer to this migration manager instance
 * @return 0 upon sucessful thread creation 
 */
static int tcmi_migman_start_thread(struct tcmi_migman *self)
{
	int err = 0;

	self->msg_thread = kthread_run(tcmi_migman_msg_thread, self, 
				       "tcmi_migmsgd");
	
	if (IS_ERR(self->msg_thread)) {
		err = -EINVAL;
		goto exit0;
	}
	mdbg(INFO3, "Created message processing thread %p", self->msg_thread);

	/* error hanlding */
 exit0:
	return err;
}

/**
 * \<\<private\>\> The TCMI manager listening thread is to be
 * terminated by forcing a signal and waiting for it to terminate.
 * The thread notices the request from the manager and terminates.
 *
 * @param *self - pointer to this migration manager instance
 */
static void tcmi_migman_stop_thread(struct tcmi_migman *self)
{

	if (self->msg_thread) {
		mdbg(INFO3, "Stopping message processing thread");
		force_sig(SIGTERM, self->msg_thread);
		kthread_stop(self->msg_thread);
	}
}


/** 
 * \<\<private\>\> Message delivering method that is called back from
 * the receiving thread of TCMI comm component instance. This method
 * is reponsible for delivering the message reliably to its
 * destination which can be:
 *
 * - local migration manager message queue 
 * - local migration manager transaction slot vector
 * - TCMI task message queue 
 * - TCMI task transaction slot vector
 *
 * @param *obj - pointer to the migration manager instance.
 * passed to the TCMI comm component upon creation
 * @param *m - points to the message that is to be delivered.
 * @return 0 upon successful delivery
 */
static int tcmi_migman_deliver_msg(void *obj, struct tcmi_msg *m)
{
	int err = 0;
	struct tcmi_migman *self = TCMI_MIGMAN(obj);
	struct tcmi_task *task;

	if (TCMI_MSG_GROUP(tcmi_msg_id(m)) == TCMI_MSG_GROUP_PROC) {
		/* try finding the task based on PID  in the message */
		pid_t pid = tcmi_procmsg_dst_pid(TCMI_PROCMSG(m));
		mdbg(INFO3, "Process message %08x arrived as expected for PID=%d", 
		     tcmi_msg_id(m), pid);
		/* find task by PID */
		if (!(task = tcmi_taskhelper_find_by_pid(pid))) {
			mdbg(ERR2, "Can't find dest. task for msg ID=%08x", 
			     tcmi_msg_id(m));
			goto exit0;
		}
		/* Verify it belongs to us */
		if (tcmi_task_migman_id(task) != tcmi_migman_id(self)) {
			mdbg(ERR2, "Task(PID=%d, migman ID=%08x) doesn't belong"
			     "to migration manager ID=%08x", tcmi_task_local_pid(task),
			     tcmi_task_migman_id(task), tcmi_migman_id(self));
			goto exit1;
		}
		err = tcmi_task_deliver_msg(task, m);
		tcmi_task_put(task);
	}
	else {
		mdbg(INFO3, "Delivering regular migration message %08x",
		     tcmi_msg_id(m));
		err = tcmi_msg_deliver(m, &self->msg_queue, self->transactions);
	}

	return err;

	/* error handling */
 exit1:
	tcmi_task_put(task);
 exit0:
	return -EINVAL;
}

/**
 * \<\<private\>\> Message processing thread.
 * Sleeps on the message queue and notifies the migration manager
 * instance to process each incoming message.
 *
 * @param *data - migration manager instance
 * @return 0 upon successful thread termination.
 */
static int tcmi_migman_msg_thread(void *data)
{
	int err = 0;
	struct tcmi_msg *m;
	struct tcmi_migman *self = TCMI_MIGMAN(data);
	/* saves 2 derefences when calling the process msg method */
	process_msg_t *process_msg = self->ops->process_msg;

	if (!process_msg) {
		mdbg(ERR3, "Missing method to process messages!");
		goto exit0;
	}


	mdbg(INFO3, "Message processing thread up and running %p", current);
	while (!(kthread_should_stop() || signal_pending(current))) {
		if ((err = tcmi_queue_wait_on_empty_interruptible(&self->msg_queue)) < 0) {
			minfo(INFO1, "Signal arrived %d", err);
			break;
		}
		tcmi_queue_remove_entry(&self->msg_queue, m, node);
		if (m) {
			mdbg(INFO3, "Processing message..");
			process_msg(self, m);
		}
		else {
			mdbg(ERR3, "Processing thread woken up, queue is still empty..");
		}
	}
 exit0:
	/* very important to sync with the the thread that is terminating us */
	wait_on_should_stop();
	mdbg(INFO3, "Message processing thread terminating");
	return 0;
}

/** \<\<public\>\> Registers tcmi task into the migration manager tasks slotvec */
int tcmi_migman_add_task(struct tcmi_migman *self, struct tcmi_task* task) {
	u_int hash;

	hash = tcmi_task_hash(tcmi_task_local_pid(task), tcmi_slotvec_hashmask(self->tasks));

	//tcmi_slotvec_lock(self->tasks);
	if ( !tcmi_slotvec_insert_at(self->tasks, hash, &task->node) ) {
		//tcmi_slotvec_unlock(self->tasks);
		minfo(ERR3, "Can't insert task into a migration manager slot vector!");
		return -EINVAL;
	}
	//tcmi_slotvec_unlock(self->tasks);

	mdbg(INFO3, "Inserted task PID=%d into slot with hash %d", tcmi_task_local_pid(task), hash);

	return 0;
}

/** \<\<public\>\> Removes tcmi task from the migration manager tasks slotvec */
int tcmi_migman_remove_task(struct tcmi_migman *self, struct tcmi_task* task) {
	u_int hash;
	struct tcmi_slot* slot;

	hash = tcmi_task_hash(tcmi_task_local_pid(task), tcmi_slotvec_hashmask(self->tasks));
	
	//tcmi_slotvec_lock(self->tasks);
	slot = tcmi_slotvec_at(self->tasks, hash);

	if ( !slot ) {
		//tcmi_slotvec_unlock(self->tasks);
		minfo(ERR3, "Failed to remove task slotvec in migman tasks");
		return -EINVAL;
	}

	tcmi_slot_remove(slot, &task->node);
	//tcmi_slotvec_unlock(self->tasks);

	mdbg(INFO3, "Removed task PID=%d from slot with hash %d", tcmi_task_local_pid(task), hash);

	return 0;
}

int tcmi_migman_create_symlink(struct tcmi_migman *self, struct tcmi_ctlfs_entry * root, struct kkc_sock *sock)
{
        char n[KKC_SOCK_MAX_ADDR_LENGTH];
        //char * n = kmalloc(KKC_SOCK_MAX_ADDR_LENGTH,GFP_KERNEL);
        int i = KKC_SOCK_MAX_ADDR_LENGTH;
        mdbg(INFO4,"tcmi_migman_create_symlink");
        kkc_sock_getpeername(sock,n,KKC_SOCK_MAX_ADDR_LENGTH);
        while(i>-1 && n[i]!=':')i--;
        if(i>-1 && n[i] == ':')n[i]=0;
        if(!(self->s_migman=tcmi_ctlfs_symlink_new(root,self->d_migman,n))){
                return -EINVAL;
        }
        return 0;
}


/** String descriptor for the states of migration manager */
static char *tcmi_migman_states[] = {
	"Init",
	"Connected",
	"Shutting down",
	"Shutdown"
};


/**
 * @}
 */


