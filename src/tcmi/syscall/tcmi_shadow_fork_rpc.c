/**
 * @file tcmi_shadow_fork_rpc.c - shadow part of fork syscall rpc
 */
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/resource.h>

#include <arch/types.h>
#include <dbg.h>

#include "tcmi_shadow_fork_rpc.h"
#include "exported_symbols.h"
#include <arch/current/regs.h>

TCMI_SHADOW_RPC_GENERIC_CALL_DEF(do_fork)
{
	struct tcmi_msg **resp, *m;
	long rtn;
	mm_segment_t old_fs;
	// We can use current regs... it really does not matter as the process should immediately go to migration mode and if it ever get's out, it will use registers from a checkpoint
	struct pt_regs *regs = task_pt_regs(current);

	// Platform independend params
	uint64_t clone_flags, stack_start, stack_size;

	// Platform independend return values
	int32_t* parent_tid_ind, *child_tid_ind;
	// Return values	
	int parent_tid, child_tid;
	
	mdbg(INFO3, "Forwarded fork syscall being processed");

	m = (struct tcmi_msg*) params[0];
	resp = (struct tcmi_msg**) params[1];
	// Extract params
	clone_flags = *(uint64_t*)tcmi_rpc_procmsg_data_base( TCMI_RPC_PROCMSG(m), 0);
	stack_start = *(uint64_t*)tcmi_rpc_procmsg_data_base( TCMI_RPC_PROCMSG(m), 1);
	stack_size = *(uint64_t*)tcmi_rpc_procmsg_data_base( TCMI_RPC_PROCMSG(m), 2);

	// We disable VFORK clonning on core node.. detached node is waiting
	// TODO: This is not really good solution, because when detached node child migrates away, the waiting is broken.
	// We should introduce some rpc message VFORK done, and the guest parent should be waiting for that (Even if the parent
	// itself migrates away and becomes another guest, or ordinary task on CCN, it should first wait for this VFORK done event)
	clone_flags &= ~CLONE_VFORK;

	mdbg(INFO3, "Forwarded fork params: Clone flags: %lx Start stack: %lx, Stack size: %lu", (unsigned long)clone_flags, (unsigned long)stack_start, (unsigned long)stack_size);

	// Perform the call
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	// TODO: Check if passing parent & child is correct here
	rtn = do_fork(clone_flags, stack_start, regs, stack_size, &parent_tid, &child_tid);
	set_fs(old_fs);

	mdbg(INFO3, "Forwarded fork syscall finished.");

	// Convert results to platform independend values
	parent_tid_ind = kmalloc(sizeof(*parent_tid_ind), GFP_KERNEL);
	if ( !parent_tid_ind ) {
		return -ENOMEM;
	}

	child_tid_ind = kmalloc(sizeof(*child_tid_ind), GFP_KERNEL);
	if ( !child_tid_ind ) {
		kfree(parent_tid_ind);
		return -ENOMEM;
	}

	*parent_tid_ind = parent_tid;
	*child_tid_ind = child_tid;	
	
	// Create response with the results
	*resp = tcmi_rpcresp_procmsg_create(m, rtn, sizeof(*parent_tid_ind), parent_tid_ind,
						    sizeof(*child_tid_ind), child_tid_ind,
						(size_t)0 );

	if( *resp == NULL ) {
		kfree(parent_tid_ind);
		kfree(child_tid_ind);
		return -1;
	}

	tcmi_rpcresp_procmsg_free_on_put( TCMI_RPCRESP_PROCMSG(*resp), 0, TCMI_RPCRESP_PROCMSG_KFREE );
	tcmi_rpcresp_procmsg_free_on_put( TCMI_RPCRESP_PROCMSG(*resp), 1, TCMI_RPCRESP_PROCMSG_KFREE );

	return 0;
}
