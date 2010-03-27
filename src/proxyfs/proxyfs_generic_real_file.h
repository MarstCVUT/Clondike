/**
 * @file proxyfs_generic_real_file.h - File class for accessing generic files
 * (used by proxyfs_server_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_generic_real_file.h,v 1.2 2007/09/03 09:27:08 malatp1 Exp $
 *
 * This file is part of Proxy filesystem (ProxyFS)
 * Copyleft (C) 2007 Petr Malat
 * 
 * ProxyFS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * ProxyFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/********************** PUBLIC METHODS AND DATA *********************************/
#ifndef _PROXYFS_GENERIC_REAL_FILE_H_PUBLIC
#define _PROXYFS_GENERIC_REAL_FILE_H_PUBLIC

#include "proxyfs_real_file.h"

/**
 * @defgroup proxyfs_generic_real_file_class proxyfs_generic_real_file class
 * @ingroup proxyfs_real_file_class
 *
 * This class is used by server task for accessing generic files
 *
 * @{
 */

/** Generic_real_file declaration */
struct proxyfs_generic_real_file;

/** \<\<public\>\> Initialize generic_real_file */
int proxyfs_generic_real_file_init(struct proxyfs_generic_real_file *self);

/** \<\<public\>\> Deinitialize generic_real_file */
void proxyfs_generic_real_file_destroy(struct proxyfs_generic_real_file *self);

/** \<\<public\>\> generic_real_file construktor */
struct proxyfs_generic_real_file *proxyfs_generic_real_file_new(void);

/** \<\<public\>\> Casts to struct proxyfs_real_file_t * */
#define PROXYFS_GENERIC_REAL_FILE(arg) ((struct proxyfs_generic_real_file*)(arg))

#endif // _PROXYFS_GENERIC_REAL_FILE_H_PUBLIC

/********************** PROTECTED METHODS AND DATA ******************************/
#ifdef  PROXYFS_GENERIC_REAL_FILE_PROTECTED
#ifndef _PROXYFS_GENERIC_REAL_FILE_H_PROTECTED
#define _PROXYFS_GENERIC_REAL_FILE_H_PROTECTED

#define PROXYFS_REAL_FILE_PROTECTED // Parent
#include "proxyfs_real_file.h"

/** \<\<protected\>\> Generic_real_file definition */
struct proxyfs_generic_real_file {
	/** Parent class */
	struct proxyfs_real_file parent;
};

#endif // _PROXYFS_GENERIC_REAL_FILE_H_PROTECTED
#endif //  PROXYFS_GENERIC_REAL_FILE_PROTECTED

/********************** PRIVATE METHODS AND DATA *********************************/
#ifdef  PROXYFS_GENERIC_REAL_FILE_PRIVATE
#ifndef _PROXYFS_GENERIC_REAL_FILE_H_PRIVATE
#define _PROXYFS_GENERIC_REAL_FILE_H_PRIVATE


/** \<\<private\>\> Write to file from read buffer */
static int proxyfs_generic_real_file_write(struct proxyfs_real_file *self);

/** \<\<private\>\> Reads from file and write to writte buffer */
static int proxyfs_generic_real_file_read(struct proxyfs_real_file *self);

/** \<\<private\>\> "Open" file and send response message */
static int proxyfs_generic_real_file_open(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer);

/** \<\<private\>\> Structure with operations supporting polymorphism */
static struct proxyfs_real_file_ops generic_real_file_ops = {
	/** Reads from file and write to writte buffer */
	.read = &proxyfs_generic_real_file_read,
	/** Write to file from read buffer */
	.write = &proxyfs_generic_real_file_write,
	/** "Open" file and send response message */
	.open = &proxyfs_generic_real_file_open,
};

#endif // _PROXYFS_GENERIC_REAL_FILE_H_PRIVATE
#endif //  PROXYFS_GENERIC_REAL_FILE_PRIVATE

/**
 * @}
 */
