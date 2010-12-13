#ifndef ARG_I386_RESTART_FIXUP_H
#define ARG_I386_RESTART_FIXUP_H

// TODO: Use vsdo attribute of mm_context to check for correct location of vsdo
// TODO: Disable making of checkpoints, when we are in some part of vsdo other than syscall itself (otherwise we have to fixup stack & args for every possible EIP)

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

	mdbg(INFO3, "Calling tcmi sys restart. Thread Flags: %08lX EAX: %08lX (OEAX: %08lX Return EAX: %08lX) SP: %16lX BP: %16lX (Restore BP: %16lX) IP: %08lX (Restore IP: %08lX)", current_thread_info()->flags, regs_return_value(regs), original_ax(regs), restart->arg0, stack_pointer(regs), base_pointer(regs), restart->arg2, regs->ip, restart->arg1);
	/*mdbg(INFO3, "Calling tcmi sys restart. EAX: %08lX (OEAX: %08lX Return EAX: %08lX). EBX: %08lX ECX: %08lX EDX: %08lX", regs->eax, regs->orig_eax, restart->arg0, regs->ebx, regs->ecx, regs->edx);*/

	/* TODO: Reset restart_block? */

	/* Reset EIP to original value (either again just before the syscall, or on the other completely unrelated address */
	regs->ip = restart->arg1;			
	
	/* Restore ebp, in case we were in syscall it won't have any effect, otherwise it will set proper EBP for the application */
	base_pointer(regs) = restart->arg2;

	/* To prevent ECX & EDX clobbering via sysexit */
	set_thread_flag(TIF_IRET);
	/* 
           This is a trick how to get original EAX onto the stack.. return value of this syscall is what gets onto the stack 
	   When the syscall gets executed again (as we again set EIP) it will finally use correct EAX. The other registers are unclobbered and so we do not need to restore them
	   If we were not originally in syscall, we will simply continue with correct EAX where we finished.
         */
	return restart->arg0;
}

/** 
 * If we were in 64 bit syscall, we have to fixup some regs to work correctly after resume to user space.
 */
static inline void fixup_64bit_stack(struct restart_block* restart, struct pt_regs* regs)  {
	unsigned long stackEbp;
	mdbg(INFO4, "64bit stack");
	/* Avoid all poping after syscall as we have a different stack */
	restart->arg1 = 0xffffe413;			
	/* Extract ebp stored on top of the stack */
	stackEbp = *((unsigned long*)(regs->sp));
	/* Fixup both pointers to ebp */
	regs->bp = stackEbp;
	restart->arg2 = stackEbp;
	/* Fixup stack length (unpopped ebp; ecx & edx were not pushed on stack by 64 bit) */
	regs->sp += 4;
}

/**
 * Registeres restart block and schedules restart syscall to be executed after the process resumes userspace for first time. The main motivaction for this is to 
 * restore original AX register.
 *
 * @param task Current task
 * @param regs Current registers of the task
 * @param is_32bit_application 1, if application is 32 bit
 * @return 0 on success
 */
static int tcmi_resolve_restart_block(struct task_struct* task, struct pt_regs* regs, enum arch_ids from_arch, int is_32bit_application) {
	struct restart_block* restart = &(task_thread_info(task)->restart_block);
	mdbg(INFO3, "Setting tcmi sys restart block. EAX: %08lX (OEAX: %08lX). EBX: %08lX ECX: %08lX EDX: %08lX", regs->ax, regs->orig_ax, regs->bx, regs->cx, regs->dx);	
	/* EAX and EIP are required to be stored, the other registers won't get affected as they are already on stack (eax will be overriden by execve return value and EIP will be altered as the userspace process will execute restart syscall) */
	restart->arg0 = original_ax(regs);
	restart->arg1 = instruction_pointer(regs);	

	/* 
           EBP needs to be restored in case we were not in syscall, because in that case we execute fake syscall to get EAX on place and this fake vsyscall will clobber EBP. If we were originally in syscall,
	   this won't matter, because the real EBP was pushed before call and will be poped after, but in case we were not in syscall, we do not have this comfort
	   In order to make restart code easier, we simply store and restore EBP in both cases.
	*/
	restart->arg2 = base_pointer(regs);
	
	if ( !is_in_syscall(regs) ) {
		mdbg(INFO3, "Performing restart out of syscall.");		
		/* Set current EIP address just before the sysenter so that the restart block is executed */
		regs->ip = 0xffffe403;
		/* In addition if we are not in syscall we do not want to use orig_eax, but rather real eax */
		restart->arg0 = regs_return_value(regs);

		if ( from_arch == ARCH_X86_64 && restart->arg1 == 0xffffe405 ) {
			/* We were in a 64 bit syscall, but it was not interrupted */
			fixup_64bit_stack(restart, regs);
		}
	} else {		
		regs->ip -= 2;
		mdbg(INFO3, "Performing restart in syscall");		
		if ( from_arch == ARCH_X86_64 && restart->arg1 == 0xffffe405) { // We were in 64 bit syscall of AMD => we need fixup
			fixup_64bit_stack(restart, regs);
		}
	}

	restart->fn = tcmi_restart_fixup;

	return 0;
}

#endif
