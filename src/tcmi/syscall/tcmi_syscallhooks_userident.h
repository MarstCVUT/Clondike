/**
 * @file tcmi_syscallhooks_userident.h - 
 *                      
 * 
 *
 *
 * Date: 25/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_syscallhooks_userident.h,v 1.3 2007/09/02 13:54:30 stavam2 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2007  Petr Malat
 * 
 * TCMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * TCMI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef _TCMI_SYSCALLHOOKS_USERIDENT_H
#define _TCMI_SYSCALLHOOKS_USERIDENT_H

/** @defgroup tcmi_syscallhooks_userident_class User identification 
 *
 * @ingroup tcmi_syscallhooks_class
 *
 * A \<\<singleton\>\> class that installs user identity manipulation syscalls 
 * hooks into the kernel.
 * 
 * @{
 */

/** \<\<public\>\> Registers all PID manipolation hooks with the kernel. */
int tcmi_syscall_hooks_userident_init(void);

/** \<\<public\>\> Unregisters all PID manipolation hooks. */
void tcmi_syscall_hooks_userident_exit(void);


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SYSCALLHOOKS_USERIDENT_PRIVATE

/** \<\<private\>\> Getuid system call hook */
static long tcmi_syscall_hooks_sys_getuid(void);

/** \<\<private\>\> Setuid system call hook */
static long tcmi_syscall_hooks_sys_setuid(uid_t);

/** \<\<private\>\> Setreuid system call hook */
static long tcmi_syscall_hooks_sys_setreuid(uid_t, uid_t);

/** \<\<private\>\> Geteuid system call hook */
static long tcmi_syscall_hooks_sys_geteuid(void);

/** \<\<private\>\> Setresuid system call hook */
static long tcmi_syscall_hooks_sys_setresuid(uid_t ruid, uid_t euid, uid_t suid);

/** \<\<private\>\> Getresuid system call hook */
static long tcmi_syscall_hooks_sys_getresuid(uid_t* ruid, uid_t* euid, uid_t *suid);

#endif /* TCMI_SYSCALLHOOKS_USERIDENT_PRIVATE */

/**
 * @}
 */

#endif /* _TCMI_SYSCALLHOOKS_USERIDENT_H */

