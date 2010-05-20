/**
 * @file tcmi_shadow_wait_rpc.c - shadow part of wait syscall rpc
 */
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/resource.h>

#include <arch/types.h>
#include <dbg.h>

#include "tcmi_shadow_wait_rpc.h"
#include "exported_symbols.h"

TCMI_SHADOW_RPC_GENERIC_CALL_DEF(sys_wait4)
{
	struct tcmi_msg **resp, *m;
	long rtn;
	mm_segment_t old_fs;

	// Platform independend params
	int32_t pid;
	int32_t options;
	// Platform independend return values
	struct rusage_ind* result_usage_ind;
	int32_t* result_stats_ind;	
	// Return values	
	struct rusage* result_usage;
	int result_stats;
	
	mdbg(INFO3, "Forwarded wait syscall being processed");

	result_usage = kmalloc(sizeof(*result_usage), GFP_KERNEL);
	if ( !result_usage )
		return -ENOMEM;


	m = (struct tcmi_msg*) params[0];
	resp = (struct tcmi_msg**) params[1];
	// Extract params
	pid = *(int32_t*)tcmi_rpc_procmsg_data_base( TCMI_RPC_PROCMSG(m), 0);
	options = *(int32_t*)tcmi_rpc_procmsg_data_base( TCMI_RPC_PROCMSG(m), 1);

	mdbg(INFO3, "Waiting for pid: %d", pid);

	// Perform the call
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	rtn = sys_wait4(pid, &result_stats , options, result_usage);
	set_fs(old_fs);

	// Convert resuls to platform independend values
	result_usage_ind = kmalloc(sizeof(*result_usage_ind), GFP_KERNEL);
	if ( !result_usage_ind ) {
		kfree(result_usage);
		return -ENOMEM;
	}

	result_stats_ind = kmalloc(sizeof(int32_t), GFP_KERNEL);
	if ( !result_stats_ind ) {
		kfree(result_usage);
		kfree(result_usage_ind);
		return -ENOMEM;
	}

	rusage_to_ind(result_usage, result_usage_ind);
	*result_stats_ind = result_stats;

	mdbg(INFO3, "Forwarded wait syscall wait finished. Status: %d (ind: %d) res: %ld", result_stats, *result_stats_ind, rtn);
	
	// Create response with the results
	*resp = tcmi_rpcresp_procmsg_create(m, rtn, sizeof(*result_usage_ind), result_usage_ind,
						    sizeof(*result_stats_ind), result_stats_ind,
						(size_t)0 );

	kfree(result_usage);
	if( *resp == NULL ) {
		kfree(result_usage_ind);
		kfree(result_stats_ind);
		return -1;
	}

	tcmi_rpcresp_procmsg_free_on_put( TCMI_RPCRESP_PROCMSG(*resp), 0, TCMI_RPCRESP_PROCMSG_KFREE );
	tcmi_rpcresp_procmsg_free_on_put( TCMI_RPCRESP_PROCMSG(*resp), 1, TCMI_RPCRESP_PROCMSG_KFREE );

	return 0;
}

