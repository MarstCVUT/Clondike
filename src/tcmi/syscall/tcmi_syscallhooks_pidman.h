/**
 * @file tcmi_syscallhooks_pidman.h - syscalls hooks for proces identification manipulation.
 *                      
 * 
 *
 *
 * Date: 13/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_syscallhooks_pidman.h,v 1.2 2007/09/02 12:09:55 malatp1 Exp $
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


#ifndef _TCMI_SYSCALLHOOKS_PIDMAN_H
#define _TCMI_SYSCALLHOOKS_PIDMAN_H

/** @defgroup tcmi_syscallhooks_pidman_class PID & GID manipulation 
 *
 * @ingroup tcmi_syscallhooks_class
 *
 * A \<\<singleton\>\> class that installs PID manipulation syscalls 
 * hooks into the kernel.
 * 
 * @{
 */

/** \<\<public\>\> Registers all PID manipolation hooks with the kernel. */
int tcmi_syscall_hooks_pidman_init(void);

/** \<\<public\>\> Unregisters all PID manipolation hooks. */
void tcmi_syscall_hooks_pidman_exit(void);


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SYSCALLHOOKS_PIDMAN_PRIVATE

/** \<\<private\>\> Getppid system call hook */
static long tcmi_syscall_hooks_sys_getppid(void);

/** \<\<private\>\> Getpid system call hook */
static long tcmi_syscall_hooks_sys_getpid(void);

/** \<\<private\>\> Getpgid system call hook */
static long tcmi_syscall_hooks_sys_getpgid(pid_t pid);

/** \<\<private\>\> Setpgid system call hook */
static long tcmi_syscall_hooks_sys_setpgid(pid_t pid, pid_t pgid);

/** \<\<private\>\> Getsid system call hook */
static long tcmi_syscall_hooks_sys_getsid(pid_t pid);

/** \<\<private\>\> Setsid system call hook */
static long tcmi_syscall_hooks_sys_setsid(void);

/** \<\<private\>\> Getpgrp system call hook */
static long tcmi_syscall_hooks_sys_getpgrp(void);

#endif /* TCMI_SYSCALLHOOKS_PIDMAN_PRIVATE */

/**
 * @}
 */

#endif /* _TCMI_SYSCALLHOOKS_PIDMAN_H */

