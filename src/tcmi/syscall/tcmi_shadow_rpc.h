/**
 * @file tcmi_shadow_rpc.h - a RPC class used by shadow process .
 *
 * Date: 7/05/2007
 *
 * Author: Petr Malat
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

#ifndef _TCMI_SHADOW_RPC_H
#define _TCMI_SHADOW_RPC_H

#include "tcmi_rpc.h"
#include <tcmi/task/tcmi_task.h>
#include <tcmi/comm/tcmi_rpc_procmsg.h>
#include <tcmi/comm/tcmi_rpcresp_procmsg.h>

/** @defgroup tcmi_shadow_rpc_class tcmi_shadow_rpc class 
 *
 * @ingroup tcmi_rpc_class
 *
 * A \<\<singleton\>\> class that is executing RPC on CCN side.
 * 
 * @{
 */

/** Creates default RPC call for system call
 *
 * @param num       - number of parameters
 * @param name      - name of system call
 * @param use_errno - if true return errno when syscall returns -1
 *
 * @return system call return value
 */
#define TCMI_SHADOW_RPC_GENERIC_CALL(num, name, use_errno)\
long tcmi_shadow_rpc_##name(unsigned rpc_num, long params[TCMI_RPC_MAXPARAMS]){\
	struct tcmi_msg *m = (struct tcmi_msg*) params[0], **resp = (struct tcmi_msg**) params[1];\
	long rtn; _TCMI_SHADOW_RPC_GENERIC_CALL##num(name); \
	if( use_errno && rtn == -1) rtn = errno;\
	*resp = tcmi_rpcresp_procmsg_create(m, rtn, (size_t)0 );\
	if( *resp == NULL ) return -1;return 0;\
}


/** Used int #TCMI_SHADOW_RPC_GENERIC_CALL for defining rpc_params, if call works with them */
#define _TCMI_SHADOW_RPC_GENERIC_CALL_PARAMS long* rpc_params = (long*)tcmi_rpc_procmsg_data_base( TCMI_RPC_PROCMSG(m), 0);

/** Helper macro for #TCMI_SHADOW_RPC_GENERIC_CALL */
#define _TCMI_SHADOW_RPC_GENERIC_CALL0(name) rtn = name();

/** Helper macro for #TCMI_SHADOW_RPC_GENERIC_CALL */
#define _TCMI_SHADOW_RPC_GENERIC_CALL1(name) _TCMI_SHADOW_RPC_GENERIC_CALL_PARAMS	\
	rtn = name(rpc_params[0]);

/** Helper macro for #TCMI_SHADOW_RPC_GENERIC_CALL */
#define _TCMI_SHADOW_RPC_GENERIC_CALL2(name) _TCMI_SHADOW_RPC_GENERIC_CALL_PARAMS	\
	rtn = name(rpc_params[0],rpc_params[1]);

/** Helper macro for #TCMI_SHADOW_RPC_GENERIC_CALL */
#define _TCMI_SHADOW_RPC_GENERIC_CALL3(name) _TCMI_SHADOW_RPC_GENERIC_CALL_PARAMS	\
	rtn = name(rpc_params[0],rpc_params[1],rpc_params[2]);

/** Helper macro for #TCMI_SHADOW_RPC_GENERIC_CALL */
#define _TCMI_SHADOW_RPC_GENERIC_CALL4(name) _TCMI_SHADOW_RPC_GENERIC_CALL_PARAMS	\
	rtn = name(rpc_params[0],rpc_params[1],rpc_params[2],rpc_params[3]);

/** Helper macro for #TCMI_SHADOW_RPC_GENERIC_CALL */
#define _TCMI_SHADOW_RPC_GENERIC_CALL5(name) _TCMI_SHADOW_RPC_GENERIC_CALL_PARAMS	\
	rtn = name(rpc_params[0],rpc_params[1],rpc_params[2],rpc_params[3],rpc_params[4]);

/** Helper macro for #TCMI_SHADOW_RPC_GENERIC_CALL */
#define _TCMI_SHADOW_RPC_GENERIC_CALL6(name) _TCMI_SHADOW_RPC_GENERIC_CALL_PARAMS	\
	rtn = name(rpc_params[0],rpc_params[1],rpc_params[2],rpc_params[3],rpc_params[4],rpc_params[5]);


