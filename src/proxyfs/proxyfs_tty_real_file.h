/**
 * @file proxyfs_tty_real_file.h - File class sepcialized on tty access 
 * (used by proxyfs_server_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_tty_real_file.h,v 1.3 2007/10/20 14:24:20 stavam2 Exp $
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
#ifndef _PROXYFS_TTY_REAL_FILE_H_PUBLIC
#define _PROXYFS_TTY_REAL_FILE_H_PUBLIC

#include "proxyfs_real_file.h"

/**
 * @defgroup proxyfs_tty_real_file_class proxyfs_tty_real_file class
 * @ingroup proxyfs_real_file_class
 *
 * This class is used by server task for accessing to TTYs
 *
 * @{
 */

/** tty_real_file declaration */
struct proxyfs_tty_real_file;

/** \<\<public\>\> Initialize tty_real_file */
int proxyfs_tty_real_file_init(struct proxyfs_tty_real_file *self);

/** \<\<public\>\> Deinitialize tty_real_file */
void proxyfs_tty_real_file_destroy(struct proxyfs_tty_real_file *self);

/** \<\<public\>\> tty_real_file construktor */
struct proxyfs_tty_real_file *proxyfs_tty_real_file_new(void);

/** \<\<public\>\> Casts to struct proxyfs_real_file_t * */
#define PROXYFS_TTY_REAL_FILE(arg) ((struct proxyfs_tty_real_file*)(arg))

#endif // _PROXYFS_TTY_REAL_FILE_H_PUBLIC

/********************** PROTECTED METHODS AND DATA ******************************/
#ifdef  PROXYFS_TTY_REAL_FILE_PROTECTED
#ifndef _PROXYFS_TTY_REAL_FILE_H_PROTECTED
#define _PROXYFS_TTY_REAL_FILE_H_PROTECTED

#define PROXYFS_REAL_FILE_PROTECTED // Parent
#include "proxyfs_real_file.h"

/** tty_real_file definition */
struct proxyfs_tty_real_file {
	/** Parent class */
	struct proxyfs_real_file parent;
};

#endif // _PROXYFS_TTY_REAL_FILE_H_PROTECTED
#endif //  PROXYFS_TTY_REAL_FILE_PROTECTED

/********************** PRIVATE METHODS AND DATA *********************************/
#ifdef  PROXYFS_TTY_REAL_FILE_PRIVATE
#ifndef _PROXYFS_TTY_REAL_FILE_H_PRIVATE
#define _PROXYFS_TTY_REAL_FILE_H_PRIVATE

/** \<\<private\>\> Read from tty */
static ssize_t proxyfs_tty_real_file_tty_read(struct file * file, char *buf, size_t count, loff_t *ppos);

/** \<\<private\>\> Write to file from read buffer */
static int proxyfs_tty_real_file_write(struct proxyfs_real_file *self);

/** \<\<private\>\> Reads from file and write to writte buffer */
static int proxyfs_tty_real_file_read(struct proxyfs_real_file *self);

/** \<\<private\>\> "Open" file and send response message */
static int proxyfs_tty_real_file_open(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer);

/** \<\<public\>\> Duplicates a proxyfs real file */
static struct proxyfs_real_file* proxyfs_tty_real_file_duplicate(struct proxyfs_real_file *self);


/** <\<\private\>\> Structure with pointers to virtual methods */
static struct proxyfs_real_file_ops tty_real_file_ops = {
	/** Reads from file and write to writte buffer */
	.read = &proxyfs_tty_real_file_read,
	/** Write to file from read buffer */
	.write = &proxyfs_tty_real_file_write,
	/** "Open" file and send response message */
	.open = &proxyfs_tty_real_file_open,
	.duplicate = &proxyfs_tty_real_file_duplicate,
};

#endif // _PROXYFS_TTY_REAL_FILE_H_PRIVATE
#endif //  PROXYFS_TTY_REAL_FILE_PRIVATE

/**
 * @}
 */
