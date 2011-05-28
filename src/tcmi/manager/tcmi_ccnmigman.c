/**
 * @file tcmi_ccnmigman.c - Implementation of TCMI cluster core node migration
 *                          manager - a class that controls task migration on CCN
 *                      
 * 
 *
 *
 * Date: 04/19/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ccnmigman.c,v 1.5 2009-04-06 21:48:46 stavam2 Exp $
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

#include <tcmi/migration/tcmi_migcom.h>

#define TCMI_CCNMIGMAN_PRIVATE
#include "tcmi_ccnmigman.h"
#include "tcmi_ccnman.h"

#include <tcmi/comm/tcmi_authenticate_msg.h>
#include <tcmi/comm/tcmi_authenticate_resp_msg.h>
#include <arch/current/regs.h>

#include <director/director.h>

#include <dbg.h>

/** 2^8 elements in the shadow process hash */
#define SHADOW_ORDER 8

/**
 * \<\<public\>\> TCMI CCN manager constructor.
 * The initialization is accomplished exactly in this order:
 * - create new instance
 * - allocate new slot vector for shadow processes.
 * - delegates all remaining intialization work to the generic manager.
 * - authenticate the connecting PEN
 *
 * @param *sock - socket where the new PEN is registering
 * @param ccn_id - CCN identifier
 * @param *root - directory where the migration manager should create its
 * files and directories
 * @param *migproc - directory where the migrated process will have their info
 * @param namefmt - nameformat string for the main manager directory name (printf style)
 * @param ... - variable arguments
 * @return new TCMI CCN manager instance or NULL
 */
