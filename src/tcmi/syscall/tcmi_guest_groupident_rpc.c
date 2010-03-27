/**
 * @file tcmi_guest_rpc_useridnt.c - shadow part of proxyfs rpc
 *
 * Date: 7/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_guest_groupident_rpc.c,v 1.2 2007/09/02 13:54:30 stavam2 Exp $
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

#include "tcmi_guest_groupident_rpc.h"


/** Definition of tcmi_shadow_rpc_sys_getgroups() 
 * @param rpc_num  - RPC number (TCMI_RPC_SYS_GETGROUPS)
 * @param params[] - [0] Buffer size <br> - [1] Pointer to buffer in userspace with checked access
 *
 * @return bytes written
 *
 */
TCMI_GUEST_RPC_CALL_DEF(getgroups)
{
	struct tcmi_msg *r; long rtn; long size;

	r = tcmi_rpcresp_procmsg_get_response(
			rpc_num, sizeof(long), params, 0);

	if(r == NULL){
		mdbg(ERR3, "Response message hasn't arrived");
		return -EAGAIN;
	}

	rtn = tcmi_rpcresp_procmsg_rtn( TCMI_RPCRESP_PROCMSG(r) );
	if( rtn > 0 && params[0] > 0 ){
		size = tcmi_rpcresp_procmsg_data_size( TCMI_RPCRESP_PROCMSG(r), 0 );
		__copy_to_user((void*)params[1], tcmi_rpcresp_procmsg_data_base( TCMI_RPCRESP_PROCMSG(r), 0 ), 
				size*sizeof(gid_t));
	}
	tcmi_msg_put(r);
	return rtn;
}

/** Definition of tcmi_shadow_rpc_sys_setgroups() 
 * @param rpc_num  - RPC number (TCMI_RPC_SYS_GETGROUPS)
 * @param params[] - [0] Buffer size <br> - [1] Pointer to buffer in userspace with checked access
 *
 * @return bytes written
 *
 */
TCMI_GUEST_RPC_CALL_DEF(setgroups)
{
	struct tcmi_msg *r; long rtn, size;
	void *ker_buf;

	size = sizeof(gid_t) * params[0];

	if( (ker_buf = kmalloc( size, GFP_KERNEL )) == NULL ){
		mdbg(ERR3, "Buffer allocation failed");
		return -EAGAIN;
	}

	__copy_from_user( ker_buf, (void*)params[1], size );
	r = tcmi_rpcresp_procmsg_get_response(
			rpc_num, sizeof(long), params, size, ker_buf, 0);

	if(r == NULL){
		mdbg(ERR3, "Response message hasn't arrived");
		return -EAGAIN;
	}

	rtn = tcmi_rpcresp_procmsg_rtn( TCMI_RPCRESP_PROCMSG(r) );
	tcmi_msg_put(r);
	return rtn;
}

/** Definition of tcmi_shadow_rpc_sys_setresgid() 
 * @param rpc_num   - RPC number (TCMI_RPC_SYS_GETRESGID)
 * @param params[] - [0] Pointer to rgid <br> - [1] Pointer to egid <br> - [2] Pointer to sgid
 *
 * @return zero on success
 * */
TCMI_GUEST_RPC_CALL_DEF(getresgid)
{
	struct tcmi_msg *r; long rtn, *resp_data;
	r = tcmi_rpcresp_procmsg_get_response(
			rpc_num, 0);

	if(r == NULL){
		mdbg(ERR3, "Response message hasn't arrived");
		return -EAGAIN;
	}

	rtn = tcmi_rpcresp_procmsg_rtn( TCMI_RPCRESP_PROCMSG(r) );
	if( rtn == 0 ){
		resp_data = tcmi_rpcresp_procmsg_data_base( TCMI_RPCRESP_PROCMSG(r), 0 );
		put_user( (gid_t)resp_data[0], (gid_t*)params[0] );
		put_user( (gid_t)resp_data[1], (gid_t*)params[1] );
		put_user( (gid_t)resp_data[2], (gid_t*)params[2] );
	}
	tcmi_msg_put(r);
	return rtn;
}

