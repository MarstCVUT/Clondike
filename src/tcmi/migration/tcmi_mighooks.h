/**
 * @file tcmi_mighooks.h - a separate module that install migration hooks into the kernel.
 *                      
 * 
 *
 *
 * Date: 05/06/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_mighooks.h,v 1.5 2007/11/05 19:38:28 stavam2 Exp $
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

#include <asm/uaccess.h>

#ifndef _TCMI_MIGHOOKS_H
#define _TCMI_MIGHOOKS_H

/** @defgroup tcmi_mighooks_class tcmi_mighooks class 
 *
 * @ingroup tcmi_migration_group
 *
 * A \<\<singleton\>\> class that installs migration hooks into the kernel.
 * Currently the hooks cover:
 * - \link tcmi_mighooks_mig_mode() migration mode callback \endlink
 * - \link tcmi_mighooks_do_exit() exit system call hook \endlink
 * 
 * @{
 */

/** \<\<public\>\> Registers all hooks with the kernel. */
int tcmi_mighooks_init(void);

/** \<\<public\>\> Unregisters all hooks. */
void tcmi_mighooks_exit(void);


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_MIGHOOKS_PRIVATE
/** Exit system call hook */
static long tcmi_mighooks_do_exit(long code);

/** This hook is called upon SIGUNUSED delivery to a particular process. */
static long tcmi_mighooks_mig_mode(struct pt_regs *regs);

/** This hook is called on every execve invocation */
static long tcmi_mighooks_execve(char *filename, char __user * __user * argv, char __user * __user * envp, struct pt_regs *regs);

/** This hook replaces paths starting with /proc/self with /proc/<<PROCESS PID>> so that remote querying of /proc/self works. it is used in path_lookup kernel function */
static long tcmi_mighooks_replace_proc_self_file(const char* filename, const char** result);

#endif /* TCMI_MIGHOOKS_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_MIGHOOKS_H */

