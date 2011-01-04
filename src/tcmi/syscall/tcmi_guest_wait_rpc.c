/**
 * @file tcmi_guest_wait_rpc.c - guest side part of wait syscall
 */

#include <dbg.h>
#include <tcmi/comm/tcmi_rpcresp_procmsg.h>
#include <linux/uaccess.h>

#include <arch/types.h>
#include "tcmi_guest_wait_rpc.h"

/** sys_wait4 system call RPC 
 * @param rpc_num - RPC number
 * @param params  - pointer to array with RPC parameters - pid, stat_addr, options, rusage
 *
 * @return RPC return value
 * */
TCMI_GUEST_RPC_CALL_DEF(wait4)
{
	struct tcmi_msg *r; long rtn;
	// platform independend params
	int32_t pid_ind, options_ind;
	// Real results
	struct rusage result_usage;
	int result_stats;
	// Platform independend results
	struct rusage_ind* result_usage_ind;
	int32_t result_stats_ind;

	pid_ind = params[0];
	options_ind = params[2];

	mdbg(INFO3, "Forwarding wait syscall. Wait pid: %d", pid_ind);

	r = tcmi_rpcresp_procmsg_get_response(rpc_num, 
			sizeof(int32_t), &pid_ind, // pid
			sizeof(int32_t), &options_ind, // options
			(size_t)0); 

	if(r == NULL){
		minfo(ERR3, "Response message hasn't arrived");
		return -EAGAIN;
	}
	
	// Extracts results in platform independed form
	result_usage_ind = (struct rusage_ind*)tcmi_rpcresp_procmsg_data_base( TCMI_RPCRESP_PROCMSG(r), 0);
	result_stats_ind = *(int32_t*)tcmi_rpcresp_procmsg_data_base( TCMI_RPCRESP_PROCMSG(r), 1);	

	// Converts them to current platform
	rusage_from_ind(result_usage_ind, &result_usage);
	result_stats = result_stats_ind;

	// Copies them to a user space provided buffer
	if ( (int __user*)params[1] != NULL )
		put_user(result_stats, (int __user*)params[1]);

	if ( (void*)params[3] != NULL ) {
		copy_to_user((struct rusage __user*)params[3], &result_usage, sizeof(result_usage));
	}	

	rtn = tcmi_rpcresp_procmsg_rtn( TCMI_RPCRESP_PROCMSG(r) );
	tcmi_msg_put(r);

	mdbg(INFO3, "Wait syscall returned: %ld. Status: %d (Independend: %d)", rtn, result_stats, result_stats_ind);
	return rtn;
}
