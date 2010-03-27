/**
 * @file tcmi_syscallhooks_groupident.c - syscalls hooks for proces identification manipulation.
 *                      
 * 
 *
 *
 * Date: 13/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_syscallhooks_groupident.c,v 1.2 2007/09/02 13:54:30 stavam2 Exp $
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
#include <linux/limits.h>
#include <clondike/tcmi/tcmi_hooks.h>

#include <dbg.h>
#include <tcmi/syscall/tcmi_guest_rpc.h>

#define TCMI_SYSCALLHOOKS_GROUPIDENT_PRIVATE
#include "tcmi_syscallhooks_groupident.h"

/** 
 * \<\<public\>\> Registers all user identification syscalls hooks with the kernel.
 *
 * @return 0 upon success
 */
int tcmi_syscall_hooks_groupident_init(void)
{
	minfo(INFO3, "Registering TCMI group identification syscalls hooks\n" 
		     "For: getgid, setgid, getegid, setregid, setresgid, getgroups, setgroups");
	tcmi_hooks_register_sys_getegid(   tcmi_syscall_hooks_sys_getegid   );
	tcmi_hooks_register_sys_getgid(    tcmi_syscall_hooks_sys_getgid    );
	tcmi_hooks_register_sys_setgid(    tcmi_syscall_hooks_sys_setgid    );
	tcmi_hooks_register_sys_setregid(  tcmi_syscall_hooks_sys_setregid  );
	tcmi_hooks_register_sys_setresgid( tcmi_syscall_hooks_sys_setresgid );
	tcmi_hooks_register_sys_getresgid( tcmi_syscall_hooks_sys_getresgid );
	tcmi_hooks_register_sys_getgroups( tcmi_syscall_hooks_sys_getgroups );
	tcmi_hooks_register_sys_setgroups( tcmi_syscall_hooks_sys_setgroups );
	return 0;
}

/** 
 * \<\<public\>\> Unregisters all user identifikation syscalls hooks.
 */
void tcmi_syscall_hooks_groupident_exit(void)
{
	minfo(INFO3, "Unregistering TCMI group identification syscalls hooks");
	tcmi_hooks_unregister_sys_getegid();
	tcmi_hooks_unregister_sys_getgid();
	tcmi_hooks_unregister_sys_setgid();
	tcmi_hooks_unregister_sys_setregid();
	tcmi_hooks_unregister_sys_setresgid();
	tcmi_hooks_unregister_sys_getresgid();
	tcmi_hooks_unregister_sys_getgroups();
	tcmi_hooks_unregister_sys_setgroups();
}

/**
 * \<\<private\>\> Getgid system call hook
 *
 * @return - the session ID of the calling process
 */
static long tcmi_syscall_hooks_sys_getgid(void)
{
	return tcmi_rpc_call0(tcmi_guest_rpc, TCMI_RPC_SYS_GETGID);
}

/**
 * \<\<private\>\> Getegid system call hook
 *
 * @return effective group id
 */
static long tcmi_syscall_hooks_sys_getegid(void)
{
	return tcmi_rpc_call0(tcmi_guest_rpc, TCMI_RPC_SYS_GETEGID);
}


/**
 * \<\<private\>\> Setgid system call hook
 *
 * @return  zero on success
 */
static long tcmi_syscall_hooks_sys_setgid(gid_t gid)
{
	return tcmi_rpc_call1(tcmi_guest_rpc, TCMI_RPC_SYS_SETGID, gid);
}

/**
 * \<\<private\>\> Setregid system call hook
 *
 * @return  zero on success
 */
static long tcmi_syscall_hooks_sys_setregid(gid_t rgid, gid_t egid)
{
	return tcmi_rpc_call2(tcmi_guest_rpc, TCMI_RPC_SYS_SETREGID, rgid, egid);
}

/**
 * \<\<private\>\> Getresgid system call hook
 *
 * @return  zero on success
 */
static long tcmi_syscall_hooks_sys_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid)
{
	return tcmi_rpc_call3(tcmi_guest_rpc, TCMI_RPC_SYS_GETRESGID, (unsigned long)rgid, (unsigned long)egid, (unsigned long)sgid);
}

/**
 * \<\<private\>\> Setresgid system call hook
 *
 * @return  zero on success
 */
static long tcmi_syscall_hooks_sys_setresgid(gid_t rgid, gid_t egid, gid_t sgid)
{
	return tcmi_rpc_call3(tcmi_guest_rpc, TCMI_RPC_SYS_SETRESGID, rgid, egid, sgid);
}

/**
 * \<\<private\>\> Getgroups system call hook
 *
 * @return the session ID of the calling process
 */
static long tcmi_syscall_hooks_sys_getgroups(int size, gid_t *groups)
{
	if( size == 0 )
		return tcmi_rpc_call2(tcmi_guest_rpc, TCMI_RPC_SYS_GETGROUPS, 0, (unsigned long)NULL);
	else{
		if( !access_ok( VERIFY_WRITE, groups, size * sizeof(gid_t) ) )
			return -EFAULT;
		return tcmi_rpc_call2(tcmi_guest_rpc, TCMI_RPC_SYS_GETGROUPS, size, (unsigned long)groups);
	}
}

/**
 * \<\<private\>\> Setgroups system call hook
 *
 * @return the session ID of the calling process
 */
static long tcmi_syscall_hooks_sys_setgroups(int size, const gid_t *groups)
{
	if( size > NGROUPS_MAX )
		return -EINVAL;
	else{
		if( !access_ok(VERIFY_READ, groups, size * sizeof(gid_t)) )
			return -EFAULT;
		return tcmi_rpc_call2(tcmi_guest_rpc, TCMI_RPC_SYS_SETGROUPS, 
				size, (unsigned long)groups );
	}
}

