/**
 * @file tcmi_guest_fork_rpc.c - guest side part of fork syscall
 */

#include <dbg.h>
#include <tcmi/comm/tcmi_rpcresp_procmsg.h>
#include <linux/uaccess.h>

#include <arch/types.h>
#include "tcmi_guest_fork_rpc.h"

/** 
 * All version of system call based on fork RPC 
 * Called in the beginning of do_fork method
 *
 * @param rpc_num - RPC number
 * @param params  - pointer to array with RPC parameters - clone_flags, stack_start, regs, stack_size, parent_tidptr, child_tidptr
 *
 * @return RPC return value
 * */
TCMI_GUEST_RPC_CALL_DEF(fork)
{
	struct tcmi_msg *r; 
	long rtn;
	// platform independend params
	uint64_t clone_flags, stack_start, stack_size;

	// Real results
	int parent_tid, child_tid;

	clone_flags = params[0];
	stack_start = params[1];
	stack_size  = params[3];

	mdbg(INFO3, "Forwarding fork syscall. Ptid filled: %d Ctid filled: %d", (void*)params[4] != NULL, (void*)params[5] != NULL);

	r = tcmi_rpcresp_procmsg_get_response(rpc_num, 
			sizeof(uint64_t), &clone_flags, 
			sizeof(uint64_t), &stack_start, 
			sizeof(uint64_t), &stack_size, 
			(size_t)0); 

	if(r == NULL){
		mdbg(ERR3, "Response message hasn't arrived");
		return -EAGAIN;
	}
	
	// Extracts results in platform independ form and convert them
	parent_tid = *(int32_t*)tcmi_rpcresp_procmsg_data_base( TCMI_RPCRESP_PROCMSG(r), 0);
	child_tid = *(int32_t*)tcmi_rpcresp_procmsg_data_base( TCMI_RPCRESP_PROCMSG(r), 1);	

	rtn = tcmi_rpcresp_procmsg_rtn( TCMI_RPCRESP_PROCMSG(r) );

	if ( rtn > 0 ) { // Perform this only in case fork succeeded!		
		// Copies them to a user space provided buffer	
/* TODO: THis is not correct think to do.. the value needs to be set "lazily"
		if ( (void*)params[4] != NULL ) {
			put_user(parent_tid, (int __user*)params[4]);		
		}	
		if ( (void*)params[5] != NULL ) {
			put_user(child_tid, (int __user*)params[5]);
		}	
*/
	}
	
	tcmi_msg_put(r);

	mdbg(INFO3, "Fork syscall returned: %ld. Parent tid: %d Child tid: %d", rtn, parent_tid, child_tid);
	return rtn;
}
