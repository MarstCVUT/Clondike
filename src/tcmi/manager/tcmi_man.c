/**
 * @file tcmi_man.c - TCMI cluster node manager - a template class
 *                    that is further implemented by CCN/PEN
 *                      
 * 
 *
 *
 * Date: 05/05/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_man.c,v 1.6 2007/10/20 14:24:20 stavam2 Exp $
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

#include <tcmi/ctlfs/tcmi_ctlfs.h>
#include <tcmi/lib/tcmi_sock.h>
#include <tcmi/task/tcmi_task.h>
#include "tcmi_ccnmigman.h"

#include <kkc/kkc.h>

#define TCMI_MAN_PRIVATE
#include "tcmi_man.h"

#include <dbg.h>

/** maximum number of connected nodes */
#define MAX_NODES 40

/** 
 * \<\<public\>\> Initializes the TCMI manager.
 * This generic init method, initializes basic singleton instance items
 * common for both TCMI managers (CCN and PEN). The initialization is
 * done in this order:
 * - create a slot vector for migration managers
 * - create TCMI ctlfs directories 
 * - create TCMI ctlfs control files
 *
 *
 * @param *self - a particular singleton instance to be initialized
 * @param *root - root directory of the filesystem, where all entries
 * are to be created
 * @param *ops - instance specific operations
 * @param *man_dirname - name for the main directory of the manager (e.g. ccn or pen)
 * also represent connected nodes (e.g. all PEN's from  CCN prospective)
 * @return 0 upon success
 */
int tcmi_man_init(struct tcmi_man *self,  
		  struct tcmi_ctlfs_entry *root,
		  struct tcmi_man_ops *ops,
		  const char *man_dirname)
{
	if (!ops) {
		mdbg(ERR3, "Missing migration manager operations!");
		goto exit0;
	}
	atomic_set(&self->ready, 0);
	
	self->ops = ops;

	if (!(self->mig_mans = tcmi_slotvec_new(MAX_NODES)))
		goto exit0;
	if (tcmi_man_init_ctlfs_dirs(self, root, man_dirname) < 0)
		goto exit1;
	if (tcmi_man_init_migration(self) < 0)
		goto exit2;
	if (tcmi_man_init_ctlfs_files(self) < 0)
		goto exit3;
	atomic_set(&self->ready, 1);
	get_random_bytes(&self->id, sizeof(u_int32_t));
	return 0;
		
	/* error handling */
 exit3:
	tcmi_man_stop_migration(self);
 exit2:
	tcmi_man_stop_ctlfs_dirs(self);
 exit1:
	tcmi_slotvec_put(self->mig_mans);
 exit0:
	return -EINVAL;
}

/**
 * Waits for all migration managers to terminate.
 *
 * Note: We perform wait by active polling of count of remaining migmas (100ms) interval. 
 * 	 In case there were no remote processes running, there would be no wait as all migmans were terminated synchronously.
 * 	 In case there were remote processes, the waiting time for their migration back would be likely in order of seconds, so 100ms does not add much latency
 *	 In case somebody volunteers, the code could be migrated to notify/wait patter to get rid of polling
 */
static void tcmi_man_wait_for_migmans_termination(struct tcmi_man *self) {
	int is_empty;
	
	tcmi_slotvec_lock(self->mig_mans);
	is_empty = tcmi_slotvec_empty(self->mig_mans);
	tcmi_slotvec_unlock(self->mig_mans);
		
	while (!is_empty) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(100)); 		
	  
	  	tcmi_slotvec_lock(self->mig_mans);
		is_empty = tcmi_slotvec_empty(self->mig_mans);
		tcmi_slotvec_unlock(self->mig_mans);
	}
}

/** 
 * \<\<public\>\> The manager is shutdown exactly in this order:
 * - unregister all ctlfs files as we don't want anymore requests 
 * from users.
 *
 * - shutdown the migration framework. This requires terminating all
 * migration managers(results in termination or migration back of all 
 * processes)
 * - unregister all ctlfs directories
 *
 * @param *self - a particular manager singleton instance to be shutdown
 */
void tcmi_man_shutdown(struct tcmi_man *self)
{
	if ( atomic_cmpxchg(&self->ready, 1, 0) == 1 ) {
		tcmi_man_stop_ctlfs_files(self);
		tcmi_man_stop_ctlfs_dirs(self);
		/* Request async mig mans shutdown */
		tcmi_man_stop_migration(self);
		/* Wait for migmans to terminate */
		tcmi_man_wait_for_migmans_termination(self);
		tcmi_slotvec_put(self->mig_mans);
	}
}

