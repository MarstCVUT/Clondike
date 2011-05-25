/**
 * @file tcmi_penman.c - Implementation of TCMI cluster process execution
 *                       manager - a class that controls the process
 *                       execution node
 *                      
 * 
 *
 *
 * Date: 04/17/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_penman.c,v 1.5 2009-04-06 21:48:46 stavam2 Exp $
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


#include <linux/kthread.h>

#include <tcmi/migration/tcmi_migcom.h>
#include <tcmi/ctlfs/tcmi_ctlfs.h>
#include "tcmi_penmigman.h"

#include <tcmi/task/tcmi_task.h>
#include <tcmi/task/tcmi_taskhelper.h>
#include <tcmi/migration/tcmi_npm_params.h>
#include <kkc/kkc.h>

#define TCMI_PENMAN_PRIVATE
#include "tcmi_penman.h"

#include <dbg.h>

/**
 * \<\<public\>\> Singleton instance initializer.
 * The initialization is accomplished exactly in this order:
 * - delegate rest of initialization to TCMI generic manager
 *
 * @param *root - root directory of the filesystem, where all entries
 * are to be created
 * @return 0 upon success
 */
int tcmi_penman_init(struct tcmi_ctlfs_entry *root)
{
	minfo(INFO1, "Initializing TCMI PEN manager");

	if (tcmi_man_init(TCMI_MAN(&self), root, &penman_ops, "pen") < 0) {
		mdbg(ERR3, "TCMI PEN manager - generic init failed");
		goto exit0;
	}
	return 0;
		
	/* error handling */
 exit0:
	return -EINVAL;
}

/** 
 * \<\<public\>\> The manager is shutdown exactly in this order:
 * - unregister all ctlfs files as we don't want anymore requests 
 * from users.
 * - shutdown the migration framework. This includes
 * migrating all processes back and terminating all migration managers
 * - unregister all ctlfs directories
 *
 */
void tcmi_penman_shutdown(void)
{
	minfo(INFO1, "Shutting down the TCMI PEN manager");

	tcmi_man_shutdown(TCMI_MAN(&self));
}


/** @addtogroup tcmi_penman_class
 *
 * @{
 */


/** 
 * \<\<private\>\> Creates the static files 
 * described \link tcmi_penman_class here \endlink. 
 *
 * @return 0 upon success
 */
static int tcmi_penman_init_ctlfs_files(void)
{
	mdbg(INFO4, "Creating files");
	if (!(self.f_connect = 
	      tcmi_ctlfs_strfile_new(tcmi_man_root(TCMI_MAN(&self)), TCMI_PERMS_FILE_W,
				     NULL, NULL, tcmi_penman_connect,
				     KKC_MAX_WHERE_LENGTH, "connect")))
		goto exit0;
	return 0;

	/* error handling */
 exit0:
	return -EINVAL;

}


/** 
 *\<\<private\>\> Unregisters all control files.
 */
static void tcmi_penman_stop_ctlfs_files(void)
{
	mdbg(INFO3, "Destroying  TCMI PEN manager ctlfs files");

	tcmi_ctlfs_file_unregister(self.f_connect);
	tcmi_ctlfs_entry_put(self.f_connect);
}

/**
 * @return 1, if there is a duplicate address for some of the peers
 */
static int check_duplicates(const char* address, int address_len) {
    int ret = 0;
    struct tcmi_slot *slot;
    tcmi_slot_node_t* node;
    struct tcmi_migman *migman = NULL;    
    tcmi_slotvec_lock(TCMI_MAN(&self)->mig_mans);
    
    tcmi_slotvec_for_each_used_slot(slot, TCMI_MAN(&self)->mig_mans) {
	    tcmi_slot_for_each(node, slot) {		    
		    migman = tcmi_slot_entry(node, struct tcmi_migman, node);		    		    		    		    
		    
		    if ( kkc_sock_is_address_equal_to(tcmi_migman_sock(migman),address,address_len, 1) ) {
			ret = 1;
			goto found;		    
		    }		   
	    };
    };
    
found:    
    tcmi_slotvec_unlock(TCMI_MAN(&self)->mig_mans);
    return ret;
}

/**
 * \<\<private\>\> Creates a new connection and instantiates a new
 * migration manager that will be responsible for handling it.
 *
 * @param *obj - pointer to an object - NULL is expected as
 * TCMI PEN manager is a singleton class.
 * @param *str - connection string - It is architecture dependent
 * and passed further on tcmi_listen class. "@" char is used to separate address of the remote node and optional authentication data
 * @return 0 upon success
 */
