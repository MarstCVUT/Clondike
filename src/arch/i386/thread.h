#ifndef ARG_I386_THREAD_H
#define ARG_I386_THREAD_H

#include <arch/arch_ids.h>
#include <asm/processor.h>

#if defined(__i386__)
/** Simple alias for current thread struct */ 
#define tcmi_thread tcmi_i386_thread
#endif

/**
 * This structure contains all information of original thread_struct that we want to migrate.
 * Only platform independed data types are used, so that we are able to read/write on both 32 and 64 bit platforms
 */
struct tcmi_i386_thread {
	struct desc_struct tls_array[GDT_ENTRY_TLS_ENTRIES];
	u_int32_t gs;
}  __attribute__((__packed__));

#if defined(__i386__)
#include <arch/x86_64/thread.h>
static inline void get_tcmi_thread(struct thread_struct* thread, struct tcmi_thread* tcmi_thread) {
	memcpy(tcmi_thread->tls_array, thread->tls_array, sizeof(tcmi_thread->tls_array));
	tcmi_thread->gs = thread->gs;
}

static inline void load_thread(struct tcmi_i386_thread* tcmi_thread, struct thread_struct* thread) {
	memcpy(thread->tls_array, tcmi_thread->tls_array, sizeof(tcmi_thread->tls_array));	
	thread->gs = tcmi_thread->gs;
}

static inline void load_x86_64_thread(struct tcmi_x86_64_thread* tcmi_thread, struct thread_struct* thread) {
	/* FS and GS TLS are for whatever reason swaped on i386 and x86_64 */
	memcpy(&thread->tls_array[0], &tcmi_thread->tls_array[1], sizeof(thread->tls_array[0]));
	memcpy(&thread->tls_array[1], &tcmi_thread->tls_array[0], sizeof(thread->tls_array[1]));
	memcpy(&thread->tls_array[2], &tcmi_thread->tls_array[2], sizeof(thread->tls_array[2]));
	
	thread->gs = 0x33; /* TODO: Hardcoded gs segment.. we can at least use constants in segment.h */ 
}

static inline void load_from_platform_thread(void* platform_thread, struct thread_struct* thread, enum arch_ids from_arch) {
	if ( from_arch == ARCH_CURRENT ) { /* Equal architecture */
		load_thread( (struct tcmi_i386_thread*) platform_thread, thread);
		return;
	}

	if ( from_arch == ARCH_X86_64 ) {
		load_x86_64_thread( (struct tcmi_x86_64_thread*) platform_thread, thread);
		return;
	}

	mdbg(ERR3, "Unsupported source platform: %d", from_arch);
}

/** TODO: Check if we really were in TLS and if not do .. /? */
#define resolve_TLS(thread, cpu, regs) do { \
	load_TLS(thread, cpu); \
	loadsegment(gs, (thread)->gs); \
} while (0)

#endif

#endif
