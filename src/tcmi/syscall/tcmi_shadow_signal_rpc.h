/**
 * @file tcmi_shadow_signal_rpc.h - a RPC class used by shadow process .
 *
 * Date: 7/06/2007
 *
 * Author: Petr Malat
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2007  Petr Malat
 *
 * $Id: tcmi_shadow_signal_rpc.h,v 1.1 2007/09/02 12:09:55 malatp1 Exp $
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

#ifndef _TCMI_SHADOW_RPC_SIGNAL_H
#define _TCMI_SHADOW_RPC_SIGNAL_H

#include "tcmi_shadow_rpc.h"

/** @defgroup tcmi_shadow_rpc_signal_class Signal sending 
 *
 * @ingroup tcmi_shadow_rpc_class
 *
 * A \<\<singleton\>\> class that is executing RPC on CCN side.
 * 
 * @{
 */

/** Declaration of tcmi_shadow_rpc_sys_kill() */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(sys_kill);

/** Declaration of tcmi_shadow_rpc_do_tkill() */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(do_tkill);

/** Declaration of tcmi_shadow_rpc_rt_sigqueueinfo() */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(rt_sigqueueinfo);

/**
 * @}
 */

#endif /* _TCMI_SHADOW_RPC_SIGNAL_H */

