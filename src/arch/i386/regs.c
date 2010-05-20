#include "regs.h"

#if defined(__i386__)
#include <dbg.h>

void* get_target_platform_registers(struct pt_regs* regs, enum arch_ids target_arch) {
	if ( target_arch == ARCH_CURRENT ) {
		struct tcmi_regs* result_regs = kmalloc(sizeof(struct tcmi_regs), GFP_KERNEL);
		if ( !result_regs )
			return NULL;

		get_registers(regs, result_regs);
		return result_regs;
	}

	if ( target_arch == ARCH_X86_64 ) {
		struct tcmi_x86_64_regs* result_regs = kmalloc(sizeof(struct tcmi_x86_64_regs), GFP_KERNEL);
		if ( !result_regs )
			return NULL;

		result_regs->orig_rax = regs->orig_ax;;
		result_regs->rax = regs->ax;
		result_regs->rbx = regs->bx;
		result_regs->rcx = regs->cx;
		result_regs->rdx = regs->dx;

		result_regs->rsi = regs->si;
		result_regs->rdi = regs->di;

		result_regs->ss = regs->ss;
		result_regs->cs = regs->cs;
		result_regs->rip = regs->ip;
		result_regs->eflags = regs->flags;
		result_regs->rbp = regs->bp;
		result_regs->rsp = regs->sp;

		result_regs->r8 = 0;
		result_regs->r9 = 0;
		result_regs->r10 = 0;
		result_regs->r11 = 0;
		result_regs->r12 = 0;
		result_regs->r13 = 0;
		result_regs->r14 = 0;
		result_regs->r15 = 0;

		return result_regs;
	}

	mdbg(ERR3, "Unsupported target platform: %d", target_arch);

	return NULL;
};

void load_from_platform_registers(void* platform_registers, struct pt_regs* regs, enum arch_ids from_arch, int is_32bit_app) {
	if ( from_arch == ARCH_CURRENT ) {
		/** Simple load of compatible registers */
		load_registers( (struct tcmi_regs*)platform_registers, regs);
		return;
	}

	if ( from_arch == ARCH_X86_64) {
		struct tcmi_x86_64_regs* tcmi_regs = (struct tcmi_x86_64_regs*)platform_registers;
		regs->orig_ax = tcmi_regs->orig_rax;;
		regs->ax = tcmi_regs->rax;
		regs->bx = tcmi_regs->rbx;
		regs->cx = tcmi_regs->rcx;
		regs->dx = tcmi_regs->rdx;

		regs->si = tcmi_regs->rsi;
		regs->di = tcmi_regs->rdi;

		regs->ss = tcmi_regs->ss;
		regs->cs = tcmi_regs->cs;
		regs->ip = tcmi_regs->rip;
		regs->flags = tcmi_regs->eflags;
		regs->bp = tcmi_regs->rbp;
		regs->sp = tcmi_regs->rsp;

		/* TODO: How to restore fs and es? It is not stored on 64 stack.. */
		regs->fs = 0;
		regs->es = 0;

		return;
	}


	mdbg(ERR3, "Unsupported source platform: %d", from_arch);
};

int is_in_syscall(struct pt_regs* regs) {
	return ((regs->orig_ax >= 0) &&
			   (regs->ax == -ERESTARTNOHAND ||
			    regs->ax == -ERESTARTSYS ||
			    regs->ax == -ERESTART_RESTARTBLOCK ||
			    regs->ax == -ERESTARTNOINTR));
};

void debug_registers(struct pt_regs* regs) {
	mdbg(INFO4, "ESP  : %08lx    EBP : %08lx",regs->sp,regs->bp);
	mdbg(INFO4, "ESI  : %08lx    EDI : %08lx",regs->si,regs->di);
	mdbg(INFO4, "CS   : %08lx    DS  : %08lx", regs->cs,regs->ds);
	mdbg(INFO4, "EIP  : %08lx    OEAX: %08lX",regs->ip,regs->orig_ax);
	mdbg(INFO4, "EAX  : %08lx    EBX : %08lX",regs->ax,regs->bx);
	mdbg(INFO4, "ECX  : %08lx    EDX : %08lX",regs->cx,regs->dx);
};

#endif
