#ifndef ARCH_IDS_H
#define ARCH_IDS_H

/** Enumeration representing all supported architecture types */
enum arch_ids {
	UNKNOWN,
	ARCH_I386,
	ARCH_X86_64,
	ARCH_UM,
	
	ARCH_COUNT
};

extern char* arch_names[];

/** Definition of ARCH_CURRENT points to enum element representing architecture type of the current kernel */
#if defined(__i386__)
#define ARCH_CURRENT ARCH_I386
#elif defined(__x86_64__)
#define ARCH_CURRENT ARCH_X86_64
#elif defined(__arch_um__)
#define ARCH_CURRENT ARCH_UM
#endif

#endif
