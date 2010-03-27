/**
 * @file tcmi_shadow_wait_rpc.h - a RPC class used by shadow process for wait syscall
 */

#ifndef _TCMI_SHADOW_RPC_WAIT_H
#define _TCMI_SHADOW_RPC_WAIT_H

#include "tcmi_shadow_rpc.h"

/** @defgroup tcmi_shadow_rpc_wait_class Processing sys_wait4
 *
 * @ingroup tcmi_shadow_rpc_class
 *
 * A \<\<singleton\>\> class that is executing RPC on CCN side.
 * 
 * @{
 */

/** Declaration of tcmi_shadow_rpc_sys_wait4() */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(sys_wait4);

/**
 * @}
 */

#endif /* _TCMI_SHADOW_RPC_WAIT_H */

