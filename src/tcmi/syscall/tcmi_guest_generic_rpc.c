/**
 * @file tcmi_guest_generic_rpc.c - PEN part of RPCs
 *
 *
 * Date: 7/05/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_guest_generic_rpc.c,v 1.2 2007/10/20 14:24:20 stavam2 Exp $
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
#include <tcmi/comm/tcmi_rpc_procmsg.h>
#include <tcmi/comm/tcmi_rpcresp_procmsg.h>

#include "tcmi_guest_rpc.h"

#define TCMI_GUEST_RPC_SYS_N(arg) \
long tcmi_guest_rpc_sys_##arg(unsigned rpc_num, long params[TCMI_RPC_MAXPARAMS]){\
	struct tcmi_msg *r; long ret;\
	if(arg>0) r = tcmi_rpcresp_procmsg_get_response(rpc_num, (size_t)((arg)*sizeof(long)), params, (size_t)0);\
	else r = tcmi_rpcresp_procmsg_get_response(rpc_num, (size_t)0);\
	if ( r == NULL ) {\
		mdbg(ERR3, "Response message hasn't arrived");\
		return -EAGAIN;\
	}\
	ret = tcmi_rpcresp_procmsg_rtn(TCMI_RPCRESP_PROCMSG(r));\
	tcmi_msg_put(r);\
	return ret;\
}

/** Generic guest RPC method for call with no parameters 
 * @param rpc_num - RPC number
 * @param params  - pointer to array with RPC parameters
 *
 * @return RPC return value
 * */
TCMI_GUEST_RPC_SYS_N(0)

/** Generic guest RPC method for call with one parameters 
 * @param rpc_num - RPC number
 * @param params  - pointer to array with RPC parameters
 *
 * @return RPC return value
 * */
TCMI_GUEST_RPC_SYS_N(1)

/** Generic guest RPC method for call with two parameters 
 * @param rpc_num - RPC number
 * @param params  - pointer to array with RPC parameters
 *
 * @return RPC return value
 * */
TCMI_GUEST_RPC_SYS_N(2)

/** Generic guest RPC method for call with three parameters 
 * @param rpc_num - RPC number
 * @param params  - pointer to array with RPC parameters
 *
 * @return RPC return value
 * */
TCMI_GUEST_RPC_SYS_N(3)

/**
 * @}
 */