static inline int tcmi_man_do_send_generic_user_message(struct tcmi_man *self, int target_slot_index, int user_data_size, char* user_data, struct tcmi_migman *migman) {
	int err = 0;
	struct tcmi_msg* msg;

	// TODO: Node id is passed incorrectly, but on the other hand this node id is complete bullshit and should be likely removed, right?
	if ( !( msg = TCMI_MSG(tcmi_generic_user_msg_new_tx(self->id, user_data, user_data_size)) ) ) { 
		mdbg(ERR3, "Error creating response message");
		return - ENOMEM;		;
	}
	if ((err = tcmi_msg_send_anonymous(msg, tcmi_migman_sock(migman)))) {
		mdbg(ERR3, "Error sending user message %d", err);
		err = -EINVAL;
	}
	tcmi_msg_put(msg);

	return err;
}

int tcmi_man_send_generic_user_message(struct tcmi_man *self, int target_slot_index, int user_data_size, char* user_data) {
	int err;
	struct tcmi_slot *slot;
	struct tcmi_migman *migman = NULL;

	/* lookup the migration manager first */
	tcmi_slotvec_lock(self->mig_mans);
	if (!(slot = tcmi_slotvec_at(self->mig_mans, target_slot_index))) {
		mdbg(ERR3, "Invalid migration manager identifier %d", target_slot_index);
		err = -ENOENT;
		goto exit0;
	}
	/* there is always only one mig man per slot */
	tcmi_slot_find_first(migman, slot, node);
	/* Slot might exist, but it might still be empty*/
	if (!tcmi_migman_get(migman)) {
		mdbg(ERR3, "No migration manager in the slot(%d)", tcmi_slot_index(slot));
		err = -ENOENT;
		goto exit0;
	}
	tcmi_slotvec_unlock(self->mig_mans);

	err = tcmi_man_do_send_generic_user_message(self, target_slot_index, user_data_size, user_data, migman);

	/* release the migration manager */
	tcmi_migman_put(migman);

	return err;

	/* error handling */
 exit0:
	tcmi_slotvec_unlock(self->mig_mans);
	return err;
}

/** @addtogroup tcmi_man_class
 *
 * @{
 */


/** 
 * \<\<private\>\> Creates the directories as described in \link
 * tcmi_man_class here \endlink. It is necessary to get the current
 * root directory of the TCMI ctlfs first. So that it won't get
 * unlinked under us.
 *
 * @param *self - a particular manager singleton instance
 * @param *root - root directory of the filesystem, where all entries
 * are to be created
 * @param *man_dirname - name for the main directory of the manager (e.g. ccn or pen)
 * @return 0 upon success
 */

static int tcmi_man_init_ctlfs_dirs(struct tcmi_man *self,  
				    struct tcmi_ctlfs_entry *root,
				    const char *man_dirname)
{
	mdbg(INFO4, "Creating directories");
	if (!root)
		goto exit0;

	if (!(self->d_man = tcmi_ctlfs_dir_new(root, TCMI_PERMS_DIR, man_dirname)))
		goto exit0;

	if (!(self->d_nodes = 
	      tcmi_ctlfs_dir_new(self->d_man, TCMI_PERMS_DIR, "nodes")))
		goto exit1;

	if (!(self->d_mig = 
	      tcmi_ctlfs_dir_new(self->d_man, TCMI_PERMS_DIR, "mig")))
		goto exit2;

	if (!(self->d_migproc = 
	      tcmi_ctlfs_dir_new(self->d_mig, TCMI_PERMS_DIR, "migproc")))
		goto exit3;

	if (self->ops->init_ctlfs_dirs && self->ops->init_ctlfs_dirs()) {
		mdbg(ERR3, "Failed to create specific ctlfs directories!");
		goto exit4;
	}
	return 0;

	/* error handling */
 exit4:
	tcmi_ctlfs_entry_put(self->d_migproc);
 exit3:
	tcmi_ctlfs_entry_put(self->d_mig);
 exit2:
	tcmi_ctlfs_entry_put(self->d_nodes);
 exit1:
	tcmi_ctlfs_entry_put(self->d_man);
 exit0:
	return -EINVAL;

}
/** 
 * \<\<private\>\> Creates the static files 
 * described \link tcmi_man_class here \endlink. 
 *
 * @param *self - a particular manager singleton instance
 * @return 0 upon success
 */
