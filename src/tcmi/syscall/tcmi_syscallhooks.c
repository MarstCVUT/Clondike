/**
 * @file tcmi_syscallhooks.c - a separate module that install syscalls hooks into the kernel.
 *                      
 * 
 *
 *
 * Date: 17/04/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_syscallhooks.c,v 1.5 2007/10/11 21:00:26 stavam2 Exp $
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

#include <dbg.h>
#include "tcmi_syscallhooks_signal.h"
#include "tcmi_syscallhooks_pidman.h"
#include "tcmi_syscallhooks_userident.h"
#include "tcmi_syscallhooks_groupident.h"
#include "tcmi_syscallhooks_other.h"

#define TCMI_SYSCALLHOOKS_PRIVATE
#include "tcmi_syscallhooks.h"

/** @addtogroup tcmi_syscallhooks_class
 *
 * @{
 */

/** 
 * \<\<public\>\> Registers all syscalls hooks with the kernel.
 *
 * @return 0 upon success
 */
int tcmi_syscall_hooks_init(void)
{
	minfo(INFO1, "Registering TCMI syscalls hooks");
	tcmi_syscall_hooks_signal_init();
	tcmi_syscall_hooks_pidman_init();
	tcmi_syscall_hooks_userident_init();
	tcmi_syscall_hooks_groupident_init();
	tcmi_syscall_hooks_other_init();
	return 0;
}

/** 
 * \<\<public\>\> Unregisters all syscalls hooks.
 */
void tcmi_syscall_hooks_exit(void)
{
	minfo(INFO1, "Unregistering TCMI syscalls hooks");
	tcmi_syscall_hooks_signal_exit();
	tcmi_syscall_hooks_pidman_exit();
	tcmi_syscall_hooks_userident_exit();
	tcmi_syscall_hooks_groupident_exit();
	tcmi_syscall_hooks_other_exit();
}



/**
 * @}
 */
