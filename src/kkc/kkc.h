/** @file kkc.h - Kernel to Kernel Communication Library for
 *                communication between two Linux Kernels
 *
 * Date: 04/13/2005
 *
 * Author: Jan Capek, Daniel Langr (original version for 2.4 kernel)
 *
 * $Id: kkc.h,v 1.3 2007/08/15 20:30:23 malatp1 Exp $
 *
 * This file is part of Kernel-to-Kernel Communication Library(KKC)
 * Copyleft (C) 2005  Jan Capek
 * 
 * KKC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * KKC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _KKC_H
#define _KKC_H

#include <linux/module.h>
#include <linux/types.h>

#include "kkc_sock.h"
#include "kkc_sock_sleeper.h"
#include "kkc_arch.h"


/** @defgroup kkc_class KKC
 *
 * KKC is a \<\<singleton\>\> class that provides interkernel
 * communication facility. KKC sockets are used as an abstraction of
 * a connection. This component ensures loading architecture specific
 * modules via \link kkc_arch_class kkc_arch \endlink. 
 *
 * The communication features can be divided into two groups: 
 *
 * -# Socket creation and connecting to the destination or listening
 * on a specified interface. This is handled by connect/listen
 * operations as follows: Based on the string specified by the user
 * (e.g. tcp:192.168.0.1:2000), it gets the instance of the particular
 * architecture (tcp in this case) and delegates the work to the
 * particular architecture to instantiate the socket. Then, the new socket
 * is asked to setup a connection or listening.
 * -# Regular socket operations - to send/receive data, disconnect,
 * append/remove a process to the socket waitqueue. These operations
 * are KKC socket methods and this class provides only aliases.
 *
 * @{
 */


/** Maximum where string length*/
#define KKC_MAX_WHERE_LENGTH (KKC_MAX_ARCH_LENGTH + KKC_SOCK_MAX_ADDR_LENGTH)


/** \<\<public\>\> Creates an architecture specific socket and
 * performs a connection to the destination. */
extern int kkc_connect(struct kkc_sock **sock, const char *addr);
/** \<\<public\>\> Creates an architecture specific socket and starts
 * listening for incoming connnections. */
extern int kkc_listen(struct kkc_sock **sock, const char *addr);

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef KKC_PRIVATE

/** Creates a new socket and Establishes a new connection or
 * listening */
static int kkc_establish(struct kkc_sock **sock, const char *addr,
			 int (*kkc_sock_method)(struct kkc_sock *, char *));
/** Gets the architecture based on the where string. */
static int kkc_get_architecture(const char *where, struct kkc_arch **arch, char *addr);

#endif /* KKC_PRIVATE */

/**
 * @}
 */

#endif	/* _KKC_H */
