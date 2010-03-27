#ifndef REGS_COMMON_H
#define REGS_COMMON_H

#include <arch/arch_ids.h>
#include <dbg.h>

/**
 * Loads value of pt_regs into the current architecture type specific regs. 
 * It can be done directly by copying as both structures are same.
 *
 * @param regs Current registers
 * @param tcmi_regs Target registers 
 */
static inline void get_registers(struct pt_regs* regs, struct tcmi_regs* tcmi_regs) {
	mdbg(INFO4, "Regs size: %lu TCMI regs size: %lu:", (unsigned long)sizeof(struct pt_regs), (unsigned long)sizeof(struct tcmi_regs));
	BUG_ON(sizeof(struct pt_regs) != sizeof(struct tcmi_regs));

	memcpy(tcmi_regs, regs, sizeof(struct tcmi_regs));
};

/**
 * Loads value of tcmi_regs into the regs. Used for restoring register context.
 * It can be done directly by copying as both structures are same.
 *
 * @param tcmi_regs Source registers
 * @param regs Target registers
 */
static inline void load_registers( struct tcmi_regs* tcmi_regs, struct pt_regs* regs) {
	BUG_ON(sizeof(struct pt_regs) != sizeof(struct tcmi_regs));

	memcpy(regs, tcmi_regs, sizeof(struct tcmi_regs));
};

/**
 * Generic method that converts current registers to target platform registers and returns pointer to those registers.
 * The whole flow of checkpoint/restart is as follow. 
 *  - First application that is being checkpointed determines if it is able to migrate to the target_arch
 *  - If so, it creates target arch registers
 *  - Application that is being restarted simply loads registers of its own native type.
 * 
 * @param regs Current registers
 * @param target_arch target architecture for which we want to have the registers
 * @return Pointer to target architecture specific registers. Caller is responsible for freeing these registers!
 */
extern void* get_target_platform_registers(struct pt_regs* regs, enum arch_ids target_arch);

/**
 * Loads registers from checkpointed registers
 *
 * @param platform_registers tcmi_regs structure of the "from_arch"
 * @param regs Registers to be filled
 * @param from_arch Architecture used for creating provided tcmi_regs
 * @param is_32bit_app 1 if the application being restored is 32 bit
 */
extern void load_from_platform_registers(void* platform_registers, struct pt_regs* regs, enum arch_ids from_arch, int is_32bit_app);

/**
 * Dumps debug information about current platform registers
 *
 * @param regs Current platform registers to be dumped
 */
extern void debug_registers(struct pt_regs* regs);


/** Checks whether a process with specified registers has been in
 * system call. A process in system call has:
 * - orig_eax >=0 which denotes the original system call number
 * - eax has one of the error values that describes why the
 * process has terminated the system call prematurely.
 */
extern int is_in_syscall(struct pt_regs* regs);

#endif
