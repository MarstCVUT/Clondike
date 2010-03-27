/**
 * @file tcmi_guest_wait_rpc.h - a RPC class used by guest process for processing wait syscall
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

#ifndef _TCMI_GUEST_RPC_WAIT_H
#define _TCMI_GUEST_RPC_WAIT_H

#include "tcmi_guest_rpc.h"
/** @defgroup tcmi_guest_rpc_wait_class Wait syscall
 *
 * @ingroup tcmi_guest_wait_class
 *
 * A \<\<singleton\>\> class that is executing RPC on PEN side.
 * 
 * @{
 */

TCMI_GUEST_RPC_CALL_DEF(wait4);

/**
 * @}
 */

#endif /* _TCMI_GUEST_RPC_FORK_H */

