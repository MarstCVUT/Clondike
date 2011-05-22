/**
 * @file tcmi_syscallhooks_other.c - other syscalls hooks.
 *                      
 * 
 *
 *
 * Date: 13/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_syscallhooks_other.c,v 1.5 2008/01/17 14:36:44 stavam2 Exp $
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

#define TCMI_SYSCALLHOOKS_OTHER_PRIVATE
#include "tcmi_syscallhooks_other.h"

#include <tcmi/manager/tcmi_man.h>
#include <tcmi/manager/tcmi_penman.h>
#include <tcmi/manager/tcmi_ccnman.h>
#include <tcmi/task/tcmi_guesttask.h>

#include <proxyfs/proxyfs_server.h>
#include <director/director.h>

#include <asm/uaccess.h>
#include <linux/err.h>

/** 
 * \<\<public\>\> Registers all syscalls hooks with the kernel.
 *
 * @return 0 upon success
 */
int tcmi_syscall_hooks_other_init(void)
{
	minfo(INFO1, "Registering TCMI other syscalls hooks\n"
			"(wait)");
	tcmi_hooks_register_sys_wait4(tcmi_syscall_hooks_sys_wait4);
	tcmi_hooks_register_pre_fork(tcmi_syscall_hooks_pre_fork);
	tcmi_hooks_register_in_fork(tcmi_syscall_hooks_in_fork);
	tcmi_hooks_register_post_fork(tcmi_syscall_hooks_post_fork);
	
	return 0;
}

/** 
 * \<\<public\>\> Unregisters all syscalls hooks.
 */
void tcmi_syscall_hooks_other_exit(void)
{
	minfo(INFO1, "Unregistering TCMI other syscalls hooks"
			"(wait)");
	tcmi_hooks_unregister_sys_wait4();
	tcmi_hooks_unregister_pre_fork();
	tcmi_hooks_unregister_in_fork();
	tcmi_hooks_unregister_post_fork();
}

/** @addtogroup tcmi_syscallhooks_class
 *
 * @{
 */

/**
 * \<\<private\>\> sys_wait4 system call hook.  
 */
static long tcmi_syscall_hooks_sys_wait4(pid_t pid, int __user *stat_addr, int options, struct rusage __user *ru) {
	return tcmi_rpc_call4(tcmi_guest_rpc, TCMI_RPC_SYS_WAIT4, pid, (unsigned long)stat_addr, options, (unsigned long)ru);
};


/**
 * \<\<private\>\> pre-fork
 */
static long tcmi_syscall_hooks_pre_fork(unsigned long clone_flags, unsigned long stack_start, 
					struct pt_regs *regs, unsigned long stack_size, 
					int __user *parent_tidptr, int __user *child_tidptr
) {
	// Pre-fork is hooked only on DN, on CN we fork normally
	if ( current->tcmi.task_type == guest ) {
		return tcmi_rpc_call6(tcmi_guest_rpc, TCMI_RPC_SYS_FORK, clone_flags, stack_start, (unsigned long)regs, stack_size, (unsigned long)parent_tidptr, (unsigned long)child_tidptr);
	}

	return 0;
};

/**
 * \<\<private\>\> in-fork... performs attachement of guest/shadow task to TCMI
 */
static long tcmi_syscall_hooks_in_fork(struct task_struct* child) {
	if ( current->tcmi.task_type == guest ) {
		// Parent task is guest => child will be guest
		struct tcmi_penman* pen_man = tcmi_penman_get_instance();

		return tcmi_man_fork(TCMI_MAN(pen_man), current, child);
	} else if ( current->tcmi.task_type == shadow ) {
		// Parent task is shadow => child should be shadow 
		struct tcmi_ccnman* ccn_man = tcmi_ccnman_get_instance();
		
		return tcmi_man_fork(TCMI_MAN(ccn_man), current, child);
	}

	return 0;	
}

/**
 * \<\<private\>\> post-fork
 */
static long tcmi_syscall_hooks_post_fork(struct task_struct* child, long res, pid_t remote_pid, 
					int __user *parent_tidptr, int __user *child_tidptr) 
{
	if ( current->tcmi.task_type == guest ) {
		// Guest task hook
		struct tcmi_task* child_task = NULL;
		if ( IS_ERR((void*)res) ) {			
			mdbg(INFO3, "Resetting fork return values due to error");	
			if ( parent_tidptr != NULL )
				//put_user((int)-1, parent_tidptr);
			if ( child_tidptr != NULL ) {				
				//put_user((int)-1, child_tidptr);
			}
		} else {
			child_task = TCMI_TASK(child->tcmi.tcmi_task);
		}

		tcmi_guesttask_post_fork(TCMI_TASK(current->tcmi.tcmi_task), child_task, res, remote_pid);

		return remote_pid;
	} else if ( current->tcmi.task_type == shadow ) {
		// Shadow task hook

		// We have to duplicate all proxyfs file references for child.. this way we are in a same situation as
		// if the fork happend on CCN before the migration
		if (!IS_ERR((void*)res) ) {			
			proxyfs_server_duplicate_all_parent(current, child);
		}
	}

	director_task_fork(res, current->pid);

	return res;
};


/**
 * @}
 */
