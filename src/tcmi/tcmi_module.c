/**
 * @file tcmi_module.c - main TCMI module
 *                      - on module init:
 *                         initializes the TCMI CCN manager component
 *                         initializes the TCMI PEN manager component
 *                      - on module unload: 
 *                         shutsdown the TCMI CCN manager component
 *                         shutsdown the TCMI PEN manager component
 *
 * Date: 05/05/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_module.c,v 1.4 2008/05/02 19:59:20 stavam2 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2005  Jan Capek
 * 
 * TCMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/module.h>

#include <tcmi/ctlfs/tcmi_ctlfs.h>
#include <tcmi/manager/tcmi_ccnman.h>
#include <tcmi/manager/tcmi_penman.h>
#include <tcmi/task/tcmi_taskhelper.h>

#include <tcmi/syscall/tcmi_syscallhooks.h>
#include <tcmi/migration/tcmi_mighooks.h>

#include <director/director.h>

#include <dbg.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek, Petr Malat");

/** @defgroup tcmi_module_class TCMI
 *
 * This \<\<singleton\>\> class is the base of the infrastructure and
 * is responsible for initialization/shutdown of TCMI. The
 * initializition requires notifying CCN and/or PEN manager resp. 
 *
 * Initialized TCMI provides control in user space to:
 * - migrate a process from a CCN to a PEN
 * - migrate a process home from a PEN to a CCN
 * - display information about migrated processes
 * - connect a PEN to CCN
 * - setup interfaces where CCN listens for incoming connections from
 * PEN's
 * - shutdown the whole infrastructure
 * @{
 */


/** Ctl FS file for setting/reading of debug settings */
static struct tcmi_ctlfs_entry *debug_file = NULL;

/**
 * Method to set debug flag via a ctlfs file
 */
static int tcmi_debug_set(void *obj, void *data)
{
	int new_debug = *((int *)data);

	minfo(INFO3, "Setting TCMI debug to %d", new_debug);

	tcmi_dbg = new_debug;
	return 0;
}

 
/**
 * Reader of debug state from ctlfs file
 */
static int tcmi_debug_get(void *obj, void *data)
{
	*((int *)data) = tcmi_dbg;
	return 0;
}


static int tcmi_send_generic_user_message_handler(int is_core_slot, int target_slot_index, int user_data_size, char* user_data) {
	if ( is_core_slot ) {
		return tcmi_man_send_generic_user_message(TCMI_MAN(tcmi_ccnman_get_instance()), target_slot_index, user_data_size, user_data);
	} else {
		return tcmi_man_send_generic_user_message(TCMI_MAN(tcmi_penman_get_instance()), target_slot_index, user_data_size, user_data);
	}
}

static void tcmi_register_director_user_message_handler(void) {
	director_register_send_generic_user_message_handler(tcmi_send_generic_user_message_handler);
}


/**
 * Module initialization.
 * Initializes 3 core components:
 * - tcmi hooks in kernel
 * - CCN manager - (when enabled in kernel)
 * - PEN manager - (when enabled in kernel)
 *
 * The resulting node setup depends how TCMI has been configured in
 * kernel.
 *
 * @return 0 upon successfull initialization
 */
static int __init tcmi_module_init(void)
{
	struct tcmi_ctlfs_entry *root;
	minfo(INFO1, "Loading the TCMI framework");
	tcmi_dbg = 1; // Enable debug temporarily at least for startup time

 	root = tcmi_ctlfs_get_root();

	tcmi_mighooks_init();
	tcmi_syscall_hooks_init();
	tcmi_register_director_user_message_handler();

	if (tcmi_ccnman_init(root) < 0) {
		minfo(ERR1, "Failed initializing TCMI CCN manager");
		goto exit0;
	}
	if (tcmi_penman_init(root) < 0) {
		minfo(ERR1, "Failed initializing TCMI PEN manager");
		goto exit1;
	}

        debug_file = tcmi_ctlfs_intfile_new(root, TCMI_PERMS_FILE_RW,
			     NULL, tcmi_debug_get, tcmi_debug_set,
			     sizeof(int), "debug");
	if ( !debug_file ) {
		minfo(ERR1, "Failed to create debug control file");
		goto exit2;
	}


	minfo(INFO1, "TCMI framework loaded");
	tcmi_dbg = 0; // Startup succeeded, disable debug, it can be reenabled via ctlfs
	return 0;

	/* error handling */
 exit2:
	tcmi_penman_shutdown();
 exit1:
	tcmi_ccnman_shutdown();
 exit0:
	return -EINVAL;
}

/**
 * Module cleanup
 */
static void __exit tcmi_module_exit(void)
{

	minfo(INFO1, "Unloading TCMI framework");

	/* Managers must be shutdown first, because they may need other parts of infrastructure to proceed with migration back */
	tcmi_penman_shutdown();
	tcmi_ccnman_shutdown();
	
	tcmi_mighooks_exit();
	tcmi_syscall_hooks_exit();

	tcmi_ctlfs_file_unregister(debug_file);
	tcmi_ctlfs_entry_put(debug_file);

	tcmi_ctlfs_put_root();

	minfo(INFO1, "TCMI framework unloaded");
}
 /**
 * @}
 */


module_init(tcmi_module_init);
module_exit(tcmi_module_exit);



