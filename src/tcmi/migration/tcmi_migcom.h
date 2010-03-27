/**
 * @file tcmi_migcom.h - TCMI migration component.
 *                      
 * 
 *
 *
 * Date: 05/05/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_migcom.h,v 1.5 2007/10/20 14:24:20 stavam2 Exp $
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
#ifndef _TCMI_MIGCOM_H
#define _TCMI_MIGCOM_H

#include <linux/types.h>

#include <tcmi/manager/tcmi_migman.h>
#include <tcmi/ctlfs/tcmi_ctlfs_entry.h>
#include <tcmi/comm/tcmi_msg.h>

struct tcmi_npm_params;

/** @defgroup tcmi_migcom_class tcmi_migcom class 
 *
 * @ingroup tcmi_migration_group
 *
 * A \<\<singleton\>\> class that handles migration requests from user
 * space as well as from  migration managers.
 * 
 * @{
 */

/** \<\<public\>\> Migrates a task from a CCN to a PEN */
extern int tcmi_migcom_emigrate_ccn_ppm_p(pid_t pid, struct tcmi_migman *migman);

/** \<\<public\>\> Migrates non-preemptively a task from a CCN to a PEN */
extern int tcmi_migcom_emigrate_ccn_npm(pid_t pid, struct tcmi_migman *migman, struct pt_regs* regs, struct tcmi_npm_params* npm_params);

/** \<\<public\>\> Immigrates a task - handles accepting the task on the PEN side */
extern int tcmi_migcom_immigrate(struct tcmi_msg *m, struct tcmi_migman *migman);

/** \<\<public\>\> Migrates task home from PEN (method can be used both on PEN or CCN) */
extern int tcmi_migcom_migrate_home_ppm_p(pid_t pid);

/** \<\<public\>\> Fork of guest */
extern int tcmi_migcom_guest_fork(struct task_struct* parent, struct task_struct* child, struct tcmi_migman* migman);

/** \<\<public\>\> Fork of shadow */
extern int tcmi_migcom_shadow_fork(struct task_struct* parent, struct task_struct* child, struct tcmi_migman* migman);

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_MIGCOM_PRIVATE

/** Initial thread for a newly 'immigrated' task. */
static int tcmi_migcom_migrated_task(void *data);

/** Migration mode handler. */
static void tcmi_migcom_mig_mode_handler(void);

#endif /* TCMI_MIGCOM_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_MIGCOM_H */

