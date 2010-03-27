#ifndef ARG_X86_64_REGS_H
#define ARG_X86_64_REGS_H

#include <arch/arch_ids.h>
#include <linux/types.h>
#include <linux/ptrace.h>

#if defined(__x86_64__)
/** Simple alias for current register struct */ 
#define tcmi_regs tcmi_x86_64_regs
#endif

/**
 * Copy of corresponding pt_regs struct in ptrace.h. Must be kept in sync with kernel pt_regs!
 *
 * The reason why we make a copy here is, that we want to use multiple pt_regs concurrently (of multiple platforms), but
 * this is not possible with standard pt_regs, so we create our name-distinguished copy. 
 * We also have to modify the types of the registers, to use platform neutral types.
 */
struct tcmi_x86_64_regs {
	u_int64_t r15;
	u_int64_t r14;
	u_int64_t r13;
	u_int64_t r12;
	u_int64_t rbp;
	u_int64_t rbx;
/* arguments: non interrupts/non tracing syscalls only save upto here*/
 	u_int64_t r11;
	u_int64_t r10;
	u_int64_t r9;
	u_int64_t r8;
	u_int64_t rax;
	u_int64_t rcx;
	u_int64_t rdx;
	u_int64_t rsi;
	u_int64_t rdi;
	u_int64_t orig_rax;
/* end of arguments */
/* cpu exception frame or undefined */
	u_int64_t rip;
	u_int64_t cs;
	u_int64_t eflags;
	u_int64_t rsp;
	u_int64_t ss;
/* top of stack page */
}  __attribute__((__packed__));

#include <arch/common/regs.h>

#if defined(__x86_64__)
#define stack_pointer(regs) ((regs)->sp)
#define base_pointer(regs) ((regs)->bp)
#define original_ax(regs) ((regs)->orig_ax)

extern void* get_target_platform_registers(struct pt_regs* regs, enum arch_ids target_arch);
extern void load_from_platform_registers(void* platform_registers, struct pt_regs* regs, enum arch_ids from_arch, int is_32bit_app);
extern int is_in_syscall(struct pt_regs* regs);
extern void debug_registers(struct pt_regs* regs);

#define start_thread_32(regs,new_rip,new_rsp) do { \
	asm volatile("movl %0,%%fs" :: "r" (0)); \
	asm volatile("movl %0,%%es; movl %0,%%ds": :"r" (__USER32_DS)); \
	load_gs_index(0); \
	(regs)->ip = (new_rip); \
	(regs)->sp = (new_rsp); \
	(regs)->flags = 0x200; \
	(regs)->cs = __USER32_CS; \
	(regs)->ss = __USER32_DS; \
	set_fs(USER_DS); \
} while(0) 

#define platform_start_thread(regs, new_rip, new_rsp, program_arch, is_32_bit) do { \
	if ( program_arch == ARCH_CURRENT && !is_32_bit) { \
		start_thread(regs, new_rip, new_rsp); \
	} else \
	if ( program_arch == ARCH_I386 || (program_arch == ARCH_CURRENT && is_32_bit) ) { \
		start_thread_32(regs, new_rip, new_rsp); \
	} else { \
		/* Should not get here */ \
		BUG_ON(1); \
	}\
} while(0)

#define check_is_application_32bit() \
	({ 							\
        	test_thread_flag(TIF_IA32); \
	})

#endif

#endif
