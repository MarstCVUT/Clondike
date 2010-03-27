/**
 * @file tcmi_syscallhooks_userident.c - syscalls hooks for proces identification manipulation.
 *                      
 * 
 *
 *
 * Date: 13/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_syscallhooks_userident.c,v 1.3 2007/09/02 15:40:39 malatp1 Exp $
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


#include <asm/uaccess.h>
#include <linux/capability.h>
#include <clondike/tcmi/tcmi_hooks.h>

#include <dbg.h>
#include <tcmi/syscall/tcmi_guest_rpc.h>

#define TCMI_SYSCALLHOOKS_USERIDENT_PRIVATE
#include "tcmi_syscallhooks_userident.h"

/** 
 * \<\<public\>\> Registers all user identification syscalls hooks with the kernel.
 *
 * @return 0 upon success
 */
int tcmi_syscall_hooks_userident_init(void)
{
	minfo(INFO3, "Registering TCMI user identification syscalls hooks\n" 
		     "For: getuid, geteuid, getresuid, setresuid, setuid" );
	tcmi_hooks_register_sys_getuid(    tcmi_syscall_hooks_sys_getuid    );
	tcmi_hooks_register_sys_setuid(    tcmi_syscall_hooks_sys_setuid    );
	tcmi_hooks_register_sys_getresuid( tcmi_syscall_hooks_sys_getresuid );
	tcmi_hooks_register_sys_setresuid( tcmi_syscall_hooks_sys_setresuid );
	tcmi_hooks_register_sys_setreuid(  tcmi_syscall_hooks_sys_setreuid );
	tcmi_hooks_register_sys_geteuid(   tcmi_syscall_hooks_sys_geteuid   );
	return 0;
}

/** 
 * \<\<public\>\> Unregisters all user identifikation syscalls hooks.
 */
void tcmi_syscall_hooks_userident_exit(void)
{
	minfo(INFO3, "Unregistering TCMI user identification syscalls hooks");
	tcmi_hooks_unregister_sys_getuid();
	tcmi_hooks_unregister_sys_setuid();
	tcmi_hooks_unregister_sys_getresuid();
	tcmi_hooks_unregister_sys_setresuid();
	tcmi_hooks_unregister_sys_setreuid();
	tcmi_hooks_unregister_sys_geteuid();
}


/**
 * \<\<private\>\> Getuid system call hook
 *
 * @return - uid of current process
 */
static long tcmi_syscall_hooks_sys_getuid(void)
{
	return tcmi_rpc_call0(tcmi_guest_rpc, TCMI_RPC_SYS_GETUID);
}

/**
 * \<\<private\>\> Setresuid system call hook
 * @param ruid - real user id
 * @param euid - efective user id
 * @param suid - saved set-user id
 *
 * @return zero on success
 */
static long tcmi_syscall_hooks_sys_setresuid(uid_t ruid, uid_t euid, uid_t suid)
{
	return tcmi_rpc_call3(tcmi_guest_rpc, TCMI_RPC_SYS_SETRESUID, ruid, euid, suid);
}

/**
 * \<\<private\>\> Geteuid system call hook
 *
 * @return efective uid of current process
 */
static long tcmi_syscall_hooks_sys_geteuid(void)
{
	return tcmi_rpc_call0(tcmi_guest_rpc, TCMI_RPC_SYS_GETEUID);
}

/** \<\<private\>\> Setuid system call hook 
 * @param uid - efective user id
 * 
 * @return zero on success
 */
static long tcmi_syscall_hooks_sys_setuid(uid_t uid)
{
	return tcmi_rpc_call1(tcmi_guest_rpc, TCMI_RPC_SYS_SETUID, uid);
}

/** \<\<private\>\> Setreuid system call hook 
 * @param ruid - real user id
 * @param euid - efective user id
 *
 * @return zero on success
 */
static long tcmi_syscall_hooks_sys_setreuid(uid_t ruid, uid_t euid)
{
	return tcmi_rpc_call2(tcmi_guest_rpc, TCMI_RPC_SYS_SETREUID, ruid, euid);
}

/** \<\<private\>\> Getresuid system call hook (all pointers points to userspace)
 * @param *ruid - pointer to uid_t for real user id
 * @param *euid - pointer to uid_t for efective user id
 * @param *suid - pointer to uid_t for saved set-user id
 *
 * @return zero on success
 * */
static long tcmi_syscall_hooks_sys_getresuid(uid_t* ruid, uid_t* euid, uid_t *suid)
{
	return tcmi_rpc_call3(tcmi_guest_rpc, TCMI_RPC_SYS_GETRESUID, (unsigned long)ruid, (unsigned long)euid, (unsigned long)suid);
}
