/**
 * @file tcmi_syscallhooks.h - a separate module that install syscalls hooks into the kernel.
 *                      
 * 
 *
 *
 * Date: 17/04/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_syscallhooks.h,v 1.2 2007/09/02 12:09:55 malatp1 Exp $
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


#ifndef _TCMI_SYSCALLHOOKS_H
#define _TCMI_SYSCALLHOOKS_H

/** @defgroup tcmi_syscallhooks_class tcmi_syscallhooks class 
 *
 * @ingroup tcmi_syscall_group
 *
 * A \<\<singleton\>\> class that installs syscalls hooks into the kernel.
 * 
 * @{
 */

/** \<\<public\>\> Registers all hooks with the kernel. */
int tcmi_syscall_hooks_init(void);

/** \<\<public\>\> Unregisters all hooks. */
void tcmi_syscall_hooks_exit(void);


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SYSCALLHOOKS_PRIVATE

#endif /* TCMI_SYSCALLHOOKS_PRIVATE */

/**
 * @}
 */


#endif /* _TCMI_SYSCALLHOOKS_H */

