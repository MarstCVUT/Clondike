#ifndef _MAKE_SYSCALL_H
#define _MAKE_SYSCALL_H

#define __syscall_return(type, res) \
do { \
	if ((unsigned long)(res) >= (unsigned long)(-(128 + 1))) { \
		errno = -(res); \
		res = -1; \
	} \
	return (type) (res); \
} while (0)

/* XXX - _foo needs to be __foo, while __NR_bar could be _NR_bar. */
#define _syscall0(type,name) \
type name(void) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name)); \
__syscall_return(type,__res); \
}

#define _syscall1(type,name,type1,arg1) \
type name(type1 arg1) \
{ \
long __res; \
__asm__ volatile ("push %%ebx ; movl %2,%%ebx ; int $0x80 ; pop %%ebx" \
	: "=a" (__res) \
	: "0" (__NR_##name),"ri" ((long)(arg1)) : "memory"); \
__syscall_return(type,__res); \
}

#define _syscall2(type,name,type1,arg1,type2,arg2) \
type name(type1 arg1,type2 arg2) \
{ \
long __res; \
__asm__ volatile ("push %%ebx ; movl %2,%%ebx ; int $0x80 ; pop %%ebx" \
	: "=a" (__res) \
	: "0" (__NR_##name),"ri" ((long)(arg1)),"c" ((long)(arg2)) \
	: "memory"); \
__syscall_return(type,__res); \
}

#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3) \
type name(type1 arg1,type2 arg2,type3 arg3) \
{ \
long __res; \
__asm__ volatile ("push %%ebx ; movl %2,%%ebx ; int $0x80 ; pop %%ebx" \
	: "=a" (__res) \
	: "0" (__NR_##name),"ri" ((long)(arg1)),"c" ((long)(arg2)), \
		  "d" ((long)(arg3)) : "memory"); \
__syscall_return(type,__res); \
}

#include <dbg.h>
#include <linux/unistd.h>
/** TODO: Export the original kernel_execve in kernel and use that instead */
static inline int copyof_kernel_execve(const char *filename, char *const argv[], char *const envp[])
{
	long __res;
	mdbg(INFO2, "Kexec before call");
	asm volatile ("push %%ebx ; movl %2,%%ebx ; int $0x80 ; pop %%ebx"
	: "=a" (__res)
	: "0" (__NR_execve),"ri" (filename),"c" (argv), "d" (envp) : "memory");

	mdbg(INFO2, "Kexec done with res: %ld", __res);
	return __res;
}

#endif
