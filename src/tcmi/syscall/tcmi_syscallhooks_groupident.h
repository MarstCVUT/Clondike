/**
 * @file tcmi_syscallhooks_groupident.h - 
 *                      
 * 
 *
 *
 * Date: 25/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_syscallhooks_groupident.h,v 1.1 2007/09/02 12:09:55 malatp1 Exp $
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


#ifndef _TCMI_SYSCALLHOOKS_GROUPIDENT_H
#define _TCMI_SYSCALLHOOKS_GROUPIDENT_H

/** @defgroup tcmi_syscallhooks_groupident_class Group identification 
 *
 * @ingroup tcmi_syscallhooks_class
 *
 * A part of tcmi_syscallhooks class that installs group identity manipulation syscalls 
 * hooks into the kernel.
 * 
 * @{
 */

/** \<\<public\>\> Registers all PID manipolation hooks with the kernel. */
int tcmi_syscall_hooks_groupident_init(void);

/** \<\<public\>\> Unregisters all PID manipolation hooks. */
void tcmi_syscall_hooks_groupident_exit(void);


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SYSCALLHOOKS_GROUPIDENT_PRIVATE

/** \<\<private\>\> Getgid system call hook */
static long tcmi_syscall_hooks_sys_getgid(void);

/** \<\<private\>\> Setgid system call hook */
static long tcmi_syscall_hooks_sys_setgid(gid_t);

/** \<\<private\>\> Setregid system call hook */
static long tcmi_syscall_hooks_sys_setregid(gid_t, gid_t);

/** \<\<private\>\> Setresgid system call hook */
static long tcmi_syscall_hooks_sys_setresgid(gid_t, gid_t, gid_t);

/** \<\<private\>\> Getegid system call hook */
static long tcmi_syscall_hooks_sys_getegid(void);

/** \<\<private\>\> Getgroups system call hook */
static long tcmi_syscall_hooks_sys_getgroups(int size, gid_t *groups);

/** \<\<private\>\> Setgroups system call hook */
static long tcmi_syscall_hooks_sys_setgroups(int size, const gid_t *groups);

/** \<\<private\>\> Getresgid system call hook */
static long tcmi_syscall_hooks_sys_getresgid(gid_t *, gid_t *, gid_t *);

#endif /* TCMI_SYSCALLHOOKS_GROUPIDENT_PRIVATE */

/**
 * @}
 */

#endif /* _TCMI_SYSCALLHOOKS_GROUPIDENT_H */

