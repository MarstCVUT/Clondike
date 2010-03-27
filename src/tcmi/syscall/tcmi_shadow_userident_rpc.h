/**
 * @file tcmi_shadow_rpc_userident.h - a RPC class used by shadow process .
 *
 * Date: 7/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_shadow_userident_rpc.h,v 1.1 2007/09/02 12:09:55 malatp1 Exp $
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

#ifndef _TCMI_SHADOW_RPC_USERIDENT_H
#define _TCMI_SHADOW_RPC_USERIDENT_H

#include "tcmi_shadow_rpc.h"

/** @defgroup tcmi_shadow_rpc_userident_class User identification 
 *  @ingroup tcmi_shadow_rpc_class
 *
 * A part of tcmi_shadow_rpc with RPCs for user identification
 * 
 * @{
 */

/** Setuid() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(setuid);

/** Setreuid() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(setreuid);

/** Setresuid() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(setresuid);

/** Geteuid() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(geteuid);

/** Getuid() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(getuid);

/** Getresuid() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(getresuid);

/**
 * @}
 */

#endif /* _TCMI_SHADOW_RPC_USERIDENT_H */

