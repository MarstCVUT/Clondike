/**
 * @file tcmi_syscallhooks_signal.c - other syscalls hooks.
 *                      
 * 
 *
 *
 * Date: 13/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_syscallhooks_signal.c,v 1.2 2007/09/02 13:54:30 stavam2 Exp $
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

#include <linux/capability.h>
#include <clondike/tcmi/tcmi_hooks.h>

#include <dbg.h>
#include <tcmi/syscall/tcmi_guest_rpc.h>

#define TCMI_SYSCALLHOOKS_SIGNAL_PRIVATE
#include <asm-generic/siginfo.h>
#include "tcmi_syscallhooks_signal.h"

/** 
 * \<\<public\>\> Registers all syscalls hooks with the kernel.
 *
 * @return 0 upon success
 */
int tcmi_syscall_hooks_signal_init(void)
{
	minfo(INFO3, "Registering TCMI signal syscalls hooks\n"
		     "For: kill, tkill, tgkill, sigqueue");
	tcmi_hooks_register_sys_kill(tcmi_syscall_hooks_sys_kill);
	tcmi_hooks_register_do_tkill(tcmi_syscall_hooks_do_tkill);
	tcmi_hooks_register_sys_rt_sigqueueinfo( tcmi_syscall_hooks_sys_rt_sigqueueinfo );
	return 0;
}

/** 
 * \<\<public\>\> Unregisters all syscalls hooks.
 */
void tcmi_syscall_hooks_signal_exit(void)
{
	minfo(INFO3, "Unregistering TCMI signal syscalls hooks");
	tcmi_hooks_unregister_sys_kill();
	tcmi_hooks_unregister_do_tkill();
	tcmi_hooks_unregister_sys_rt_sigqueueinfo();
}

/** @addtogroup tcmi_syscallhooks_class
 *
 * @{
 */

/**
 * \<\<private\>\> Kill system call hook.  
 *
 * @param pid - PID of signal target
 * @param sig - what signal
 *
 * @return - zero on sucess.
 */
static long tcmi_syscall_hooks_sys_kill(int pid, int sig)
{
	return tcmi_rpc_call2(tcmi_guest_rpc, TCMI_RPC_SYS_KILL, pid, sig);
}

/**
 * \<\<private\>\> do_tkill hook. 
 * Handles both tkill and tgkill system call
 *
 * @param tgid - the thread group ID of the thread
 * @param pid - the PID of the thread
 * @param sig - signal to be sent
 *
 * @return - zero on sucess.
 */
static long tcmi_syscall_hooks_do_tkill(int tgid, int pid, int sig)
{
	return tcmi_rpc_call3(tcmi_guest_rpc, TCMI_RPC_SYS_TKILL, tgid, pid, sig);
}


/**
 * \<\<private\>\> sys_rt_sigqueueinfo hook. 
 * @param pid - the PID of the thread
 * @param sig - signal to be sent
 * @param *info - pointer siginfo struct in kernel space
 *
 * @return - zero on sucess.
 */
static long tcmi_syscall_hooks_sys_rt_sigqueueinfo(int pid, int sig, siginfo_t *info)
{
	return tcmi_rpc_call3(tcmi_guest_rpc, TCMI_RPC_SYS_SIGQUEUE, pid, sig, (unsigned long)info);
}

