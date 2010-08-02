#ifndef ARG_I386_REGS_H
#define ARG_I386_REGS_H

#include <arch/arch_ids.h>
#include <linux/types.h>
#include <linux/ptrace.h>

#if defined(__i386__)
/** Simple alias for current register struct */ 
#define tcmi_regs tcmi_i386_regs
#endif

/**
 * Copy of corresponding pt_regs struct in ptrace.h. Must be kept in sync with kernel pt_regs!
 *
 * The reason why we make a copy here is, that we want to use multiple pt_regs concurrently (of multiple platforms), but
 * this is not possible with standard pt_regs, so we create our name-distinguished copy. 
* We also have to modify the types of the registers, to use platform neutral types.
 */
struct tcmi_i386_regs {
	u_int32_t ebx;
	u_int32_t ecx;
	u_int32_t edx;
	u_int32_t esi;
	u_int32_t edi;
	u_int32_t ebp;
	u_int32_t eax;
	u_int32_t  xds;
	u_int32_t  xes;
	u_int32_t  xfs;
	int32_t  xgs; 
	int32_t orig_eax; /* This is the only signed register as this register is never again seen by the user space and in kernel space we want to keep signess */
	u_int32_t eip;
	u_int32_t  xcs;
	u_int32_t eflags;
	u_int32_t esp;
	u_int32_t  xss;
}  __attribute__((__packed__));

#include <arch/common/regs.h>
#include <arch/x86_64/regs.h>

#if defined(__i386__)
#define stack_pointer(regs) ((regs)->sp)
#define base_pointer(regs) ((regs)->bp)
#define original_ax(regs) ((regs)->orig_ax)

extern void* get_target_platform_registers(struct pt_regs* regs, enum arch_ids target_arch);
extern void load_from_platform_registers(void* platform_registers, struct pt_regs* regs, enum arch_ids from_arch, int is_32bit_app);
extern int is_in_syscall(struct pt_regs* regs);
extern void debug_registers(struct pt_regs* regs);

#define platform_start_thread(regs, new_rip, new_rsp, program_arch, is_32_bit) do { \
	start_thread(regs, new_rip, new_rsp); \
} while(0)

#define check_is_application_32bit() \
	({ 							\
		1;						\
	})

#endif

#endif