static int tcmi_penman_connect(void *obj, void *str)
{
	int err, separation_index, auth_data_size = 0; 
	char *connect_str = (char *) str;
	char *auth_data = NULL;
	char *separation_char = NULL;
	struct tcmi_migman *migman;
	struct tcmi_slot *slot;
	struct kkc_sock *sock;

	mdbg(INFO3, "Connection request with param: %s", connect_str);

	// @ serves as a special separator of "node address" from "authentication data
	if ( (separation_char=strchr(connect_str, '@')) != NULL ) {
		separation_index = separation_char - connect_str;
		// TODO: Here we use strlen on auth data length calculation, which means we loose a capability of passing pure binary data.. improve it if possible
		auth_data_size = strlen(connect_str) - separation_index - 1;
		// We separate the string by zero so that it is internally threated as a short string for kkc soct
		connect_str[separation_index] = '\0';
		auth_data = connect_str + separation_index + 1;
	}			
	
	if ( check_duplicates(connect_str, strlen(connect_str)) ) {
	    mdbg(ERR3, "Duplicate peer, already connected:'%s' ", connect_str);
	    goto exit0;
	}
	  

	/* check if there is incoming connection */
	if (kkc_connect(&sock, connect_str)) {
		mdbg(ERR3, "Failed connecting to '%s' ", connect_str);
		goto exit0;
	}
	mdbg(INFO3, "Connected to local: '%s', remote: '%s', auth_data_size: %d", 
	     kkc_sock_getsockname2(sock), kkc_sock_getpeername2(sock), auth_data_size);
	/* Reserve an empty slot */
	if (!(slot = tcmi_man_reserve_migmanslot(TCMI_MAN(&self)))) {
		mdbg(ERR3, "Failed allocating an empty slot for PEN migman"); 
		goto exit1;
	}

	/* Accept the first incoming connection */
	if (!(migman = tcmi_penmigman_new(sock, 
					  tcmi_man_id(TCMI_MAN(&self)),
					  slot,
					  tcmi_man_nodes_dir(TCMI_MAN(&self)), 					  
					  tcmi_man_migproc_dir(TCMI_MAN(&self)),
					  "%d", tcmi_slot_index(slot)))) {
		minfo(ERR1, "Failed to instantiate the PEN migration manager"); 
		goto exit2;
	}
	
	if ((err = tcmi_penmigman_auth_ccn(TCMI_PENMIGMAN(migman), auth_data_size, auth_data)) < 0) {
		minfo(ERR1, "Failed to authenticate the CCN! %d", err); 
		goto exit3;
	}
	tcmi_slot_insert(slot, tcmi_migman_node(migman));

	kkc_sock_put(sock);

	return 0;
/* error handling */
 exit3:
	tcmi_migman_put(migman);
 exit2:
	tcmi_man_release_migmanslot(TCMI_MAN(&self), slot); 
 exit1:
	kkc_sock_put(sock);
 exit0:
	return -EINVAL;
}


/**
 * \<\<private\>\> Stops the PEN manager.
 *
 */
static void tcmi_penman_stop(void)
{
	/* Nothing to be done for now.. */
}

/**
 * Performs non-preemtive migration back from PEN.
 * Operates on "current" task
 */
int tcmi_penman_migrateback_npm(struct tcmi_man *self, struct pt_regs* regs, struct tcmi_npm_params* npm_params) {
	struct tcmi_task* task = current->tcmi.tcmi_task;

	BUG_ON(current->tcmi.tcmi_task == NULL);
	BUG_ON(current->tcmi.task_type != guest);

	// Submits task specific method for performing npm migback
	tcmi_task_submit_method(task, tcmi_task_migrateback_npm, npm_params, sizeof(*npm_params));
	// And enter directly to migmide as we are already in the task context
	tcmi_taskhelper_do_mig_mode(regs);

	return 0;
}

/** Singleton instance pre-initialization */
static struct tcmi_penman self = {
	.f_connect = NULL,
};


/** Migration manager operations that support polymorphism */
static struct tcmi_man_ops penman_ops = {
	.init_ctlfs_files = tcmi_penman_init_ctlfs_files, 
	.stop_ctlfs_files = tcmi_penman_stop_ctlfs_files,
	.stop = tcmi_penman_stop,
	.migrate_home_ppm_p = tcmi_migcom_migrate_home_ppm_p,
	.fork = tcmi_migcom_guest_fork,
};

/**
 * @}
 */