struct tcmi_migman* tcmi_ccnmigman_new(struct kkc_sock *sock, u_int32_t ccn_id, struct tcmi_slot* manager_slot,
					  struct tcmi_ctlfs_entry *root,
				       struct tcmi_ctlfs_entry *migproc,
					  const char namefmt[], ...)
{
	struct tcmi_ccnmigman *migman;
	va_list args;

	minfo(INFO2, "Creating new TCMI CCN migration manager");
	if (!(migman = TCMI_CCNMIGMAN(kmalloc(sizeof(struct tcmi_ccnmigman), 
					      GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate memory for CCM migration manager");
		goto exit0;
	}
	init_waitqueue_head(&migman->wq);
	va_start(args, namefmt);
	if (tcmi_migman_init(TCMI_MIGMAN(migman), sock, ccn_id, 0, UNKNOWN, manager_slot, root, migproc,
			     &ccnmigman_ops, namefmt, args) < 0) {
		mdbg(ERR3, "TCMI CCN migman initialization failed!");
		va_end(args);
		goto exit1;
	}
	va_end(args);
	
	return TCMI_MIGMAN(migman);

	/* error handling */
 exit1:
	kfree(migman);
 exit0:
	return NULL;
}

/**
 * \<\<public\>\> Authenticates the connecting PEN expecting a
 * registration message and processing it. Authentication is
 * considered successful if after processing the message the new
 * manager state is 'CONNECTED'.  The check is fully left upto the
 * message processing framework. All that needs to be done is wait
 * for a given time if the state changes. Of course, the connection
 * message processing wakes us up earlier when needed.
 *
 * @param *self - pointer to this migration manager instance
 * @return 0 if successfully, -EINVAL otherwise
 */
int tcmi_ccnmigman_auth_pen(struct tcmi_ccnmigman *self)
{
	int ret;
	mdbg(INFO3, "Waiting for connection authentication.. %p", self);
	ret = wait_event_interruptible_timeout(self->wq,
					       (tcmi_migman_state(TCMI_MIGMAN(self)) == 
						TCMI_MIGMAN_CONNECTED), 5*HZ); 
	mdbg(INFO3, "woke up after %d, checking authentication result", ret);
	return (tcmi_migman_state(TCMI_MIGMAN(self)) == TCMI_MIGMAN_CONNECTED ? 
		0 : -EINVAL); 
}

/** @addtogroup tcmi_ccnmigman_class
 *
 * @{
 */



/** 
 * \<\<private\>\> Creates the static files 
 * described \link tcmi_ccnmigman_class here \endlink. 
 *
 * @param *self - pointer to this migration manager instance
 * @return 0 upon success
 */
static int tcmi_ccnmigman_init_ctlfs_files(struct tcmi_migman *self)
{
	struct tcmi_ccnmigman *self_ccn = TCMI_CCNMIGMAN(self);
	mdbg(INFO4, "Creating files");
	if (!(self_ccn->f_load = 
	      tcmi_ctlfs_intfile_new(tcmi_migman_root(self), TCMI_PERMS_FILE_R,
				     self, NULL, tcmi_ccnmigman_load,
				     sizeof(int), "load")))
		goto exit0; 

	return 0;

	/* error handling */
 exit0:
	return -EINVAL;

}


/** 
 *\<\<private\>\>  Unregisters and releases all control files.
 *
 * @param *self - pointer to this migration manager instance
 */
static void tcmi_ccnmigman_stop_ctlfs_files(struct tcmi_migman *self)
{
	struct tcmi_ccnmigman *self_ccn = TCMI_CCNMIGMAN(self);
	mdbg(INFO3, "Destroying  TCMI ctlfs files - CCN migman");
	tcmi_ctlfs_file_unregister(self_ccn->f_load);
	tcmi_ctlfs_entry_put(self_ccn->f_load);
}

/**
 * \<\<private\>\> Called on stop request
 * 
 * @param *self - pointer to this migration manager instance
 */
static void tcmi_ccnmigman_stop(struct tcmi_migman *self, int remote_requested) {
    director_node_disconnected(tcmi_migman_slot_index(self), 1, remote_requested);
}


/** 
 * \<\<private\>\> Read method for the TCMI ctlfs - reports current
 * PEN load. Load is reported based on the last sent info from PEN
 *
 *
 * @param *obj - this CCN manager instance
 * @param *data - pointer to this migration manager instance
 * @return 0 upon success
 */
static int tcmi_ccnmigman_load(void *obj, void *data)
{
	return 0;
}



/** 
 * \<\<private\>\> Called upon migman destruction.
 *  *
 * @param *self - pointer to this migration manager instance
 */
static void tcmi_ccnmigman_free(struct tcmi_migman *self)
{
	/* struct tcmi_ccnmigman *self_ccn  = TCMI_CCNMIGMAN(self); */
	/* nothing to do */
}


/**
 * \<\<private\>\> Processes a TCMI message m. 
 * The message is desposed afterwords.
 *
 * @param *self - pointer to this migration manager instance
 * @param *m - pointer to a message to be processed
 */
static void tcmi_ccnmigman_process_msg(struct tcmi_migman *self, struct tcmi_msg *m)
{
	struct tcmi_msg *resp;
	int result_code = 0;
	int accepted;
	struct tcmi_authenticate_msg *auth_msg;
	struct tcmi_generic_user_msg *user_msg;

	/* struct tcmi_ccnmigman *self_ccn = TCMI_CCNMIGMAN(self); */
	int err;
	mdbg(INFO4, "Processing message ID");

	switch(tcmi_msg_id(m)) {
	case TCMI_AUTHENTICATE_MSG_ID:
		auth_msg = TCMI_AUTHENTICATE_MSG(m);
		minfo(INFO1, "Authentication message arrived.");
		
		// Inform director that the node was connected 
		err = director_node_connected(kkc_sock_getpeername2(tcmi_migman_sock(self)), tcmi_migman_slot_index(self), tcmi_authenticate_msg_auth_data_size(auth_msg), tcmi_authenticate_msg_auth_data(auth_msg), &accepted);
		// The check for accept is performed only if the director responded, otherwise the call is ignored
		if ( !err && !accepted ) {
			minfo(ERR1, "Director rejected peer connection");
			break;
		}

		self->pen_id = tcmi_authenticate_msg_pen_id(auth_msg);
		self->peer_arch_type = tcmi_authenticate_msg_arch(auth_msg);

		if ( !( resp = tcmi_authenticate_resp_msg_new_tx(tcmi_msg_req_id(m), self->ccn_id, ARCH_CURRENT, result_code, tcmi_ccnman_get_mount_params()) ) ) { 
			mdbg(ERR3, "Error creating response message");
			break;
		}
		if ((err = tcmi_msg_send_anonymous(resp, tcmi_migman_sock(self)))) {
			mdbg(ERR3, "Error sending response message %d", err);
		}
		tcmi_msg_put(resp);
		tcmi_migman_set_state(self, TCMI_MIGMAN_CONNECTED);
		wake_up(&(TCMI_CCNMIGMAN(self)->wq));
		break;
	case TCMI_GENERIC_USER_MSG_ID:
		user_msg = TCMI_GENERIC_USER_MSG(m);
		minfo(INFO1, "User message arrived.");

		err = director_generic_user_message_recv(tcmi_generic_user_msg_node_id(user_msg), 1, tcmi_migman_slot_index(self), tcmi_generic_user_msg_user_data_size(user_msg), tcmi_generic_user_msg_user_data(user_msg));
		if ( err ) {
			mdbg(ERR3, "Error in processing user message %d", err);
		}
		break;
	default:
		minfo(INFO1, "Unexpected message ID %x, no handler available.", 
		      tcmi_msg_id(m));
		break;
	}

	/* release the message */
	tcmi_msg_put(m);

}




/** Migration manager operations that support polymorphism */
static struct tcmi_migman_ops ccnmigman_ops = {
	.init_ctlfs_files = tcmi_ccnmigman_init_ctlfs_files, 
	.stop_ctlfs_files = tcmi_ccnmigman_stop_ctlfs_files,
	.stop = tcmi_ccnmigman_stop,
	.free = tcmi_ccnmigman_free,
	.process_msg = tcmi_ccnmigman_process_msg,
};

/**
 * @}
 */


