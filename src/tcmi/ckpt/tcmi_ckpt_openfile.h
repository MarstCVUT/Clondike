/**
 * @file tcmi_ckpt_openfile.h - a helper class that provides functionality to
 *                              store/restore information about an open file in the checkpoint
 *                      
 * 
 *
 *
 * Date: 04/30/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ckpt_openfile.h,v 1.4 2007/09/03 01:17:58 malatp1 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2005  Jan Capek
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
#ifndef _TCMI_CKPT_OPENFILE_H
#define _TCMI_CKPT_OPENFILE_H

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <asm/uaccess.h>

#define __KERNEL_SYSCALLS__
#include <linux/syscalls.h> 
#include <linux/unistd.h> 

#include "tcmi_ckpt.h"

#include <dbg.h>

/** @defgroup tcmi_ckpt_openfile_class tcmi_ckpt_openfile class 
 *
 * @ingroup tcmi_ckpt_class
 *
 * This is a helper \<\<singleton\>\> class used by the \link
 * tcmi_ckpt_class checkpoint \endlink and provides functionality to:
 *
 * - write an information about an open file into the checkpoint 
 * - restore/reopen a file based on information in the checkpoint 
 *
 * The checkpoint instance during checkpoint image creation passes
 * each file along with its file descriptor and current file
 * descriptor cache to this component. The are two separate cases:
 * - the file information has already been written into the checkpoint -
 * this is indicated by a file object being already present in the 
 * \link tcmi_fdcache_class file descriptor cache \endlink.
 * In this case only a short information about a file is written
 * into the checkpoint file - it's current file descriptor and the
 * file descriptor that it's duplicating
 * - the file information hasn't been written into the checkpoint
 * (not present in the fd cache). In this case a full record
 * is required:
 *     - file descriptor
 *     - current position in file
 *     - file flags used when opening the file
 *     - file access mode
 *     - file type(regular/directory/char/block device/pipe/socket/symlink)
 *     - full pathname
 *
 *
 * Initial implementation supports checkpointing of regular files and
 * directories only. There is a limited support when checkpointing
 * open character devices - seek operation is omitted. Future
 * implementation when restoring the open files should consider the
 * circumstances and decided to reopen the original file or special
 * proxy file. The main purpose of the proxy file will be to forward
 * all requests to the original node.
 * 
 * @todo Implement handling of special files (character devices/block
 * devices, pipes, sockets)
 *
 *  @{
 */

/** describes various chunks of the checkpoint file */
typedef enum {
	/* new file */
	TCMI_CKPT_OPENFILE_NEWFILE = 0,
	/* duplicate file descriptor, refers to some regular file */
	TCMI_CKPT_OPENFILE_DUPFILE
} tcmi_ckpt_openfile_chunk_t;

/** Describes generic header that consists of a file type only - the
 * options are new or dup file*/
struct tcmi_ckpt_openfile_generic_hdr {
	/** type of the open file */
	tcmi_ckpt_openfile_chunk_t  restore_type;
} __attribute__((__packed__));


/** Describes a file occurs in the checkpoint for the first time. */
struct tcmi_ckpt_openfile_newfile_hdr {
	/** file descriptor */
	int32_t fd;
	/** current position in the file. */
	u_int64_t pos;
	/** flags specified by the user when opening the file */
	u_int32_t flags;
	/** access mode for the file */
	u_int32_t mode; /* mode_t mode; */
	/** file type */
	u_int32_t type;
	/** length of the pathname string (including trailing zero) */
	int32_t pathname_size;
} __attribute__((__packed__));


/** Describes a duplicate file. The duplicate file descriptor refers
 * to the original file object that has been open for the first
 * time. That is: 'dup_fd' has to be duplicated into 'fd' upon
 * checkpoint restart. */
struct tcmi_ckpt_openfile_dupfile_hdr {
	/** file descriptor */
	int32_t fd;
	/** file descriptor that refers to the actual file. */
	int32_t dup_fd;
} __attribute__((__packed__));

/** \<\<public\>\> Writes an open file information into the checkpoint file. */
extern int tcmi_ckpt_openfile_write(struct tcmi_ckpt *ckpt, int fd, struct file *file);
/** \<\<public\>\> Reads and restores an open file from the checkpoint file. */
extern int tcmi_ckpt_openfile_read(struct tcmi_ckpt *ckpt);


/**
 * \<\<public\>\> Class method - checks if the specified
 * file(represented by its inode) is supported by checkpoint.
 *
 * @param *inode - inode to be checked
 */
static inline int tcmi_ckpt_openfile_supported(struct inode *inode)
{
	return (S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) || S_ISCHR(inode->i_mode) || S_ISFIFO(inode->i_mode) );
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CKPT_OPENFILE_PRIVATE




/** Writes a duplicate file descriptor chunk into the checkpoint file. */
static int tcmi_ckpt_openfile_write_dupfd(struct tcmi_ckpt *ckpt, int fd, int dup_fd);

/** Writes a new file chunk into the checkpoint file. */
static int tcmi_ckpt_openfile_write_newfd(struct tcmi_ckpt *ckpt, int fd, 
					  struct file *file);

/** Reads a duplicate file descriptor chunk from the checkpoint file. */
static int tcmi_ckpt_openfile_read_dupfd(struct tcmi_ckpt *ckpt);

/** Reads a new file chunk from the checkpoint file */
static int tcmi_ckpt_openfile_read_newfd(struct tcmi_ckpt *ckpt);

static inline void tcmi_ckpt_proxy_path(char *page, char *name)
{
	char buf[128];
	snprintf(buf, 127, "%s/%s", "/proxyfs", page);
	strcpy( page, buf );
}

#endif /* TCMI_CKPT_OPENFILE_PRIVATE */


/**
 * @}
 */

#endif /* _TCMI_CKPT_OPENFILE_H */

