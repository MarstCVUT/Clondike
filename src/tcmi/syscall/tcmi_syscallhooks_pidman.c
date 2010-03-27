/**
 * @file tcmi_syscallhooks_pidman.c - syscalls hooks for proces identification manipulation.
 *                      
 * 
 *
 *
 * Date: 13/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_syscallhooks_pidman.c,v 1.7 2007/10/20 14:24:20 stavam2 Exp $
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


#include <clondike/tcmi/tcmi_hooks.h>

#include <dbg.h>
#include <tcmi/syscall/tcmi_guest_rpc.h>
#include <tcmi/task/tcmi_task.h>
#include <arch/current/make_syscall.h>

#define TCMI_SYSCALLHOOKS_PIDMAN_PRIVATE
#include "tcmi_syscallhooks_pidman.h"

/** 
 * \<\<public\>\> Registers all PID manipulaton syscalls hooks with the kernel.
 *
 * @return 0 upon success
 */
int tcmi_syscall_hooks_pidman_init(void)
{
	minfo(INFO3, "Registering TCMI PID manipulation syscalls hooks\n"
		     "For: getpid, getppid, getpgit, setpgit, getsid, setsid, getpgrp");
	tcmi_hooks_register_sys_getppid( tcmi_syscall_hooks_sys_getppid );
	tcmi_hooks_register_sys_getpid ( tcmi_syscall_hooks_sys_getpid  );
	tcmi_hooks_register_sys_getpgid( tcmi_syscall_hooks_sys_getpgid );
	tcmi_hooks_register_sys_setpgid( tcmi_syscall_hooks_sys_setpgid );
	tcmi_hooks_register_sys_getsid ( tcmi_syscall_hooks_sys_getsid  );
	tcmi_hooks_register_sys_setsid ( tcmi_syscall_hooks_sys_setsid  );
	tcmi_hooks_register_sys_getpgrp( tcmi_syscall_hooks_sys_getpgrp );
	return 0;
}

/** 
 * \<\<public\>\> Unregisters all PID manipulation syscalls hooks.
 */
void tcmi_syscall_hooks_pidman_exit(void)
{
	minfo(INFO3, "Unregistering TCMI PID manipulation syscalls hooks\n");
	tcmi_hooks_unregister_sys_getppid();
	tcmi_hooks_unregister_sys_getpid();
	tcmi_hooks_unregister_sys_getpgid();
	tcmi_hooks_unregister_sys_setpgid();
	tcmi_hooks_unregister_sys_getsid();
	tcmi_hooks_unregister_sys_setsid();
	tcmi_hooks_unregister_sys_getpgrp();
}


/**
 * \<\<private\>\> Getppid system call hook
 *
 * @return pid of parent process.
 */
static long tcmi_syscall_hooks_sys_getppid(void)
{	
	return tcmi_rpc_call0(tcmi_guest_rpc, TCMI_RPC_SYS_GETPPID);
}

/**
 * \<\<private\>\> Getpid system call hook
 *
 * @return pid of current process.
 */
static long tcmi_syscall_hooks_sys_getpid(void)
{
	mdbg(INFO3, "Getpid hook called. Returning via fast-path pid %d. Physical pid: %d",tcmi_task_remote_pid(current->tcmi.tcmi_task), current->pid);
	/* Won't work with threads */
	return tcmi_task_remote_pid(current->tcmi.tcmi_task);
}

/**
 * \<\<private\>\> Getpgid system call hook
 *
 * @param pid - PID  
 * @return the process group ID of the process specified by pid.
 */
static long tcmi_syscall_hooks_sys_getpgid(pid_t pid)
{
	return tcmi_rpc_call1(tcmi_guest_rpc, TCMI_RPC_SYS_GETPGID, pid);
}

/**
 * \<\<private\>\> Setpgid system call hook
 * @param pid - PID of affected process 
 * @param sig - target group ID
 *
 * @return zero on sucess
 */
static long tcmi_syscall_hooks_sys_setpgid(pid_t pid, pid_t pgid)
{
	return tcmi_rpc_call2(tcmi_guest_rpc, TCMI_RPC_SYS_SETPGID, pid, pgid);
}

/**
 * \<\<private\>\> Getsid system call hook
 * @param pid - 
 *
 * @return the session ID of the process with process ID pid
 */
static long tcmi_syscall_hooks_sys_getsid(pid_t pid)
{
	return tcmi_rpc_call1(tcmi_guest_rpc, TCMI_RPC_SYS_GETSID, pid);
}

/**
 * \<\<private\>\> Setsid system call hook
 *
 * @return the session ID of the calling process
 */
static long tcmi_syscall_hooks_sys_setsid(void)
{
	return tcmi_rpc_call0(tcmi_guest_rpc, TCMI_RPC_SYS_SETSID);
}

/**
 * \<\<private\>\> Getpgrp system call hook
 *
 * @return current process group
 */
static long tcmi_syscall_hooks_sys_getpgrp(void)
{
	return tcmi_rpc_call0(tcmi_guest_rpc, TCMI_RPC_SYS_GETPGRP);
}


