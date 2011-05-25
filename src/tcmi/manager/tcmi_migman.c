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
		     enum arch_ids peer_arch_type, struct tcmi_slot* manager_slot,
		     struct tcmi_ctlfs_entry *root,
		     struct tcmi_ctlfs_entry *migproc,
		     struct tcmi_migman_ops *ops,
		     const char namefmt[], va_list args)
{
	int err = -EINVAL;

	minfo(INFO2, "Creating new TCMI migration manager");
	atomic_set(&self->state, TCMI_MIGMAN_INIT);
	self->ccn_id = ccn_id;
	self->pen_id = pen_id;
	self->manager_slot = manager_slot;
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
 * @return Returns 1, if the instance was destroyed as a result of this call
 */
int tcmi_migman_put(struct tcmi_migman *self)
{
	if (self && atomic_dec_and_test(&self->ref_count)) {
		mdbg(INFO3, "Shutting down the TCMI migration manager %p", self);
		tcmi_migman_free(self);
		kfree(self);
		
		return 1;
	}
	
	return 0;
}

/** @addtogroup tcmi_migman_class
 *
 * @{
 */

/** Removes migman from a list of migration managers of the associated manager. */
static void tcmi_migman_unhash(struct tcmi_migman *self) {
      /** Check if we were not already unhashed */
      if ( !tcmi_slot_node_unhashed(&self->node) ) {
	  tcmi_slot_lock(self->manager_slot);
	  tcmi_slot_remove_first(self, self->manager_slot, node);
	  tcmi_slot_unlock(self->manager_slot);
      }
}


/** 
 * \<\<private\>\> Free method is has two purposes:
 * -# when the last reference is released, it will free all
 *    instance resources, except for the instance itself.
 * -# calls the child class free method if defined
 *
 * Note: It is important to free communication channels first, i.e. communication socket, processing thread and files. This ensures,
 * that no new messages that may require reference to migman for processing will arrive. File operations are synchronized with releasing
 * so it may be possible that some file works with the migman during the free method, but only until the file is ''synchronously'' released.
 */
static void tcmi_migman_free(struct tcmi_migman *self)
{ 
	/* terminate the processing thread first to prevent any
	 * message processing */
	tcmi_migman_stop_thread(self);
	/* also stop receiving all messages */
	tcmi_comm_put(self->comm);

	tcmi_migman_stop_ctlfs_files(self);
	tcmi_migman_stop_ctlfs_dirs(self);
	tcmi_sock_put(self->sock);
	tcmi_migman_unhash(self);
	

	/* now we are safe and we can start cleaning the rest */
	if (self->ops->free)
		self->ops->free(self);
				
	/* TODO: check for stale transactions.. */
	tcmi_slotvec_put(self->transactions);
	/* Note: Assumes, all tasks are already migrated back. */
	tcmi_slotvec_put(self->tasks);
}

/**
 * Requests asynchronous shutdown of connection with peer.
 * First, it asynchronously requests emigration of tasks to their home node, in case we are on detached node.
 * Then it notifies peer about current node being disconnected.
 * Finally, a self reference is dropped so that the migman is free to be destroyed when all tasks are finished
 */
void tcmi_migman_stop(struct tcmi_migman *self) {
    tcmi_migman_stop_perform(self, 0);
}

static void tcmi_migman_notify_peer_disconnect(struct tcmi_migman *self) {
    struct tcmi_msg* msg;
    int err;

    if ( !( msg = TCMI_MSG(tcmi_disconnect_msg_new_tx()) ) ) { 
	    minfo(ERR3, "Error creating disconnect message");
    } else {
	if ((err = tcmi_msg_send_anonymous(msg, tcmi_migman_sock(self)))) {
		minfo(ERR3, "Error sending disconnect message %d", err);
	}
	tcmi_msg_put(msg);
    }	    
}
  
/**
 * Actual implementation of stop, see tcmi_migman_stop
 * 
 * @param remote_requested True, if stop request was initiated as a response to received disconnect message
 * @Return 1, if the migman instance was destroyed in context of this method
 */
static int tcmi_migman_stop_perform(struct tcmi_migman *self, int remote_requested) {
     minfo(INFO3, "Requesting stop of migration manager: %d Remote requested: %d", tcmi_migman_slot_index(self), remote_requested);
     
     if ( tcmi_migman_change_state(self, TCMI_MIGMAN_CONNECTED, TCMI_MIGMAN_SHUTTING_DOWN) ) {
	/* We are the first to request stop operation */
	
	if (self->ops->stop)
	    self->ops->stop(self, remote_requested);

	if ( !remote_requested ) {
	    /* Sent notification to peer about disconnection being performed.. even if this fails (peer may be dead already), we proceed further */
	    tcmi_migman_notify_peer_disconnect(self);
	}		
	
	/* Finally drop the reference to "self". If this is the last reference, the instance is going to be unhased and deleted.
	   At this phase it is possible some tasks are still running and so they have reference to this instance. Asynchronous emigration requests were already issued */
	return tcmi_migman_put(self);
     } else {
	minfo(INFO3, "Migration manager stop was already requested -> Ignoring this call. Manager state is: %d", tcmi_migman_state(self));
     }
     
     return 0;
}

/** Sends a kill signal to all tasks managed by this manager */
static void kill_all_tasks(struct tcmi_migman *self) {	
  	struct tcmi_task* task;
	struct tcmi_slot *slot;
	tcmi_slot_node_t* node;

	tcmi_slotvec_lock(self->tasks);
	/* Iterate over all managed tasks */
	tcmi_slotvec_for_each_used_slot(slot, self->tasks) {
		tcmi_slot_for_each(node, slot) {
			task = tcmi_slot_entry(node, struct tcmi_task, node);
			mdbg(INFO3, "Killing task: local_pid=%d", tcmi_task_local_pid(task));			
			tcmi_task_signal_peer_lost(task);
		};
	};
	tcmi_slotvec_unlock(self->tasks);  
}

/**
 * Kills all tasks associated with this migman and drops reference to the migman (in case it was not already shutting down).
 * The method is useful in case peer died without cleaning up its own tasks, because we need to clean them up before we can release reference to peer.
 * 
 * @return 1, if the migman instance was destroyed in context of this method
 */
int tcmi_migman_kill(struct tcmi_migman *self) {
     minfo(INFO3, "Requesting kill of migration manager: %d", tcmi_migman_slot_index(self));
         
     kill_all_tasks(self);

     if ( tcmi_migman_change_state(self, TCMI_MIGMAN_CONNECTED, TCMI_MIGMAN_SHUTTING_DOWN) ) {
	/* Stop operation was not yet requested -> request it. */
	return tcmi_migman_put(self);
     } else {
	minfo(INFO3, "Migration manager stop was already requested -> Do not perform release this call. Manager state is: %d", tcmi_migman_state(self));
     }

          
     return 0;
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

	if (!(self->f_stop = 
	      tcmi_ctlfs_intfile_new(self->d_migman, TCMI_PERMS_FILE_W,
				     self, NULL, tcmi_migman_stop_request,
				     sizeof(int), "stop")))
		goto exit1;

	if (!(self->f_kill = 
	      tcmi_ctlfs_intfile_new(self->d_migman, TCMI_PERMS_FILE_W,
				     self, NULL, tcmi_migman_kill_request,
				     sizeof(int), "kill")))
		goto exit2;

	if (self->ops->init_ctlfs_files && self->ops->init_ctlfs_files(self)) {
		mdbg(ERR3, "Failed to create specific ctlfs files!");
		goto exit3;
	}

	return 0;

	/* error handling */
 exit3:
 	tcmi_ctlfs_file_unregister(self->f_kill);
	tcmi_ctlfs_entry_put(self->f_kill); 	
 exit2:
 	tcmi_ctlfs_file_unregister(self->f_stop);
	tcmi_ctlfs_entry_put(self->f_stop); 
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

	/* We have to use non-locking unregister, because the unregister could be called from this file's write method */
	tcmi_ctlfs_file_unregister2(self->f_stop, TCMI_CTLFS_FILE_FROM_METHOD);
	tcmi_ctlfs_entry_put(self->f_stop);
	/* We have to use non-locking unregister, because the unregister could be called from this file's write method */
	tcmi_ctlfs_file_unregister2(self->f_kill, TCMI_CTLFS_FILE_FROM_METHOD);
	tcmi_ctlfs_entry_put(self->f_kill);

	tcmi_ctlfs_file_unregister(self->f_state);
	tcmi_ctlfs_entry_put(self->f_state);
}

/** 
 * \<\<private\>\> Requests asynchronous shutdown of this migration manager
 *
 * @param *obj - this migration manager instance
 * @param *data - ignored
 * @return 0 upon success
 */
static int tcmi_migman_stop_request(void *obj, void *data) {
    struct tcmi_migman *self = TCMI_MIGMAN(obj);
    tcmi_migman_stop(self);
    
    return 0;
}

/** 
 * \<\<private\>\> Requests asynchronous kill of this migration manager
 *
 * @param *obj - this migration manager instance
 * @param *data - ignored
 * @return 0 upon success
 */
static int tcmi_migman_kill_request(void *obj, void *data) {
    struct tcmi_migman *self = TCMI_MIGMAN(obj);
    tcmi_migman_kill(self);
    
    return 0;
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
		
		if ( self->msg_thread != current ) {
		  /* We can only call stop if we are not actually the thread being stopped! */
		  kthread_stop(self->msg_thread);
		}
		
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
	int migman_freed = 0;
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
			switch(tcmi_msg_id(m)) {
  			    default:
				process_msg(self, m);
				break;

			    case TCMI_DISCONNECT_MSG_ID:
			        minfo(INFO3, "Received disconnect message");
				/* 
				  Initiate shutdown upon reception of disconnect message 
				  Note: It is important not to touch self after this call and before should_stop is checked again, because self reference may be already invalid!
				*/
				migman_freed = tcmi_migman_stop_perform(self, 1);
				break;
			}
		}
		else {
			mdbg(ERR3, "Processing thread woken up, queue is still empty..");
		}
	}
 exit0:
	/* In case the migman was freed by this thread, no should_stop sync is performed */
	if ( !migman_freed ) {
	    /* We have to wait for thread terminating us as it is going to call stop at some tim */
	    wait_on_should_stop();
	}
	
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