/** Creates default RPC call for system call declaration
 *
 * @param name - name of system call
 */
#define TCMI_SHADOW_RPC_GENERIC_CALL_DEF(name)\
long tcmi_shadow_rpc_##name(unsigned rpc_num, long params[TCMI_RPC_MAXPARAMS])

/** tcmi_shadow_rpc class declaration */
extern struct tcmi_rpc *tcmi_shadow_rpc;
/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SHADOW_RPC_PRIVATE
#include "tcmi_shadow_signal_rpc.h"
#include "tcmi_shadow_wait_rpc.h"
#include "tcmi_shadow_fork_rpc.h"
#include "tcmi_shadow_pidman_rpc.h"
#include "tcmi_shadow_userident_rpc.h"
#include "tcmi_shadow_groupident_rpc.h"

/** Table with RPC handlers */
long (*tcmi_calltable_shadow[TCMI_MAX_RPC_NUM + 1])(unsigned,long[TCMI_MAX_RPC_NUM]) = {
	/* Signal ********************************************/
	[TCMI_RPC_SYS_KILL] 	= &tcmi_shadow_rpc_sys_kill,
	[TCMI_RPC_SYS_TKILL] 	= &tcmi_shadow_rpc_do_tkill,
	[TCMI_RPC_SYS_SIGQUEUE]	= &tcmi_shadow_rpc_rt_sigqueueinfo,

	/* Pid, gid and session manipulation *****************/
	[TCMI_RPC_SYS_GETPGID]	= &tcmi_shadow_rpc_sys_getpgid,
	[TCMI_RPC_SYS_GETPGRP]	= &tcmi_shadow_rpc_sys_getpgrp,
	[TCMI_RPC_SYS_GETPPID]	= &tcmi_shadow_rpc_sys_getppid,
	[TCMI_RPC_SYS_GETSID]	= &tcmi_shadow_rpc_sys_getsid,
	[TCMI_RPC_SYS_SETPGID]	= &tcmi_shadow_rpc_sys_setpgid,
	[TCMI_RPC_SYS_SETSID]	= &tcmi_shadow_rpc_sys_setsid,

	/* User identification *******************************/
	[TCMI_RPC_SYS_GETUID]	= &tcmi_shadow_rpc_getuid,
	[TCMI_RPC_SYS_GETEUID]	= &tcmi_shadow_rpc_geteuid,
	[TCMI_RPC_SYS_GETRESUID]= &tcmi_shadow_rpc_getresuid,
	[TCMI_RPC_SYS_SETUID]	= &tcmi_shadow_rpc_setuid,
	[TCMI_RPC_SYS_SETREUID]	= &tcmi_shadow_rpc_setreuid,
	[TCMI_RPC_SYS_SETRESUID]= &tcmi_shadow_rpc_setresuid,

	/* Group identification ******************************/
	[TCMI_RPC_SYS_GETGID]	= &tcmi_shadow_rpc_getgid,
	[TCMI_RPC_SYS_GETEGID]	= &tcmi_shadow_rpc_getegid,
	[TCMI_RPC_SYS_GETGROUPS]= &tcmi_shadow_rpc_getgroups,
	[TCMI_RPC_SYS_GETRESGID]= &tcmi_shadow_rpc_getresgid,
	[TCMI_RPC_SYS_SETGID]	= &tcmi_shadow_rpc_setgid,
	[TCMI_RPC_SYS_SETGROUPS]= &tcmi_shadow_rpc_setgroups,
	[TCMI_RPC_SYS_SETREGID]	= &tcmi_shadow_rpc_setregid,
	[TCMI_RPC_SYS_SETRESGID]= &tcmi_shadow_rpc_setresgid,
	
	[TCMI_RPC_SYS_WAIT4]= &tcmi_shadow_rpc_sys_wait4,
	[TCMI_RPC_SYS_FORK]= &tcmi_shadow_rpc_do_fork,
};
#endif /* TCMI_SHADOW_RPC_PRIVATE */
/**
 * @}
 */

#endif /* _TCMI_SHADOW_RPC_H */

