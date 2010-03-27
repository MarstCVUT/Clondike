/**
 * @file tcmi_syscallhooks_signal.h - other syscall hooks.
 *                      
 * 
 *
 *
 * Date: 13/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_syscallhooks_signal.h,v 1.1 2007/09/02 12:09:55 malatp1 Exp $
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

#ifndef _TCMI_SYSCALLHOOKS_SIGNAL_H
#define _TCMI_SYSCALLHOOKS_SIGNAL_H

/** @defgroup tcmi_syscallhooks_signal_class Signal sending 
 *
 * @ingroup tcmi_syscallhooks_class
 *
 * A \<\<singleton\>\> class that installs syscalls hooks into the kernel.
 * 
 * @{
 */

/** \<\<public\>\> Registers all hooks with the kernel. */
int tcmi_syscall_hooks_signal_init(void);

/** \<\<public\>\> Unregisters all hooks. */
void tcmi_syscall_hooks_signal_exit(void);


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SYSCALLHOOKS_SIGNAL_PRIVATE
/** kill() system call hook */
static long tcmi_syscall_hooks_sys_kill(int pid, int sig);

/** do_tkill() hook */
static long tcmi_syscall_hooks_do_tkill(int tgid, int pid, int sig);

/** sys_rt_sigqueueinfo() hook */
static long tcmi_syscall_hooks_sys_rt_sigqueueinfo(int pid, int sig, siginfo_t *info);

#endif /* TCMI_SYSCALLHOOKS_SIGNAL_PRIVATE */

/**
 * @}
 */


#endif /* _TCMI_SYSCALLHOOKS_SIGNAL_H */

