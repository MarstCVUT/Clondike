#ifndef _EXPORTED_SYMBOLS_H
#define _EXPORTED_SYMBOLS_H

int do_tkill(int tgid, int pid, int sig);
asmlinkage long sys_wait4(pid_t pid, int __user *stat_addr, int options, struct rusage __user *ru);

#endif // _EXPORTED_SYMBOLS_H
