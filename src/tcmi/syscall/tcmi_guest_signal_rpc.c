/**
 * @file tcmi_guest_signal_rpc.c - shadow part of proxyfs rpc
 *
 * Date: 7/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_guest_signal_rpc.c,v 1.2 2007/10/11 21:00:26 stavam2 Exp $
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

#include <dbg.h>
#include <tcmi/comm/tcmi_rpcresp_procmsg.h>
#include <linux/uaccess.h>

#include "tcmi_guest_signal_rpc.h"

/** Rt_sigqueueinfo system call RPC 
 * @param rpc_num - RPC number
 * @param params  - pointer to array with RPC parameters - PID, signal and pointer to siginfo structure
 *
 * @return RPC return value
 * */
TCMI_GUEST_RPC_CALL_DEF(rt_sigqueueinfo)
{
	struct tcmi_msg *r; long rtn;
	r = tcmi_rpcresp_procmsg_get_response(rpc_num, 
			2*sizeof(long), params,
			sizeof(siginfo_t), (void*)params[2], 0); // TODO: It was meant like that, not like below, right? (m.s.)
			//sizeof(siginfo_t), (void*)params[3], 0);

	if(r == NULL){
		mdbg(ERR3, "Response message hasn't arrived");
		return -EAGAIN;
	}

	rtn = tcmi_rpcresp_procmsg_rtn( TCMI_RPCRESP_PROCMSG(r) );
	tcmi_msg_put(r);
	return rtn;
}

