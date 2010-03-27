/**
 * @file tcmi_syscallhooks_other.h - other syscall hooks.
 *                      
 * 
 *
 *
 * Date: 13/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_syscallhooks_other.h,v 1.4 2007/10/20 14:24:20 stavam2 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2007  Petr Malat
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

#ifndef _TCMI_SYSCALLHOOKS_OTHER_H
#define _TCMI_SYSCALLHOOKS_OTHER_H

/** @defgroup tcmi_syscallhooks_other_class tcmi_syscallhooks_other class 
 *
 * @ingroup tcmi_syscallhooks_class
 *
 * A \<\<singleton\>\> class that installs syscalls hooks into the kernel.
 * 
 * @{
 */

/** \<\<public\>\> Registers all hooks with the kernel. */
int tcmi_syscall_hooks_other_init(void);

/** \<\<public\>\> Unregisters all hooks. */
void tcmi_syscall_hooks_other_exit(void);


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SYSCALLHOOKS_OTHER_PRIVATE
/** sys_wait4 hook */
static long tcmi_syscall_hooks_sys_wait4(pid_t pid, int __user *stat_addr, int options, struct rusage __user *ru);

static long tcmi_syscall_hooks_pre_fork(unsigned long clone_flags, unsigned long stack_start, 
					struct pt_regs *regs, unsigned long stack_size, 
					int __user *parent_tidptr, int __user *child_tidptr
);

static long tcmi_syscall_hooks_in_fork(struct task_struct* child);

static long tcmi_syscall_hooks_post_fork(struct task_struct* child, long res, pid_t remote_pid, 
					int __user *parent_tidptr, int __user *child_tidptr
);


#endif /* TCMI_SYSCALLHOOKS_OTHER_PRIVATE */

/**
 * @}
 */


#endif /* _TCMI_SYSCALLHOOKS_OTHER_H */

