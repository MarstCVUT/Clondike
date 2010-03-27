/**
 * @file tcmi_shadow_rpc_groupident.h - a RPC class used by shadow process .
 *
 * Date: 7/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_shadow_groupident_rpc.h,v 1.1 2007/09/02 12:09:55 malatp1 Exp $
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

#ifndef _TCMI_SHADOW_RPC_GROUPIDENT_H
#define _TCMI_SHADOW_RPC_GROUPIDENT_H

#include "tcmi_shadow_rpc.h"

/** @defgroup tcmi_shadow_rpc_groupident_class Group identification 
 *  @ingroup tcmi_shadow_rpc_class
 *
 * A part of tcmi_shadow_rpc with RPCs for group identification
 * 
 * @{
 */


/** Getegid() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(getegid);

/** Getgid() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(getgid);

/** Getgroups() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(getgroups);

/** Getresgid() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(getresgid);

/** Setgid() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(setgid);

/** Setgroups() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(setgroups);

/** Setregid() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(setregid);

/** Setresgid() system call */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(setresgid);

/**
 * @}
 */

#endif /* _TCMI_SHADOW_RPC_GROUPIDENT_H */

