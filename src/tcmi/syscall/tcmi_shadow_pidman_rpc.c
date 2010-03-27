/**
 * @file tcmi_shadow_rpc_pidman.c - CCN part of RPCs
 *
 * Date: 7/06/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_shadow_pidman_rpc.c,v 1.3 2007/10/20 14:24:20 stavam2 Exp $
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

#include "tcmi_shadow_pidman_rpc.h"

#include <linux/syscalls.h>

static int errno; // todo: remove this syscalls CANNOT be called from kernel on x86_64 anyway so we have to export them all...

TCMI_SHADOW_RPC_GENERIC_CALL(0, sys_getpgrp, false);
TCMI_SHADOW_RPC_GENERIC_CALL(0, sys_getppid, false);
TCMI_SHADOW_RPC_GENERIC_CALL(0, sys_setsid,  false);
TCMI_SHADOW_RPC_GENERIC_CALL(1, sys_getpgid, false);
TCMI_SHADOW_RPC_GENERIC_CALL(1, sys_getsid,  false);
TCMI_SHADOW_RPC_GENERIC_CALL(2, sys_setpgid, false);

