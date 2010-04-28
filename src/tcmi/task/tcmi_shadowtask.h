/**
 * @file tcmi_shadowtask.h - TCMI shadow task, migrated process abstraction on CCN
 *                      
 * 
 *
 *
 * Date: 04/26/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_shadowtask.h,v 1.9 2008/05/02 19:59:20 stavam2 Exp $
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
#ifndef _TCMI_SHADOWTASK_H
#define _TCMI_SHADOWTASK_H

#include "tcmi_task.h"

/** @defgroup tcmi_shadowtask_class tcmi_shadowtask class 
 * 
 * @ingroup tcmi_task_class
 *
 * This class is an abstraction for a migrated process on CCN. It
 * represents the residual dependency that the migrated process has
 * towards its originating node(CCN).  This class also does all the
 * migration work including migration back to CCN (merging the shadow
 * with the process migrating home). For this purpose, there are
 * various methods that implement various types of migration:
 * - preemptive migration using physical checkpoint image
 * (tcmi_shadowtask_*_ppm_p())
 * - preemptive migration using virtual checkpoint image
 * (tcmi_shadowtask_*_ppm_v())
 * - non-preemptive migration via execve hook
 * (tcmi_shadowtask_*_npm())
 *
 * @{
 */

/** Compound structure for the shadow task. */
struct tcmi_shadowtask {
	/** parent class instance. */
	struct tcmi_task super;
};


/** Casts to the task instance. */
#define TCMI_SHADOWTASK(t) ((struct tcmi_shadowtask*)t)

/** \<\<public\>\> Creates a new shadow task. */
extern struct tcmi_task* tcmi_shadowtask_new(pid_t local_pid, struct tcmi_migman* migman,
					     struct kkc_sock *sock, 
					     struct tcmi_ctlfs_entry *d_migproc, 
					     struct tcmi_ctlfs_entry *d_migman);



/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SHADOWTASK_PRIVATE

/** Processes a message. */
static int tcmi_shadowtask_process_msg(struct tcmi_task *self, struct tcmi_msg *m);

/** Emigrates a task to a PEN. */
static int tcmi_shadowtask_emigrate_ppm_p(struct tcmi_task *self);

/** Migrates a task back to CCN. */
static int tcmi_shadowtask_migrateback_ppm_p(struct tcmi_task *self);

/** Execve notification when merging shadow with migrating process on CCN. */
static int tcmi_shadowtask_execve(struct tcmi_task *self);

/** Handles a specified signal. */
static int tcmi_shadowtask_do_signal(struct tcmi_task *self, unsigned long signr, siginfo_t *info);

/** Verifies a successful task migration. */
static int tcmi_shadowtask_verify_migration(struct tcmi_task *self, struct tcmi_msg *resp);

/** Processes a migrate back request. */
static int tcmi_shadowtask_process_ppm_p_migr_back_guestreq_procmsg(struct tcmi_task *self, 
								   struct tcmi_msg *m);
/** Custom free method */
static void tcmi_shadowtask_free(struct tcmi_task* self);

static void tcmi_shadowtask_vfork_done(struct tcmi_task *self);

/** Return a task type */
static enum tcmi_task_type tcmi_shadowtask_get_type(void);

/** TCMI task operations that support polymorphism. */
static struct tcmi_task_ops shadowtask_ops;

#endif /* TCMI_SHADOWTASK_PRIVATE */


/**
 * @}
 */

#endif /* _TCMI_SHADOWTASK_H */

