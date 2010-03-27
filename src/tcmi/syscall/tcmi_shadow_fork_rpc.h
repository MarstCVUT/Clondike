/**
 * @file tcmi_shadow_fork_rpc.h - a RPC class used by shadow process for fork syscall
 */

#ifndef _TCMI_SHADOW_RPC_FORK_H
#define _TCMI_SHADOW_RPC_FORK_H

#include "tcmi_shadow_rpc.h"

/** @defgroup tcmi_shadow_rpc_fork_class Processing all fork based syscalls
 *
 * @ingroup tcmi_shadow_rpc_class
 *
 * A \<\<singleton\>\> class that is executing RPC on CCN side.
 * 
 * @{
 */

/** Declaration of tcmi_shadow_rpc_do_fork() */
TCMI_SHADOW_RPC_GENERIC_CALL_DEF(do_fork);

/**
 * @}
 */

#endif /* _TCMI_SHADOW_RPC_WAIT_H */