static int tcmi_man_init_ctlfs_files(struct tcmi_man *self)
{
	mdbg(INFO4, "Creating files");

	if (!(self->f_stop = 
	      tcmi_ctlfs_intfile_new(self->d_man, TCMI_PERMS_FILE_W,
				     self, NULL, tcmi_man_stop,
				     sizeof(int), "stop")))
		goto exit0;

	if (!(self->f_emig_ppm_p = 
	      tcmi_ctlfs_intfile_new(self->d_mig, TCMI_PERMS_FILE_W,
				     self, NULL, tcmi_man_emig_ppm_p,
				     sizeof(int) * 2, "emigrate-ppm-p")))
		goto exit1;
	if (!(self->f_mig_home_ppm_p = 
	      tcmi_ctlfs_intfile_new(self->d_mig, TCMI_PERMS_FILE_W,
				     self, NULL, tcmi_man_mig_home_ppm_p,
				     sizeof(int), "migrate-home")))
		goto exit2;
	if (self->ops->init_ctlfs_files && self->ops->init_ctlfs_files()) {
		mdbg(ERR3, "Failed to create specific ctlfs files!");
		goto exit3;
	}
	return 0;


	/* error handling */
 exit3:
	tcmi_ctlfs_file_unregister(self->f_mig_home_ppm_p);
	tcmi_ctlfs_entry_put(self->f_mig_home_ppm_p);
 exit2:
	tcmi_ctlfs_file_unregister(self->f_emig_ppm_p);
	tcmi_ctlfs_entry_put(self->f_emig_ppm_p);
 exit1:
	tcmi_ctlfs_file_unregister(self->f_stop);
	tcmi_ctlfs_entry_put(self->f_stop);
 exit0:
	return -EINVAL;

}

/** 
 * \<\<private\>\> Destroys all ctlfs directories, first
 * the singleton instance specific directories are destroyed
 *
 * @param *self - a particular manager singleton instance
 */
static void tcmi_man_stop_ctlfs_dirs(struct tcmi_man *self)
{
	mdbg(INFO3, "Destroying  TCMI manager ctlfs directories");

	if (self->ops->stop_ctlfs_dirs) 
		self->ops->stop_ctlfs_dirs();
	tcmi_ctlfs_entry_put(self->d_migproc);
	tcmi_ctlfs_entry_put(self->d_mig);
	tcmi_ctlfs_entry_put(self->d_nodes);
	tcmi_ctlfs_entry_put(self->d_man);
}

/** 
 *\<\<private\>\>  Unregisters and releases all control files.
 *
 * @param *self - a particular manager singleton instance
 */
static void tcmi_man_stop_ctlfs_files(struct tcmi_man *self)
{
	mdbg(INFO3, "Destroying  TCMI manager ctlfs files");

	if (self->ops->stop_ctlfs_files) 
		self->ops->stop_ctlfs_files();

	tcmi_ctlfs_file_unregister(self->f_stop);
	tcmi_ctlfs_entry_put(self->f_stop);

	tcmi_ctlfs_file_unregister(self->f_emig_ppm_p);
	tcmi_ctlfs_entry_put(self->f_emig_ppm_p);

	tcmi_ctlfs_file_unregister(self->f_mig_home_ppm_p);
	tcmi_ctlfs_entry_put(self->f_mig_home_ppm_p);
}

/** 
 * <\<private\>\>.
 *
 * @param *self - a particular manager singleton instance
 * @return 0 upon success
 */
static int tcmi_man_init_migration(struct tcmi_man *self)
{
	mdbg(INFO3, "Initializing migration components");
	return 0;
}


/** Finds and returns arbitrary migration manager that is either in INIT or CONNECTED state. */
static struct tcmi_migman * tcmi_man_get_non_shutdowning_migman(struct tcmi_man *self) {
    struct tcmi_slot *slot;
    tcmi_slot_node_t* node;
    struct tcmi_migman *migman = NULL;
    tcmi_migman_state_t migman_state;

    tcmi_slotvec_lock(self->mig_mans);
    
    tcmi_slotvec_for_each_used_slot(slot, self->mig_mans) {
	    tcmi_slot_for_each(node, slot) {		    
		    migman = tcmi_slot_entry(node, struct tcmi_migman, node);
		    migman_state = tcmi_migman_state(migman);
		   
		    if ( migman_state == TCMI_MIGMAN_INIT || migman_state == TCMI_MIGMAN_CONNECTED ) {
			goto found;		    
		    }		   
		    migman = NULL;
	    };
    };
    
found:    
    tcmi_slotvec_unlock(self->mig_mans);   
    return migman;
}

