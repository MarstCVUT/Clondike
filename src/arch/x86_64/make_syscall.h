#ifndef _MAKE_SYSCALL_H
#define _MAKE_SYSCALL_H

#include <linux/syscalls.h>

#define __syscall "syscall"

#define __syscall_clobber "r11","rcx","memory" 

#define __syscall_return(type, res) \
do { \
        if ((unsigned long)(res) >= (unsigned long)(-127)) { \
                errno = -(res); \
                res = -1; \
        } \
        return (type) (res); \
} while (0)

#define _syscall0(type,name) \
type name(void) \
{ \
long __res; \
mm_segment_t old_fs;\
old_fs = get_fs();\
set_fs(KERNEL_DS);\
__res = sys_##name(); \
set_fs(old_fs);	\
__syscall_return(type,__res); \
}

#define _syscall1(type,name,type1,arg1) \
type name(type1 arg1) \
{ \
long __res; \
mm_segment_t old_fs;\
old_fs = get_fs();\
set_fs(KERNEL_DS);\
__res = sys_##name((type1)arg1); \
set_fs(old_fs);		\
__syscall_return(type,__res); \
}

#define _syscall2(type,name,type1,arg1,type2,arg2) \
type name(type1 arg1,type2 arg2) \
{ \
long __res; \
mm_segment_t old_fs;\
old_fs = get_fs();\
set_fs(KERNEL_DS);\
__res = sys_##name((type1)arg1, (type2)arg2); \
set_fs(old_fs);		\
__syscall_return(type,__res); \
}

#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3) \
type name(type1 arg1,type2 arg2,type3 arg3) \
{ \
long __res; \
mm_segment_t old_fs;\
old_fs = get_fs();\
set_fs(KERNEL_DS);\
__res = sys_##name((type1)arg1, (type2)arg2, (type3)arg3); \
set_fs(old_fs);		\
__syscall_return(type,__res); \
}

#endif
