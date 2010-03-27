#ifndef ARG_X86_64_RESTART_FIXUP_H
#define ARG_X86_64_RESTART_FIXUP_H

// TODO: Use vsdo attribute of mm_context to check for correct location of vsdo. It is not really supported by 64 bit now, but 32 bit kernel is using it and we must be able to handle this case also here!


// TODO: Disable making of checkpoints, when we are in some part of vsdo other than syscall itself (otherwise we have to fixup stack & args for every possible EIP)

#include <asm/processor.h>

/** We need to export this in order to prepare vsyscall page for 32 bit applications */
extern int syscall32_setup_pages(struct linux_binprm *bprm, int exstack);

/** 
 * This method is used for handling migration restart. There are 2 cases:
 * 
 * Application was in syscall:
 *  We cannot directly restart the syscall, because as a part of process restart we do execve. The execve,
 * if it succeeds, will set EAX to 0. This means, the restarted syscall will not be the original syscall, but sys_restartsyscall.
 * Restart syscall is handled by the custom registered method in our case this method.
 *
 * Application was not in syscall:
 *  We cannot simply restart the application, because again the execve will override EAX content! Here we use a special trick.
 * We can assume EAX will contain 0 as in previous case (=execve succeeded, otherwise the program would not start anyway).
 * With EAX == 0 we can set fake EIP just before the vsyscall sysenter (assuming it is mapped in the process on expected address)
 * This way we will get sysretart called immediately and from the special restart function we return proper eax (and eip) in order to make 
 * the application working as expected, with all registers set correctly.
 */
static long tcmi_restart_fixup(struct restart_block *restart) {
	struct pt_regs *regs = task_pt_regs(current);

	mdbg(INFO3, "Calling tcmi sys restart. EAX: %08lX (OEAX: %08lX Return EAX: %08lX) SP: %16lX BP: %16lX (Restore BP: %16lX)", regs_return_value(regs), original_ax(regs), restart->arg0, stack_pointer(regs), base_pointer(regs), restart->arg2);
	/*mdbg(INFO3, "Calling tcmi sys restart. EAX: %08lX (OEAX: %08lX Return EAX: %08lX). EBX: %08lX ECX: %08lX EDX: %08lX", regs->eax, regs->orig_eax, restart->arg0, regs->ebx, regs->ecx, regs->edx);*/

	/* TODO: Reset restart_block? */

	/* Reset EIP to original value (either again just before the syscall, or on the other completely unrelated address */
	regs->ip = restart->arg1;			
	
	/* Restore rcx */
	regs->cx = restart->arg2;

	/* To prevent ECX & EDX clobbering via sysexit */
	set_thread_flag(TIF_IRET);
	/* 
           This is a trick how to get original EAX onto the stack.. return value of this syscall is what gets onto the stack 
	   When the syscall gets executed again (as we again set EIP) it will finally use correct EAX. The other registers are unclobbered and so we do not need to restore them
	   If we were not originally in syscall, we will simply continue with correct EAX where we finished.
         */
	return restart->arg0;
}

/** 64 bit apps do not need to use restart block.. TODO: Do this kernel patch also for 32 bit apps? */
static inline int tcmi_resolve_restart_block_64bit_app(struct task_struct* task, struct pt_regs* regs) {
	if ( is_in_syscall(regs) ) {
		mdbg(INFO3, "Performing restart in 64 bit syscall.");
		regs->ip -= 2;
		regs->ax = regs->orig_ax;
	} else {
		// For 64 bit applications we have a modified execve and we use orig_rax to set application rax, so we do not need to do restart trick
		// Do this only if we are not in syscall, otherwise we want current orig_eax to get into eax register
		regs->orig_ax = regs->ax;			
	}
	return 0;
}

/** 32 bit apps do not need to use restart block */
static inline int tcmi_resolve_restart_block_32bit_app(struct task_struct* task, struct pt_regs* regs, enum arch_ids from_arch) {
	if ( is_in_syscall(regs) ) {
		mdbg(INFO3, "Performing restart in 32 bit syscall.");
		regs->ip -= 2;
		regs->ax = regs->orig_ax;

		if ( boot_cpu_data.x86_vendor == X86_VENDOR_AMD) { // Fixup needed on AMD, as it does not support syscall in compat mode
			regs->sp += 8; /* Fixup stack pointer.. */
			*((unsigned int*)(regs->sp)) = regs->bp; // Fixup stack content.. this will be pushed from stack
			regs->bp = regs->cx; // This is normally done by 64 bit syscall, so we have to do it manually
		}
	} else {
		// For 64 bit applications we have a modified execve and we use orig_rax to set application rax, so we do not need to do restart trick
		// Do this only if we are not in syscall, otherwise we want current orig_eax to get into eax register
		regs->orig_ax = regs->ax;	

		if ( regs->ip == 0xffffe410 && boot_cpu_data.x86_vendor == X86_VENDOR_AMD) { // Fixup needed on AMD, as it does not support syscall in compat mode
			mdbg(INFO3, "Performing restart in finished 32 bit syscall.");
			/* We were in syscall, but the syscall was not interrupted and so we do not need to restart it */
			regs->bp = *((unsigned int*)(regs->sp)); // Extract real RBP
			mdbg(INFO3, "Real rbp was: %08lx.", regs->bp);
			regs->sp += 8; /* Fixup stack pointer.. */
			*((unsigned int*)(regs->sp)) = regs->bp; // Fixup stack content.. this will be pushed from stack
			regs->bp = regs->cx; // This is normally done by 64 bit syscall, so we have to do it manually
			regs->ip = 0xffffe405; // Modify return address to correct value
		}				
	}
	return 0;
}

/**
 * Registeres restart block and schedules restart syscall to be executed after the process resumes userspace for first time. The main motivaction for this is to 
 * restore original AX register.
 *
 * @param task Current task
 * @param regs Current registers of the task
 * @param is_32bit_application 1 if, the application is 32 bit
 * @return 0 on success
 */
static inline int tcmi_resolve_restart_block(struct task_struct* task, struct pt_regs* regs, enum arch_ids from_arch, int is_32bit_application) {
	//struct restart_block* restart = &task->thread_info->restart_block;
	mdbg(INFO3, "Setting tcmi sys restart block. EAX: %08lX (OEAX: %08lX). EBX: %08lX ECX: %08lX EDX: %08lX", regs->ax, regs->orig_ax, regs->bx, regs->cx, regs->dx);	

	if ( !is_32bit_application )
		return tcmi_resolve_restart_block_64bit_app(task, regs);

	/* First we have to setup syscall page into 32 process space so that it can do vsyscalls */
	if ( syscall32_setup_pages(NULL,0) ) {
		mdbg(ERR3, "Failed to install syscall page");
		return -EINVAL;
	}
	
	return tcmi_resolve_restart_block_32bit_app(task, regs, from_arch);

	//return 0;
}

#endif
