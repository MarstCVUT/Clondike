#include "regs.h"

#include <dbg.h>

#if defined(__x86_64__)
#include <arch/i386/regs.h>

void* get_target_platform_registers(struct pt_regs* regs, enum arch_ids target_arch) {
	if ( target_arch == ARCH_CURRENT ) {
		struct tcmi_regs* result_regs = kmalloc(sizeof(struct tcmi_regs), GFP_KERNEL);
		if ( !result_regs )
			return NULL;

		get_registers(regs, result_regs);
		return result_regs;
	}

	if ( target_arch == ARCH_I386 ) {
		struct tcmi_i386_regs* result_regs = kmalloc(sizeof(struct tcmi_i386_regs), GFP_KERNEL);
		if ( !result_regs )
			return NULL;

		result_regs->orig_eax = regs->orig_ax;;
		result_regs->eax = regs->ax;
		result_regs->ebx = regs->bx;
		result_regs->ecx = regs->cx;
		result_regs->edx = regs->dx;

		result_regs->esi = regs->si;
		result_regs->edi = regs->di;

		result_regs->xss = regs->ss;
		result_regs->xcs = regs->cs;
		result_regs->eip = regs->ip;
		result_regs->eflags = regs->flags;
		result_regs->ebp = regs->bp;
		result_regs->esp = regs->sp;

		/* TODO: How to restore fs and es? It is not stored on 64 stack.. */
		result_regs->xfs = 0;
		result_regs->xes = 0;

		return result_regs;
	}

	mdbg(ERR3, "Unsupported target platform: %d", target_arch);

	return NULL;
};

void load_from_platform_registers(void* platform_registers, struct pt_regs* regs, enum arch_ids from_arch, int is_32bit_app) {
	if ( from_arch == ARCH_CURRENT ) {
		/** Simple load of equal registers */
		load_registers( (struct tcmi_regs*)platform_registers, regs);

		if ( is_32bit_app ) {
			regs->cs = __USER32_CS;		
			regs->ss = __USER32_DS;
		}

		return;
	}

	if ( from_arch == ARCH_I386) {
		struct tcmi_i386_regs* tcmi_regs = (struct tcmi_i386_regs*)platform_registers;
		regs->orig_ax = tcmi_regs->orig_eax;;
		regs->ax = tcmi_regs->eax;
		regs->bx = tcmi_regs->ebx;
		regs->cx = tcmi_regs->ecx;
		regs->dx = tcmi_regs->edx;

		regs->si = tcmi_regs->esi;
		regs->di = tcmi_regs->edi;

/*		regs->ss = tcmi_regs->xss;
		regs->cs = tcmi_regs->xcs; */

		regs->cs = __USER32_CS;		
		regs->ss = __USER32_DS;
		regs->ip = tcmi_regs->eip;
		regs->flags = tcmi_regs->eflags;
		regs->bp = tcmi_regs->ebp;
		regs->sp = tcmi_regs->esp;

		regs->r8 = 0;
		regs->r9 = 0;
		regs->r10 = 0;
		regs->r11 = 0;
		regs->r12 = 0;
		regs->r13 = 0;
		regs->r14 = 0;
		regs->r15 = 0;

		return;
	}


	mdbg(ERR3, "Unsupported source platform: %d", from_arch);
};

int is_in_syscall(struct pt_regs* regs) {
	return (( (long)regs->orig_ax >= 0) &&			
			   (regs->ax == -ERESTARTNOHAND ||		
			    regs->ax == -ERESTARTSYS ||		
			    regs->ax == -ERESTART_RESTARTBLOCK ||	
			    regs->ax == -ERESTARTNOINTR));
};

void debug_registers(struct pt_regs* regs) {
	mdbg(INFO4, "RSP  : %16lx    RBP : %16lx",regs->sp,regs->bp);
	mdbg(INFO4, "RSI  : %16lx    RDI : %16lx",regs->si,regs->di);
	mdbg(INFO4, "CS   : %16lx     SS  : %16lx", regs->cs,regs->ss);		
	mdbg(INFO4, "RIP  : %16lx    ORAX: %16lX",regs->ip,regs->orig_ax);
	mdbg(INFO4, "RAX  : %16lx    RBX : %16lX",regs->ax,regs->bx);
	mdbg(INFO4, "RCX  : %16lx    RDX : %16lX",regs->cx,regs->dx);
	mdbg(INFO4, "R08  : %16lx    R09 : %16lX",regs->r8,regs->r9);
	mdbg(INFO4, "R10  : %16lx    R11 : %16lX",regs->r10,regs->r11);
	mdbg(INFO4, "R12  : %16lx    R13 : %16lX",regs->r12,regs->r12);
	mdbg(INFO4, "R14  : %16lx    R15 : %16lX",regs->r14,regs->r15);
};

#endif
