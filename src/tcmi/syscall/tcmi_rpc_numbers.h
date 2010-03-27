#ifndef TCMI_RPC_NUMBERS_H
#define TCMI_RPC_NUMBERS_H

/* Signal ********************************************/
#define TCMI_RPC_SYS_KILL 		1
#define TCMI_RPC_SYS_SIGQUEUE		2
#define TCMI_RPC_SYS_TKILL 		3

/* Pid, gid and session manipulation *****************/
#define TCMI_RPC_SYS_GETPGID 		4
#define TCMI_RPC_SYS_GETPID 		5
#define TCMI_RPC_SYS_GETPPID 		6
#define TCMI_RPC_SYS_GETSID 		7
#define TCMI_RPC_SYS_SETPGID 		8
#define TCMI_RPC_SYS_SETSID 		9
#define TCMI_RPC_SYS_GETPGRP		10

/* User identification *******************************/
#define TCMI_RPC_SYS_GETEUID 		11
#define TCMI_RPC_SYS_GETRESUID		12
#define TCMI_RPC_SYS_GETUID 		13
#define TCMI_RPC_SYS_SETRESUID 		14
#define TCMI_RPC_SYS_SETREUID 		15
#define TCMI_RPC_SYS_SETUID 		16

/* Group identification ******************************/
#define TCMI_RPC_SYS_GETEGID 		17
#define TCMI_RPC_SYS_GETGID 		18
#define TCMI_RPC_SYS_GETGROUPS		19
#define TCMI_RPC_SYS_GETRESGID		20
#define TCMI_RPC_SYS_SETGID 		21	
#define TCMI_RPC_SYS_SETGROUPS		23
#define TCMI_RPC_SYS_SETREGID		24
#define TCMI_RPC_SYS_SETRESGID		25 


/* Wait ***************/
#define TCMI_RPC_SYS_WAIT4		26

/* Fork ***************/
#define TCMI_RPC_SYS_FORK		27

/** The highest number used in RPC identification */
#define TCMI_MAX_RPC_NUM 30

#endif
