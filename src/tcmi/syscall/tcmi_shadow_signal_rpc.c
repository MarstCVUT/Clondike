/**
 * @file tcmi_shadow_signal_rpc.c - shadow part of rpc
 *
 * Date: 7/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_shadow_signal_rpc.c,v 1.3 2007/10/11 21:00:26 stavam2 Exp $
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
#include <linux/syscalls.h>
#include <linux/uaccess.h>

static int errno;
#include <arch/current/make_syscall.h>

#include "tcmi_shadow_signal_rpc.h"
#include "exported_symbols.h"

static inline _syscall3(long, rt_sigqueueinfo, int, pid, int, sig, siginfo_t*, info);

TCMI_SHADOW_RPC_GENERIC_CALL(2, sys_kill, true);
TCMI_SHADOW_RPC_GENERIC_CALL(3, do_tkill, true);

TCMI_SHADOW_RPC_GENERIC_CALL_DEF(rt_sigqueueinfo)
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
	rtn = rt_sigqueueinfo(rpc_params[0], rpc_params[1], ker_buf);
	set_fs(old_fs);
	if( rtn == -1)
		rtn = errno;
	*resp = tcmi_rpcresp_procmsg_create(m, rtn, (size_t)0 );
	if( *resp == NULL ) 
		return -1;
	return 0;

}