/**
 * \<\<private\>\> Stops all migration components.  Terminates all
 * migration managers, they are responsible for migrating all
 * processes to the home node or a another execution node.
 *
 * @param *self - a particular manager singleton instance
 */
static void tcmi_man_stop_migration(struct tcmi_man *self)
{
	struct tcmi_migman *migman = tcmi_man_get_non_shutdowning_migman(self);
	while (migman) {	  
		mdbg(INFO3, "Requesting migration manager shutdown.");
		tcmi_migman_stop(migman);
		
		migman = tcmi_man_get_non_shutdowning_migman(self);
	}
}

/**
 * \<\<private\>\> If the user called this method with parameter > 0,
 * the TCMI manager stops the migration
 * framework.  This results in migrating all processes back to CCN and
 * terminating all migration managers.
 *
 * @param *obj - pointer to a particular TCMI manager singleton instance
 * @param *data - if the value that it points to is >0, the action is 
 * taken
 * @return always 0 (success)
 *
 * NOTE: What is a purpose of this method? There is no inverse operation to stop, so is that essentially different from shutdown? ms
 */
static int tcmi_man_stop(void *obj, void *data)
{  
	int stop = *((int *)data);
	struct tcmi_man *self = TCMI_MAN(obj);

	mdbg(INFO3, "TCMI manager stop request, action = %d", stop);
	if (stop) {
		/* TODO: Stop operations are called just here, but shouldn't we call them as well on shutdown? */
		if (self->ops->stop) 
			self->ops->stop();
		tcmi_man_stop_migration(self);
	}
	return 0;
}

/** 
 * \<\<private\>\> TCMI ctlfs write method - emigration PPM P 
 * What needs to be done: 
 * - find the requested migration manager
 * - call instance specific emigration method.
 *
 * @param *obj - pointer to a particular TCMI manager singleton instance
 * @param *data - an array of two integers first contains PID, second
 * one contains the desired manager identifier as it appears in 
 * ctlfs. It is the name of its directory.
 * @return 0 upon success
 */
static int tcmi_man_emig_ppm_p(void *obj, void *data)
{
	int err = 0;
	pid_t pid = *((int *)data);
	u_int32_t migman_id = *(((int *)data) + 1);
	struct tcmi_man *self = TCMI_MAN(obj);
	struct tcmi_slot *slot;
	struct tcmi_migman *migman = NULL;

	
	mdbg(INFO2, "Emigration request PID %d, migration manager %d", 
	     pid, migman_id);
	
	/* lookup the migration manager first */
	tcmi_slotvec_lock(self->mig_mans);
	if (!(slot = tcmi_slotvec_at(self->mig_mans, migman_id))) {
		mdbg(ERR3, "Invalid migration manager identifier %d", migman_id);
		goto exit0;
	}
	/* there is always only one mig man per slot */
	tcmi_slot_find_first(migman, slot, node);
	/* Slot might exist, but it might still be empty*/
	if (!tcmi_migman_get(migman)) {
		mdbg(ERR3, "No migration manager in the slot(%d)", tcmi_slot_index(slot));
		goto exit0;
	}
	tcmi_slotvec_unlock(self->mig_mans);

	if (self->ops->emigrate_ppm_p)
		err = self->ops->emigrate_ppm_p(pid, migman);

	/* release the migration manager */
	tcmi_migman_put(migman);

	return err;

	/* error handling */
 exit0:
	tcmi_slotvec_unlock(self->mig_mans);
	return -EINVAL;
}

/** 
 * \<\<public\>\> Method that performs non-preemtive migration  
 *
 * @param pid Pid of the process to be migrated
 * @param migman_id Id of the migration manager to be used for the migration
 * @param npm_params Params of the non-preemptive migration
 */
