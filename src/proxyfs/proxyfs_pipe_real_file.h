/**
 * @file proxyfs_pipe_real_file.h - File class for accessing pipe files
 * (used by proxyfs_server_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_pipe_real_file.h,v 1.3 2007/11/05 19:38:28 stavam2 Exp $
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
#ifndef _PROXYFS_PIPE_REAL_FILE_H_PUBLIC
#define _PROXYFS_PIPE_REAL_FILE_H_PUBLIC

#include "proxyfs_real_file.h"

/**
 * @defgroup proxyfs_pipe_real_file_class proxyfs_pipe_real_file class
 * @ingroup proxyfs_real_file_class
 *
 * This class is used by server task for accessing pipe files
 *
 * @{
 */

/** pipe_real_file declaration */
struct proxyfs_pipe_real_file;

/** \<\<public\>\> Initialize pipe_real_file */
int proxyfs_pipe_real_file_init(struct proxyfs_pipe_real_file *self);

/** \<\<public\>\> Deinitialize pipe_real_file */
void proxyfs_pipe_real_file_destroy(struct proxyfs_pipe_real_file *self);

/** \<\<public\>\> pipe_real_file construktor */
struct proxyfs_pipe_real_file *proxyfs_pipe_real_file_new(void);

/** \<\<public\>\> Casts to struct proxyfs_real_file_t * */
#define PROXYFS_PIPE_REAL_FILE(arg) ((struct proxyfs_pipe_real_file*)(arg))

#endif // _PROXYFS_PIPE_REAL_FILE_H_PUBLIC

/********************** PROTECTED METHODS AND DATA ******************************/
#ifdef  PROXYFS_PIPE_REAL_FILE_PROTECTED
#ifndef _PROXYFS_PIPE_REAL_FILE_H_PROTECTED
#define _PROXYFS_PIPE_REAL_FILE_H_PROTECTED

#define PROXYFS_REAL_FILE_PROTECTED // Parent
#include "proxyfs_real_file.h"

/** \<\<protected\>\> pipe_real_file definition */
struct proxyfs_pipe_real_file {
	/** Parent class */
	struct proxyfs_real_file parent;
	/** Write counter */
	uint32_t write_counter;
	/** True if next write must be atomic */
	int atomic_write;
	/** Helper buffer for atomic write (used when data overlaps) */
	char atomic_buf[PIPE_BUF];
};

#endif // _PROXYFS_PIPE_REAL_FILE_H_PROTECTED
#endif //  PROXYFS_PIPE_REAL_FILE_PROTECTED

/********************** PRIVATE METHODS AND DATA *********************************/
#ifdef  PROXYFS_PIPE_REAL_FILE_PRIVATE
#ifndef _PROXYFS_PIPE_REAL_FILE_H_PRIVATE
#define _PROXYFS_PIPE_REAL_FILE_H_PRIVATE


/** \<\<private\>\> Write to file from read buffer */
static int proxyfs_pipe_real_file_write(struct proxyfs_real_file *self);

/** \<\<private\>\> Reads from file and write to writte buffer */
static int proxyfs_pipe_real_file_read(struct proxyfs_real_file *self);

/** \<\<private\>\> "Open" file and send response message */
static int proxyfs_pipe_real_file_open(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer);

/** \<\<private\>\> Duplicates file */
static struct proxyfs_real_file* proxyfs_pipe_real_file_duplicate(struct proxyfs_real_file *self);


/** <\<\private\>\> Structure with pointers to virtual methods */
static struct proxyfs_real_file_ops pipe_real_file_ops = {
	/** Reads from file and write to writte buffer */
	.read = &proxyfs_pipe_real_file_read,
	/** Write to file from read buffer */
	.write = &proxyfs_pipe_real_file_write,
	/** "Open" file and send response message */
	.open = &proxyfs_pipe_real_file_open,
	.duplicate = &proxyfs_pipe_real_file_duplicate,
};

#endif // _PROXYFS_PIPE_REAL_FILE_H_PRIVATE
#endif //  PROXYFS_PIPE_REAL_FILE_PRIVATE

/**
 * @}
 */
