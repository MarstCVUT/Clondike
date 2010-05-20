/**
 * @file tcmi_guesttask.h - TCMI guesttask, migrated process abstraction on PEN
 *                      
 * 
 *
 *
 * Date: 04/26/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_guesttask.h,v 1.6 2007/10/20 14:24:20 stavam2 Exp $
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
#ifndef _TCMI_GUESTTASK_H
#define _TCMI_GUESTTASK_H

#include "tcmi_task.h"

/** @defgroup tcmi_guesttask_class tcmi_guesttask class 
 * 
 * @ingroup tcmi_task_class
 * 
 * This class is an abstraction for an 'immigrated' process on PEN. It
 * represents the migrated process, so that the TCMI framework is able
 * to control it.
 *
 * This class also does all the migration work including migration back
 * to CCN. For this purpose, there are various methods that implement 
 * various types of migration:
 * - preemptive migration using physical checkpoint image
 * (tcmi_guesttask_*_ppm_p())
 * - preemptive migration using virtual checkpoint image
 * (tcmi_guesttask_*_ppm_v())
 * - non-preemptive migration via execve hook
 * (tcmi_guesttask_*_npm())
 *
 * @{
 */

/** Compound structure for the shadow task. */
struct tcmi_guesttask {
	/** parent class instance. */
	struct tcmi_task super;

};


/** Casts to the task instance. */
#define TCMI_GUESTTASK(t) ((struct tcmi_guesttask*)t)

/** \<\<public\>\> Creates a new guest task. */
extern struct tcmi_task* tcmi_guesttask_new(pid_t local_pid, struct tcmi_migman* migman,
					   struct kkc_sock *sock, 
					   struct tcmi_ctlfs_entry *d_migproc, 
					   struct tcmi_ctlfs_entry *d_migman);


/** \<\<public\>\> Called on post-fork hook on guest task that was forked (parent) */
extern int tcmi_guesttask_post_fork(struct tcmi_task* self, struct tcmi_task* child, long fork_result, pid_t remote_child_pid);


/** \<\<public\>\> Used to set proper tid after a new process was forked.. it must be run in that process context */
extern int tcmi_guesttask_post_fork_set_tid(void *self, struct tcmi_method_wrapper *wr);

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_GUESTTASK_PRIVATE

/** Processes a message. */
static int tcmi_guesttask_process_msg(struct tcmi_task *self, struct tcmi_msg *m);

/** Emigrates a task to a PEN - PPM w/ physical ckpt image. */
//static int tcmi_guesttask_emigrate_ppm_p(struct tcmi_task *self);

/** Migrates a task back to CCN - PPM w/ physical ckpt image. */
static int tcmi_guesttask_migrateback_ppm_p(struct tcmi_task *self);

/** Exit notification for the shadow on CCN. */
static int tcmi_guesttask_exit(struct tcmi_task *self, long code);

/** Handles a specified signal. */
static int tcmi_guesttask_do_signal(struct tcmi_task *self, unsigned long signr, siginfo_t *info);

/** Processes a PPM_P migration request. */
static int tcmi_guesttask_process_p_emigrate_msg(struct tcmi_task *self, 
						    struct tcmi_msg *m);
/** Return a task type. */
static enum tcmi_task_type tcmi_guesttask_get_type(void);

/** TCMI task operations that support polymorphism */
static struct tcmi_task_ops guesttask_ops;

#endif /* TCMI_GUESTTASK_PRIVATE */


/**
 * @}
 */

#endif /* _TCMI_GUESTTASK_H */

