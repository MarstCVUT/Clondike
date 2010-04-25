/**
 * @file tcmi_ccnmigman.h - TCMI cluster core node manager - a class that
 *                       controls the cluster core node.
 *                      
 * 
 *
 *
 * Date: 04/19/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ccnmigman.h,v 1.3 2007/10/20 14:24:20 stavam2 Exp $
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
#ifndef _TCMI_CCNMIGMAN_H
#define _TCMI_CCNMIGMAN_H

#include <linux/wait.h>

#include "tcmi_migman.h"

/** @defgroup tcmi_ccnmigman_class tcmi_ccnmigman class 
 *
 * @ingroup tcmi_migman_class
 *
 * TCMI CCN migration manager controls migrating tasks for a
 * particular CCN-PEN pair and resides as the name describes - on a
 * CCN.  Basic message handling capabilities are used from the \link
 * tcmi_migman_class generic migration manager \endlink.
 * 
 * There are specific tasks that needs to be accomplished:
 * - authenticate a connecting PEN
 *
 * It provides an additional entry in the TCMI ctlfs. This file reports
 * an average load of a PEN that is connected to this migration manager.
 * 
 * @{
 */


/**
 * A compound structure that holds all current shadow processes and
 * other migration related data.
 */
struct tcmi_ccnmigman {
	/** parent class instance. */
	struct tcmi_migman super;

	/** TCMI ctlfs - reports current load of the remote PEN. */
	struct tcmi_ctlfs_entry *f_load;

	/** General purpose wait queue, e.g. used when syncing the
	 * context performing authentication with authentication
	 * processing method */
	wait_queue_head_t wq;
};
/** Casts to the CCN migration manager. */
#define TCMI_CCNMIGMAN(migman) ((struct tcmi_ccnmigman *)migman)


/** \<\<public\>\> TCMI CCN migration manager constructor. */
extern struct tcmi_migman* tcmi_ccnmigman_new(struct kkc_sock *sock, u_int32_t ccn_id, struct tcmi_slot* manager_slot,
					      struct tcmi_ctlfs_entry *root,
					      struct tcmi_ctlfs_entry *migproc,
					      const char namefmt[], ...);

/** \<\<public\>\> Tries authenticating a PEN. */
extern int tcmi_ccnmigman_auth_pen(struct tcmi_ccnmigman *self);

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CCNMIGMAN_PRIVATE


/** Initializes TCMI ctlfs files. */
static int tcmi_ccnmigman_init_ctlfs_files(struct tcmi_migman *self);
/** Destroys all TCMI ctlfs files. */
static void tcmi_ccnmigman_stop_ctlfs_files(struct tcmi_migman *self);


/** Read method for the TCMI ctlfs - reports current PEN load. */
static int tcmi_ccnmigman_load(void *obj, void *data);

/** Frees CCN mig. manager specific resources. */
static void tcmi_ccnmigman_free(struct tcmi_migman *self);

/** Processes a TCMI message m. */
static void tcmi_ccnmigman_process_msg(struct tcmi_migman *self, struct tcmi_msg *m);

/** CCN Migration manager operations that support polymorphism */
static struct tcmi_migman_ops ccnmigman_ops;

#endif /* TCMI_CCNMIGMAN_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_CCNMIGMAN_H */