int tcmi_man_emig_npm(struct tcmi_man *self, pid_t pid, u_int32_t migman_id, struct pt_regs* regs, struct tcmi_npm_params* npm_params)
{
	int err = 0;
	struct tcmi_slot *slot;
	struct tcmi_migman *migman = NULL;
	
	mdbg(INFO2, "Non preemptive emigration request PID %d, migration manager %d", 
	     pid, migman_id);
	
	/* lookup the migration manager first */
	tcmi_slotvec_lock(self->mig_mans);
	if (!(slot = tcmi_slotvec_at(self->mig_mans, migman_id))) {
		mdbg(ERR3, "Invalid migration manager identifier %d", migman_id);
		goto exit0;
	}
	/* there is always only one mig man per slot */
	tcmi_slot_find_first(migman, slot, node);
	/* Slot might exist, but it might still be empty*/
	if (!tcmi_migman_get(migman)) {
		mdbg(ERR3, "No migration manager in the slot(%d)", tcmi_slot_index(slot));
		goto exit0;
	}
	tcmi_slotvec_unlock(self->mig_mans);

	if (self->ops->emigrate_npm)
		err = self->ops->emigrate_npm(pid, migman, regs, npm_params);

	/* release the migration manager */
	tcmi_migman_put(migman);

	return err;

	/* error handling */
 exit0:
	tcmi_slotvec_unlock(self->mig_mans);
	return -EINVAL;
}

int tcmi_man_fork(struct tcmi_man *self, struct task_struct* parent, struct task_struct* child) {
	int err = 0;
	struct tcmi_slot *slot;
	struct tcmi_migman *migman = NULL;
	u_int migman_slot_index;
	struct tcmi_task* tcmi_parent;
	unsigned long flags;

	// We perform all manager fork operation with interrupts disabled to prevent possible lock inversion problems
	// Not sure what the intention was, but it does not work well.. interfere with getinode on ccfs initialization 
	// (there are actually more interferences and we ideally need to get rid of getinode in this call chain altoghether)
//	raw_local_irq_save(flags);

	tcmi_parent = TCMI_TASK(parent->tcmi.tcmi_task);
	migman_slot_index = tcmi_task_migman_slot_index(tcmi_parent);

	BUG_ON(parent->tcmi.task_type != shadow && parent->tcmi.task_type != guest);
	BUG_ON(tcmi_parent->migman->ccn_id != self->id && tcmi_parent->migman->pen_id != self->id);

	mdbg(INFO2, "Performing fork of task pid: %d to child: %d (mig manager %u)", parent->pid, child->pid, migman_slot_index);

	if ( child->tcmi.tcmi_task != parent->tcmi.tcmi_task ) { // This should hold, since the task struts were simply copied		
		mdbg(ERR3, "Assertion error! Tasks after fork should have same tcmi task");
		return -EINVAL;
	}

	// We've verified the task is really copied and now we need to prepare it to be attached.. for that we need to reset its TCMI structures first
	child->tcmi.tcmi_task = NULL;
	child->tcmi.mig_mode_handler = NULL;
	
	/* lookup the migration manager first */
	tcmi_slotvec_lock(self->mig_mans);
	if (!(slot = tcmi_slotvec_at(self->mig_mans, migman_slot_index))) {
		mdbg(ERR3, "Invalid migration manager identifier %lu", (unsigned long)migman_slot_index);
		goto exit0;
	}
	/* there is always only one mig man per slot */
	tcmi_slot_find_first(migman, slot, node);
	/* Slot might exist, but it might still be empty*/
	if (!tcmi_migman_get(migman)) {
		mdbg(ERR3, "No migration manager in the slot(%d)", tcmi_slot_index(slot));
		goto exit0;
	}
	tcmi_slotvec_unlock(self->mig_mans);

	
	if (self->ops->fork)
		err = self->ops->fork(parent, child, migman);

//	raw_local_irq_restore(flags);
	/* release the migration manager, task has now its own reference to it */
	tcmi_migman_put(migman);	

	return err;

	/* error handling */
exit0:
	raw_local_irq_restore(flags);
	tcmi_slotvec_unlock(self->mig_mans);
	return -EINVAL;
}

/** 
 * \<\<private\>\> TCMI ctlfs write method - migration home.
 *
 * Migration home requires delegating work to the instance specific method
 * once we extract the requested PID.

 * @param *obj - pointer to a particular TCMI manager singleton instance
 * @param *data - contains PID that is to be migrated home
 * @return 0 upon success
 */
static int tcmi_man_mig_home_ppm_p(void *obj, void *data)
{
	int err = 0;
	pid_t pid = *((int *)data);
	struct tcmi_man *self = TCMI_MAN(obj);
	
	mdbg(INFO2, "Migration home request for PID %d", pid);
	

	if (self->ops->migrate_home_ppm_p)
		err = self->ops->migrate_home_ppm_p(pid);

	return err;
}


/**
 * @}
 */


