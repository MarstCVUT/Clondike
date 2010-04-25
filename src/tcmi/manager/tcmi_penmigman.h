/**
 * @file tcmi_penmigman.h - TCMI cluster core node manager - a class that
 *                       controls the cluster core node.
 *                      
 * 
 *
 *
 * Date: 04/19/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_penmigman.h,v 1.6 2009-04-06 21:48:46 stavam2 Exp $
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
#ifndef _TCMI_PENMIGMAN_H
#define _TCMI_PENMIGMAN_H

#include <linux/wait.h>

#include "tcmi_migman.h"
#include <tcmi/migration/fs/fs_mount_params.h>

/** @defgroup tcmi_penmigman_class tcmi_penmigman class 
 *
 * @ingroup tcmi_migman_class
 * 
 * TCMI PEN migration manager controls migrating tasks for a
 * particular CCN-PEN pair and resides as the name describes - on a
 * PEN.  Basic message handling capabilities are used from the \link
 * tcmi_migman_class generic migration manager \endlink.
 * 
 * There are specific tasks that it needs to accomplish:
 * - authenticate at the CCN that it is connected to
 * - accept migrating tasks
 *
 * 
 * @{
 */


/**
 * A compound structure that holds all current shadow processes and
 * other migration related data.
 */
struct tcmi_penmigman {
	/** parent class instance. */
	struct tcmi_migman super;
	/** TCMI ctlfs - migrate-home-all file */
	struct tcmi_ctlfs_entry *f_mighome_all;

	/** Mount params to be used when starting processes from associated CCN */
	struct fs_mount_params mount_params;
};
/** Casts to the CCN migration manager. */
#define TCMI_PENMIGMAN(migman) ((struct tcmi_penmigman *)migman)


/** \<\<public\>\> TCMI PEN migration manager constructor. */
extern struct tcmi_migman* tcmi_penmigman_new(struct kkc_sock *sock, u_int32_t pen_id, struct tcmi_slot* manager_slot,
					      struct tcmi_ctlfs_entry *root,
					      struct tcmi_ctlfs_entry *migproc,
					      const char namefmt[], ...);

/** \<\<public\>\> Tries authenticating itself at the CCN that it is connected to. */
extern int tcmi_penmigman_auth_ccn(struct tcmi_penmigman *self,  int auth_data_length, char* auth_data);

/** \<\<public\>\> Getter of mount params structure. */
static inline struct fs_mount_params* tcmi_penmigman_get_mount_params(struct tcmi_penmigman *self) {
	return &self->mount_params;
};

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_PENMIGMAN_PRIVATE


/** Initializes TCMI ctlfs files. */
static int tcmi_penmigman_init_ctlfs_files(struct tcmi_migman *self);
/** Destroys all TCMI ctlfs files. */
static void tcmi_penmigman_stop_ctlfs_files(struct tcmi_migman *self);


/** Frees CCN mig. manager specific resources. */
static void tcmi_penmigman_free(struct tcmi_migman *self);

/** Processes a TCMI message m. */
static void tcmi_penmigman_process_msg(struct tcmi_migman *self, struct tcmi_msg *m);


/** CCN Migration manager operations that support polymorphism */
static struct tcmi_migman_ops penmigman_ops;

#endif /* TCMI_PENMIGMAN_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_PENMIGMAN_H */

