/**
 * @file tcmi_guest_rpc.c - PEN part of RPCs
 *
 *
 * Date: 7/05/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_guest_rpc.c,v 1.1 2007/09/02 12:09:55 malatp1 Exp $
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
#define TCMI_RPC_PRIVATE
#define TCMI_GUEST_RPC_PRIVATE
#include "tcmi_guest_rpc.h"

struct tcmi_rpc tcmi_guest_rpc_struct = {
	.calltable = tcmi_calltable_guest,
};

struct tcmi_rpc *tcmi_guest_rpc = &tcmi_guest_rpc_struct;

/**
 * @}
 */

