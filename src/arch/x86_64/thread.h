#ifndef ARG_X86_64_THREAD_H
#define ARG_X86_64_THREAD_H

#if defined(__x86_64__)
/** Simple alias for current thread struct */ 
#define tcmi_thread tcmi_x86_64_thread
#endif

struct tcmi_x86_64_thread {
	u_int64_t tls_array[GDT_ENTRY_TLS_ENTRIES];
	u_int64_t gs, fs;
	u_int32_t gsindex, fsindex;
} __attribute__((__packed__));

#if defined(__x86_64__)
#include <arch/i386/thread.h>
static inline void get_tcmi_thread(struct thread_struct* thread, struct tcmi_thread* tcmi_thread) {
	memcpy(tcmi_thread->tls_array, thread->tls_array, sizeof(tcmi_thread->tls_array));
	tcmi_thread->gs = thread->gs;
	tcmi_thread->gsindex = thread->gsindex;
	tcmi_thread->fs = thread->fs;
	tcmi_thread->fsindex = thread->fsindex;

	mdbg(INFO4, "GS  : %16lx    GS index : %4x",thread->gs, thread->gsindex);
	mdbg(INFO4, "FS  : %16lx    FS index : %4x",thread->fs, thread->fsindex);
}

static inline void load_thread(struct tcmi_x86_64_thread* tcmi_thread, struct thread_struct* thread) {
	memcpy(thread->tls_array, tcmi_thread->tls_array, sizeof(tcmi_thread->tls_array));
	thread->gs = tcmi_thread->gs;
	thread->gsindex = tcmi_thread->gsindex;
	thread->fs = tcmi_thread->fs;
	thread->fsindex = tcmi_thread->fsindex;

	mdbg(INFO4, "GS  : %16lx    GS index : %4x",thread->gs, thread->gsindex);
	mdbg(INFO4, "FS  : %16lx    FS index : %4x",thread->fs, thread->fsindex);

	/*
	thread->tls_array[0] = tcmi_thread->tls_array[1];
	thread->tls_array[1] = tcmi_thread->tls_array[0];
	thread->tls_array[2] = tcmi_thread->tls_array[2];

	tcmi_thread->gs = thread->gs;
	thread->gsindex = tcmi_thread->gsindex;
	*/
}

static inline void load_i386_thread(struct tcmi_i386_thread* tcmi_thread, struct thread_struct* thread) {
	/* FS and GS TLS are for whatever reason swaped on i386 and x86_64 */
	thread->tls_array[0] = tcmi_thread->tls_array[1];
	thread->tls_array[1] = tcmi_thread->tls_array[0];
	thread->tls_array[2] = tcmi_thread->tls_array[2];

	// TODO: Gs is now assumed to be used for TLS and so we simply load it, but some more checks, whether it was really like that, would be nice */
	thread->gs = 0;
	thread->gsindex = GS_TLS_SEL;
	// TODO: Load fs
	thread->fs = 0;
	thread->fsindex = 0;
}

static inline void load_from_platform_thread(void* platform_thread, struct thread_struct* thread, enum arch_ids from_arch) {
	if ( from_arch == ARCH_CURRENT ) { /* Equal architecture */
		int i;
		load_thread( (struct tcmi_x86_64_thread*) platform_thread, thread);

		for ( i=0; i < 3; i++ ) {		
			unsigned long long sg32bit = 1;
			unsigned long long usablebit = 1;
 			sg32bit = sg32bit << 54;	
			usablebit = usablebit << 52;
			//mdbg(INFO3, "LDT[%d]: %llX is seg 32: %d Usable: %d", i, thread->tls_array[i], ((sg32bit & thread->tls_array[i]) > 0), ((usablebit & thread->tls_array[i])) > 0); 
		}
		return;
	}

	if ( from_arch == ARCH_I386 ) {
		int i;
		load_i386_thread( (struct tcmi_i386_thread*) platform_thread, thread);
		for ( i=0; i < 3; i++ ) {		
			unsigned long long sg32bit = 1;
			unsigned long long usablebit = 1;
 			sg32bit = sg32bit << 54;	
			usablebit = usablebit << 52;
			//mdbg(INFO3, "LDT[%d]: %llX is seg 32: %d Usable: %d", i, thread->tls_array[i], ((sg32bit & thread->tls_array[i]) > 0), ((usablebit & thread->tls_array[i])) > 0); 
		}
		return;
	}

	mdbg(ERR3, "Unsupported source platform: %d", from_arch);
}

//asm volatile("movl %0,%%fs" :: "g"((thread)->fsindex));

#define resolve_TLS(thread, cpu, regs) do { \
	load_TLS(thread, cpu); \
	load_gs_index((thread)->gsindex); \
	if ( (thread)->fs )				\
		wrmsrl(MSR_FS_BASE, (thread)->fs); \
	\
} while (0)

#endif


#endif
