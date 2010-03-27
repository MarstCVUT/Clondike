/**
 * @file tcmi_shadow_rpc_userident.c - CCN part of RPCs
 *
 * Date: 7/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_shadow_userident_rpc.c,v 1.3 2007/10/11 21:00:26 stavam2 Exp $
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

static int errno;
#include <arch/current/make_syscall.h>

#include "tcmi_shadow_userident_rpc.h"

static inline _syscall0(long, getuid     );
static inline _syscall0(long, geteuid    );
static inline _syscall1(long,  setuid,    uid_t, uid);
static inline _syscall2(long,  setreuid,  uid_t, ruid, uid_t, euid);
static inline _syscall3(long,  setresuid, uid_t, ruid, uid_t, euid, uid_t, suid);
static inline _syscall3(long,  getresuid, uid_t*, ruid, uid_t*, euid, uid_t*, suid);

TCMI_SHADOW_RPC_GENERIC_CALL(0, geteuid,   false);
TCMI_SHADOW_RPC_GENERIC_CALL(0, getuid,    false);
TCMI_SHADOW_RPC_GENERIC_CALL(1, setuid,    true );
TCMI_SHADOW_RPC_GENERIC_CALL(2, setreuid,  true );
TCMI_SHADOW_RPC_GENERIC_CALL(3, setresuid, true );


/*
long tcmi_shadow_rpc_getuid(unsigned rpc_num, long params[TCMI_RPC_MAXPARAMS]) {
	mm_segment_t old_fs;
	struct tcmi_msg *m = (struct tcmi_msg*) params[0], **resp = (struct tcmi_msg**) params[1];
	long rtn; 


	old_fs = get_fs();
	set_fs(KERNEL_DS);
	rtn = sys_getuid();
	set_fs(old_fs);

	*resp = tcmi_rpcresp_procmsg_create(m, rtn, (size_t)0 );
	if( *resp == NULL ) 
		return -1;
	return 0;
}
*/

TCMI_SHADOW_RPC_GENERIC_CALL_DEF(getresuid)
{
	struct tcmi_msg **resp, *m;
	long rtn;
	uid_t *uids;
	mm_segment_t old_fs;

	m = (struct tcmi_msg*) params[0];
	resp = (struct tcmi_msg**) params[1];

	if( (uids = kmalloc( 3 * sizeof(uid_t), GFP_KERNEL ) ) == NULL ){
			mdbg(ERR3, "Buffer allocation failed");
			return -1;
	}
	
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	rtn = getresuid( uids, uids + 1, uids + 2 );
	set_fs(old_fs);

	if( rtn == -1) // Should be never true
		rtn = errno;
	else{
		if( (*resp = tcmi_rpcresp_procmsg_create(m, rtn, 3 * sizeof(uid_t), uids, (size_t)0 )) == NULL ){
			kfree( uids );
			return -1;
		}
		tcmi_rpcresp_procmsg_free_on_put( TCMI_RPCRESP_PROCMSG(*resp), 0, TCMI_RPCRESP_PROCMSG_KFREE );
	}
	
	return 0;
}

