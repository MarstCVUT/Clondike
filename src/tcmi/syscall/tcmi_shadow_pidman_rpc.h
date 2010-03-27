/**
 * @file tcmi_shadow_rpc_pidman.h - a RPC class used by shadow process .
 *
 * Date: 7/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_shadow_pidman_rpc.h,v 1.2 2007/10/20 14:24:20 stavam2 Exp $
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

#ifndef _TCMI_SHADOW_RPC_PIDMAN_H
#define _TCMI_SHADOW_RPC_PIDMAN_H

#include "tcmi_shadow_rpc.h"

/** @defgroup tcmi_shadow_rpc_pidman_class PID & GID manipulation 
 *  @ingroup tcmi_shadow_rpc_class
 *
 * A part of tcmi_shadow_rpc with RPCs for PID and GID manipulation 
 * 
 * @{
 */

/** Declaration of tcmi_shadow_rpc_sys_getppid() */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(sys_getppid);

/** Declaration of tcmi_shadow_rpc_sys_setsid() */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(sys_setsid);

/** Declaration of tcmi_shadow_rpc_sys_getsid() */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(sys_getsid);

/** Declaration of tcmi_shadow_rpc_sys_setpgid() */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(sys_setpgid);

/** Declaration of tcmi_shadow_rpc_sys_getpgid() */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(sys_getpgid);

/** Declaration of tcmi_shadow_rpc_sys_getpgrp() */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(sys_getpgrp);

/**
 * @}
 */

#endif /* _TCMI_SHADOW_RPC_PIDMAN_H */

