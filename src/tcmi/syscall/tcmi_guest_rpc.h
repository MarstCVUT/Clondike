/**
 * @file tcmi_guest_rpc.h - a RPC class used by guest process .
 *
 * Date: 7/05/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_guest_rpc.h,v 1.3 2007/10/20 14:24:20 stavam2 Exp $
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

#ifndef _TCMI_GUEST_RPC_H
#define _TCMI_GUEST_RPC_H

#include "tcmi_rpc.h"
/** @defgroup tcmi_guest_rpc_class tcmi_guest_rpc class 
 *
 * @ingroup tcmi_rpc_class
 *
 * A \<\<singleton\>\> class that is executing RPC on PEN side.
 * 
 * @{
 */

/** tcmi_guest_rpc class declaration */
extern struct tcmi_rpc *tcmi_guest_rpc;

/** Creates default RPC call for system call declaration
 *
 * @param name - name of system call
 */
#define TCMI_GUEST_RPC_CALL_DEF(name)\
long tcmi_guest_rpc_sys_##name(unsigned rpc_num, long params[TCMI_RPC_MAXPARAMS])

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_GUEST_RPC_PRIVATE
#include "tcmi_guest_generic_rpc.h"
#include "tcmi_guest_signal_rpc.h"
#include "tcmi_guest_wait_rpc.h"
#include "tcmi_guest_fork_rpc.h"
#include "tcmi_guest_groupident_rpc.h"
#include "tcmi_guest_userident_rpc.h"

/** Guest RPC handlers */
long (*tcmi_calltable_guest[TCMI_MAX_RPC_NUM + 1])(unsigned,long[TCMI_RPC_MAXPARAMS]) = {
	/* Signal ********************************************/
	[TCMI_RPC_SYS_KILL] 	= &tcmi_guest_rpc_sys_2,
	[TCMI_RPC_SYS_SIGQUEUE] = &tcmi_guest_rpc_sys_rt_sigqueueinfo,
	[TCMI_RPC_SYS_TKILL] 	= &tcmi_guest_rpc_sys_3,

	/* Pid, gid and session manipulation *****************/
	[TCMI_RPC_SYS_GETPPID]	= &tcmi_guest_rpc_sys_0,
	[TCMI_RPC_SYS_GETSID]	= &tcmi_guest_rpc_sys_1,
	[TCMI_RPC_SYS_SETSID]	= &tcmi_guest_rpc_sys_0,
	[TCMI_RPC_SYS_GETPGRP]	= &tcmi_guest_rpc_sys_0,
	[TCMI_RPC_SYS_GETPGID]	= &tcmi_guest_rpc_sys_1,
	[TCMI_RPC_SYS_SETPGID]	= &tcmi_guest_rpc_sys_2,

	/* User identification *******************************/
	[TCMI_RPC_SYS_GETEUID]	= &tcmi_guest_rpc_sys_0,
	[TCMI_RPC_SYS_GETRESUID]= &tcmi_guest_rpc_sys_getresuid,
	[TCMI_RPC_SYS_GETUID]	= &tcmi_guest_rpc_sys_0,
	[TCMI_RPC_SYS_SETRESUID]= &tcmi_guest_rpc_sys_3,
	[TCMI_RPC_SYS_SETREUID]	= &tcmi_guest_rpc_sys_2,
	[TCMI_RPC_SYS_SETUID]	= &tcmi_guest_rpc_sys_1,

	/* Group identification ******************************/
	[TCMI_RPC_SYS_GETEGID]	= &tcmi_guest_rpc_sys_0,
	[TCMI_RPC_SYS_GETGID]	= &tcmi_guest_rpc_sys_0,
	[TCMI_RPC_SYS_GETRESGID]= &tcmi_guest_rpc_sys_getresgid,
	[TCMI_RPC_SYS_GETGROUPS]= &tcmi_guest_rpc_sys_getgroups,
	[TCMI_RPC_SYS_SETGID]	= &tcmi_guest_rpc_sys_1,
	[TCMI_RPC_SYS_SETREGID]	= &tcmi_guest_rpc_sys_2,
	[TCMI_RPC_SYS_SETRESGID]= &tcmi_guest_rpc_sys_3,
	[TCMI_RPC_SYS_SETGROUPS]= &tcmi_guest_rpc_sys_setgroups,

	[TCMI_RPC_SYS_WAIT4]= &tcmi_guest_rpc_sys_wait4,

	[TCMI_RPC_SYS_FORK]= &tcmi_guest_rpc_sys_fork,
};

#endif /* TCMI_GUEST_RPC_PRIVATE */
/**
 * @}
 */

#endif /* _TCMI_GUEST_RPC_H */

