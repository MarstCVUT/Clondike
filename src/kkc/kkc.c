/** @file kkc.c - Kernel to Kernel Communication Library for
 *                communication between two Linux Kernels
 *
 * Date: 04/13/2005
 *
 * Author: Jan Capek, Daniel Langr (original version for 2.4 kernel)
 *
 * $Id: kkc.c,v 1.3 2007/08/15 20:30:23 malatp1 Exp $
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

#define KKC_PRIVATE
#include "kkc.h"

#include <dbg.h>





/** 
 * \<\<public\>\> Creates a connection the destination specified by
 * the user.  The work is delegated to the kkc_establish() method
 * along with the socket method that kkc_establish() should call on
 * the newly created socket. To create new connection -
 * kkc_sock.c::kkc_sock_connect() is used.
 *
 * @param **sock - ouput parameter - upon successful connection
 * contains a valid socket
 * @param *where - address string with architecture prefix -
 * e.g. tcp:192.168.0.1.2:2000
 * @return 0 upon success
 */
int kkc_connect(struct kkc_sock **sock, const char *where)
{
	return kkc_establish(sock, where, kkc_sock_connect);
}


/** 
 * \<\<public\>\> Starts listening on a specified interface.  The work
 * is delegated to the kkc_establish() method along with the socket
 * method that kkc_establish() should call on the newly created
 * socket. To create new listening - kkc_sock.c::kkc_sock_connect() is
 * used.
 *
 * @param **sock - ouput parameter - upon successful creation of 
 * listening contains a valid socket
 * @param *where - address string with architecture prefix -
 * e.g. tcp:192.168.0.1.2:2000
 * @return 0 upon success
 */
int kkc_listen(struct kkc_sock **sock, const char *where)
{
	return kkc_establish(sock, where, kkc_sock_listen);
}

/** @addtogroup kkc_class
 *
 * @{
 */

/** 
 * \<\<private\>\> Creates an architecture specific socket and starts
 * listening or connects to the destination.  First an architecture,
 * based on the prefix in the address, has to be retrieved.  The
 * creation of the socket is delegated to the archictecture. If
 * everything goes fine, the socket is asked to perform an operation
 * specified by the user - kkc_sock_method().
 *
 * @param **sock - ouput parameter - upon successful creation of 
 * a connection or listening contains a valid socket
 * @param *where - address string with architecture prefix -
 * e.g. tcp:192.168.0.1.2:2000
 * @param *kkc_sock_method - method called on the created socket. 
 * This would yield a new connection or a listening, depending
 * on what method the user specified 
 * @return 0 upon success
 */
static int kkc_establish(struct kkc_sock **sock, const char *where,
			 int (*kkc_sock_method)(struct kkc_sock *, char *))
{

	struct kkc_arch *arch;
	int err = 0;
	char addr[KKC_SOCK_MAX_ADDR_LENGTH];
	*sock = NULL;

	mdbg(INFO4, "Requesting architecture structure and network address");
	if ((err = kkc_get_architecture(where, &arch, addr)) < 0) {
		mdbg(ERR4, "Cannot get architecture structure");
		err = -ENOMEM;
		goto exit0;
	}

	/* instantiate the socket */
	if (!((*sock) = KKC_SOCK(kkc_arch_obj_new(arch)))) {
		mdbg(ERR4, "Failed to create a new socket");
		err = -ENOMEM;
		goto exit1;
	}
	if (kkc_sock_method(*sock, addr)) {
		mdbg(ERR4, "KKC socket method failed");
		err = -ENOMEM;
		goto exit2;
	}
	/* release the architecture as the socket has created its own
	 * reference */
	kkc_arch_put(arch);
	return 0;
	/* error handling */
 exit2:
	kkc_sock_put(*sock);
	*sock = NULL; /* return no socket to the user */
 exit1:
	kkc_arch_put(arch);
 exit0:
	return err;
}

/**
 * \<\<private\>\> Helper function for parsing 'where' string and
 * requesting for particular architecture structure. As side effect
 * fills 'addr' string with address part of 'where'.
 *
 * @param *where - pointer to the where request string
 * @param **arch - output parameter - stores the architecture if one found
 * @param *addr - storage for the address string, that gets extracted from 
 * the where string
 * @return 0 upon success.
 */
static int kkc_get_architecture(const char *where, struct kkc_arch **arch, char *addr)
{
	char arch_name[KKC_MAX_ARCH_LENGTH];
	char *colon_ptr;
	int colon_pos;

	int err = 0;

	/* find colon */
	if ((colon_ptr = strchr(where, ':')) == NULL) {
		mdbg(ERR4, "Invalid syntax - could not find colon");
		err = -EINVAL;
		goto exit0;
	}

	/* check position */
	colon_pos = colon_ptr - where;
	if (colon_pos > (KKC_MAX_ARCH_LENGTH - 1)) {
		mdbg(ERR4, "Invalid syntax - too long architecture name length");
		err = -EINVAL;
		goto exit0;
	}

	/* get architecture name */
	strncpy(arch_name, where, colon_pos);
	arch_name[colon_pos] = '\0';
	mdbg(INFO4, "Extracted architecture name: ""%s""", arch_name);

	/* get address */
	strncpy(addr, colon_ptr + 1, KKC_SOCK_MAX_ADDR_LENGTH - 1);
	addr[KKC_SOCK_MAX_ADDR_LENGTH - 1] = '\0';
	mdbg(INFO4, "Extracted address: ""%s""", addr); 
	

	if (!(*arch = kkc_arch_get_by_name(arch_name))) {
		mdbg(ERR4, "Could not get architecture for '%s'", arch_name);
		err = -EINVAL;
	}
		
	/* error handling */
exit0:
	return err;
}

/**
 * @}
 */


/**
 * KKC module loading
 *
 * @return always 0
 */
int __init kkc_init_module(void)
{

	minfo(INFO1, "Loading KKC");
	return 0;
}

/**
 * KKC module unloading
 */
void __exit kkc_exit_module(void)
{
	minfo(INFO1, "Unloading KKC");
}

module_init(kkc_init_module);
module_exit(kkc_exit_module);


EXPORT_SYMBOL_GPL(kkc_connect);
EXPORT_SYMBOL_GPL(kkc_listen);

/*
 * Module properties
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");
MODULE_AUTHOR("Daniel Langr");
MODULE_DESCRIPTION("Kernel to Kernel Communication Library");
