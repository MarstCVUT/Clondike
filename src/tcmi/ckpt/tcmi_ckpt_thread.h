#ifndef _TCMI_CKPT_THREAD_H
#define _TCMI_CKPT_THREAD_H

#include "tcmi_ckpt.h"
#include <arch/current/thread.h>

#include <asm/desc.h>

static inline int tcmi_ckpt_tls_write(struct tcmi_ckpt *ckpt, struct task_struct *task) {
	struct tcmi_thread tcmi_thread;
	int32_t length;
	
	get_tcmi_thread(&task->thread, &tcmi_thread);

	length = sizeof(tcmi_thread);
	if (tcmi_ckpt_write(ckpt, &length, sizeof(length)) < 0) {
		mdbg(ERR3, "Error writing thread length");
		goto exit0;
	}	

	/* write the header into the checkpoint */
	if (tcmi_ckpt_write(ckpt, &tcmi_thread, sizeof(struct tcmi_thread)) < 0) {
		mdbg(ERR3, "Error writing thread data");
		goto exit0;
	}
/*	mdbg(INFO4, "Written tls:");
	mdbg(INFO4, "GS   : %08lx     FS  : %08lx", task->thread.gs, task->thread.fs);
*/
	return 0;
		
	/* error handling */
 exit0:
	return -EINVAL;

}


static inline int tcmi_ckpt_tls_read(struct tcmi_ckpt *ckpt, struct task_struct *task, struct pt_regs *regs)
{
	int cpu;
	
	int32_t length;
	void* platform_thread;

	if (tcmi_ckpt_read(ckpt, &length, sizeof(length)) < 0) {
		mdbg(ERR3, "Error reading thread length");
		goto exit0;
	}

	platform_thread = kmalloc(length, GFP_KERNEL);
	if ( !platform_thread ) {
		mdbg(ERR3, "Not enough memory for thread struct");
		goto exit0;
	}

	if (tcmi_ckpt_read(ckpt, platform_thread, length) < 0) {
		mdbg(ERR3, "Error reading process register descriptor header");
		goto exit1;
	}
	
	load_from_platform_thread(platform_thread, &task->thread, ckpt->hdr.checkpoint_arch);

	/* Here we restore a bits of the thread_struct. We cannot copy it whole as there are some parts not-transferable (which?:).. so here we copy just what we know we need */	
	cpu = get_cpu();
	resolve_TLS(&task->thread, cpu, regs);
	put_cpu();

	/* TODO: Resolve FPU with 64 bit in mind! */
	/* Restore FPU state */
	//memcpy(&task->thread.i387, &tls.thread.i387, sizeof(task->thread.i387));
	/* Set TS flag of CR0 in order to enable lazy load of FPU state when required */
	//stts();

	kfree(platform_thread);
	return 0;
		
	/* error handling */
 exit1:
	kfree(platform_thread);
 exit0:
	return -EINVAL;
}


#endif
