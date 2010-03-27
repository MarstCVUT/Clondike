/**
 * @file tcmi_shadow_rpc_groupident.c - CCN part of RPCs
 *
 * Date: 7/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_shadow_groupident_rpc.c,v 1.3 2007/10/11 21:00:26 stavam2 Exp $
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

#include "tcmi_shadow_groupident_rpc.h"

static inline _syscall0(gid_t, getegid    );
static inline _syscall0(gid_t, getgid     );
static inline _syscall1(long,  setgid,    gid_t, gid);
static inline _syscall2(long,  getgroups, int, size, gid_t*, list);
static inline _syscall2(long,  setgroups, int, size, const gid_t*, list);
static inline _syscall2(long,  setregid,  gid_t, rgid, gid_t, egid);
static inline _syscall3(long,  getresgid, gid_t*, rgid, gid_t*, egid, gid_t*, sgid);
static inline _syscall3(long,  setresgid, gid_t, rgid, gid_t, egid, gid_t, sgid);

TCMI_SHADOW_RPC_GENERIC_CALL(0, getegid,   false);
TCMI_SHADOW_RPC_GENERIC_CALL(0, getgid,    false);
TCMI_SHADOW_RPC_GENERIC_CALL(1, setgid,    true);
TCMI_SHADOW_RPC_GENERIC_CALL(2, setregid,  true);
TCMI_SHADOW_RPC_GENERIC_CALL(3, setresgid, true);

TCMI_SHADOW_RPC_GENERIC_CALL_DEF(getresgid)
{
	struct tcmi_msg **resp, *m;
	long rtn;
	uid_t *gids;
	mm_segment_t old_fs;

	m = (struct tcmi_msg*) params[0];
	resp = (struct tcmi_msg**) params[1];

	if( (gids = kmalloc( 3 * sizeof(gid_t), GFP_KERNEL ) ) == NULL ){
			mdbg(ERR3, "Buffer allocation failed");
			return -1;
	}
	
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	rtn = getresgid( gids, gids + 1, gids + 2 );
	set_fs(old_fs);

	if( rtn == -1) // Should be never true
		rtn = errno;
	else{
		if( (*resp = tcmi_rpcresp_procmsg_create(m, rtn, 3 * sizeof(gid_t), gids, (size_t)0 )) == NULL ){
			kfree( gids );
			return -1;
		}
		tcmi_rpcresp_procmsg_free_on_put( TCMI_RPCRESP_PROCMSG(*resp), 0, TCMI_RPCRESP_PROCMSG_KFREE );
	}
	
	return 0;
}

TCMI_SHADOW_RPC_GENERIC_CALL_DEF(setgroups)
{
	struct tcmi_msg **resp, *m;
	long* rpc_params, rtn;
	void *ker_buf;
	mm_segment_t old_fs;

	m = (struct tcmi_msg*) params[0];
	resp = (struct tcmi_msg**) params[1];
	
	rpc_params = (long*)tcmi_rpc_procmsg_data_base( TCMI_RPC_PROCMSG(m), 0);
	ker_buf = (long*)tcmi_rpc_procmsg_data_base( TCMI_RPC_PROCMSG(m), 1);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	rtn = setgroups( rpc_params[0], ker_buf );
	set_fs(old_fs);
	if( rtn == -1)
		rtn = errno;
	*resp = tcmi_rpcresp_procmsg_create(m, rtn, 0 );
	if( *resp == NULL ) 
		return -1;
	return 0;
}

TCMI_SHADOW_RPC_GENERIC_CALL_DEF(getgroups)
{
	struct tcmi_msg **resp, *m;
	long* rpc_params;
	long rtn, numb_of_groups;
	unsigned int i, count;
	gid_t *ker_buf;

	m = (struct tcmi_msg*) params[0];
	resp = (struct tcmi_msg**) params[1];
	
	rpc_params = (long*)tcmi_rpc_procmsg_data_base( TCMI_RPC_PROCMSG(m), 0);

	numb_of_groups = current_cred()->group_info->ngroups;
	if( rpc_params[0] == 0 )
		*resp = tcmi_rpcresp_procmsg_create(m, numb_of_groups, 0 );
	else if( rpc_params[0] < numb_of_groups )
		*resp = tcmi_rpcresp_procmsg_create(m, -EINVAL, 0 );
	else{
		if( (ker_buf = (gid_t*)kmalloc( numb_of_groups * sizeof(gid_t), GFP_KERNEL ) ) == NULL ){
			mdbg(ERR3, "Buffer allocation failed");
			return -1;
		}

		rtn = count = numb_of_groups;
		for (i = 0; i < count; i++) {
			int cp_count = min(NGROUPS_PER_BLOCK, count);
			int off = i * NGROUPS_PER_BLOCK;
			int len = cp_count * sizeof(*ker_buf);

			memcpy(ker_buf+off, current_cred()->group_info->blocks[i], len);

			count -= cp_count;
		}

		*resp = tcmi_rpcresp_procmsg_create(m, rtn, sizeof(gid_t)*numb_of_groups, ker_buf, 0 );
		tcmi_rpcresp_procmsg_free_on_put( TCMI_RPCRESP_PROCMSG(*resp), 0, TCMI_RPCRESP_PROCMSG_KFREE );
	}

	if( *resp == NULL ) 
		return -1;
	return 0;
}


