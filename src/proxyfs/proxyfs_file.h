/**
 * @file proxyfs_file.h - Main file class (used by proxyfs_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_file.h,v 1.6 2008/05/02 19:59:20 stavam2 Exp $
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
#ifndef _PROXYFS_FILE_H_PUBLIC
#define _PROXYFS_FILE_H_PUBLIC

#include <linux/list.h>

#define WRITE_BUF_SIZE (2 << 12)
#define READ_BUF_SIZE (2 << 12)
/**
 * @defgroup proxyfs_file_class proxyfs_file class
 * @ingroup proxyfs_module_class
 *
 * This is used on between VFS 
 * and proxyfs_client task.
 *
 * @{
 */

/** \<\<public\>\> proxy_file_t declaration */
struct proxyfs_file_t;
struct proxyfs_peer_t;

/** Unique identifier of a file managed by the proxy fs system */
struct proxyfs_file_identifier {
	/** Semi-unique number identificating file. It is not unique on its own, but only in combination with pid */
	uint16_t file_ident;
	/** Pid of the process on CCN that owned the file before we overtook it into the proxyfs */
	uint16_t file_owner_pid;
}  __attribute__((__packed__));

/** \<\<public\>\> Get file status */
unsigned proxyfs_file_get_status(struct proxyfs_file_t* self, unsigned status);

/** \<\<public\>\> Get file identifier */
unsigned long proxyfs_file_get_file_ident(struct proxyfs_file_t* self);

/** \<\<public\>\> Set file identifier */
void proxyfs_file_set_file_ident(struct proxyfs_file_t* self, unsigned long encoded_identifier);

/** \<\<public\>\> Set file status */
struct proxyfs_file_t* proxyfs_file_find_in_list(unsigned long ident, struct list_head *start);

/** \<\<public\>\> Initialize proxyfs_file struct*/
int proxyfs_file_init(struct proxyfs_file_t* self);

/** \<\<public\>\> Deinitialize proxyfs_file struct*/
void proxyfs_file_destroy(struct proxyfs_file_t* self);

/** \<\<public\>\> Writes to file from peer - writes to read buffer */
int proxyfs_file_write_from_peer(struct proxyfs_file_t *self, const char *buf, size_t count);

/** \<\<public\>\> Reads from file to peer - reads from write buffer */
int proxyfs_file_read_to_peer(struct proxyfs_file_t *self, char *buf, size_t count);

/** \<\<public\>\> Submit that data was written to file on other side */
void proxyfs_file_submit_read_to_peer(struct proxyfs_file_t *self, size_t count);

/** \<\<public\>\> Get maximum amount and address of data, that can be read at one time */
void proxyfs_file_write_buf_info(struct proxyfs_file_t *self, size_t* total_size, size_t* size, void** addr);

/** \<\<public\>\> Sets peer and aquires its reference. If there was some other peer set before, releases its reference */
void proxyfs_file_set_peer(struct proxyfs_file_t *self, struct proxyfs_peer_t *peer);

/** \<\<public\>\> Checks, whether read was requested for this file */
int proxyfs_file_was_read_requested(struct proxyfs_file_t* self);
/** \<\<public\>\> Marks the file as read requested */
void proxyfs_file_mark_read_requested(struct proxyfs_file_t* self);


/** Cast to struct proxyfs_file_t* */
#define PROXYFS_FILE(arg) ((struct proxyfs_file_t*)(arg))

/** \<\<public\>\> Unset file status */
void proxyfs_file_unset_status(struct proxyfs_file_t* self, unsigned status);

/** \<\<public\>\> Set file status */
void proxyfs_file_set_status(struct proxyfs_file_t* self, unsigned status);
/** The file is opened and both sides known about it */
#define PROXYFS_FILE_OPENED 		1
/** The other side rejected opening file */
#define PROXYFS_FILE_DONT_EXISTS 	2
/** We are at and of file */
#define PROXYFS_FILE_READ_EOF 		4
/** There is space in write buffer (read only state) */
#define PROXYFS_FILE_CAN_WRITE 		8
/** There is space in read buffer (read only state) */
#define PROXYFS_FILE_CAN_READ 		16
/** There are no data pending in write buffer (read only state) */
#define PROXYFS_FILE_ALL_WRITTEN	32
/** There are no data pending in read buffer (read only state) */
#define PROXYFS_FILE_ALL_READ		64
/** Syscall transaction on file  */
#define PROXYFS_FILE_IN_SYSCALL		128
/** The file is opened and both sides known about it */
#define PROXYFS_FILE_CLOSED 		256
/** Reading end on peer is closed (pipe or socket only) */
#define PROXYFS_FILE_EPIPE		512	

/** Mask of read only file status */
#define PROXYFS_FILE_RO_STATUS ( PROXYFS_FILE_CAN_WRITE | PROXYFS_FILE_CAN_READ | PROXYFS_FILE_ALL_WRITTEN | \
				 PROXYFS_FILE_ALL_READ )
/** Mask of read/write file status */
#define PROXYFS_FILE_RW_STATUS ( PROXYFS_FILE_OPENED | PROXYFS_FILE_DONT_EXISTS | PROXYFS_FILE_READ_EOF | \
				 PROXYFS_FILE_CLOSED | PROXYFS_FILE_EPIPE       | PROXYFS_FILE_IN_SYSCALL )
					
/** TTY file type */
#define PROXYFS_FILE_TTY	1
/** Generic file type */
#define PROXYFS_FILE_GENERIC	2
/** Pipe file type */
#define PROXYFS_FILE_PIPE	3

#endif // _PROXYFS_FILE_H_PUBLIC


/********************** PROTECTED METHODS AND DATA ******************************/

#ifdef PROXYFS_FILE_PROTECTED
#ifndef _PROXYFS_FILE_H_PROTECTED
#define _PROXYFS_FILE_H_PROTECTED

#include <proxyfs/buffer.h>

/** \<\<protected\>\> Operations supporting polymorphism */
struct proxyfs_file_ops {
	/** Writes to file from peer - writes to read buffer */
	int (*write_from_peer)(struct proxyfs_file_t *, const char *, size_t);
	/** Reads from file to peer - reads from write buffer */
	int (*read_to_peer)(struct proxyfs_file_t *, char *, size_t);
	/** Call this to submit that data was writen to file on other side */
	void (*submit_read_to_peer)(struct proxyfs_file_t *, size_t);
	/** Get write buf info */
	void (*write_buf_info)(struct proxyfs_file_t *, size_t*, size_t*, void**);
};

/** \<\<protected\>\> Structure on which VFS and proxyfs_client task are interacting */
struct proxyfs_file_t {
	/** Unique file identifier */
	struct proxyfs_file_identifier file_identifier;

	/** Circular buffer for write() */
	struct circ_buf write_buf;
	/** Circular buffer for read() */
	struct circ_buf read_buf;

	/** Unconfirmed amount of data which was send */
	size_t write_buf_unconfirmed;

	/** Helper variable, telling whether the file was ever requested to read some data. see MSG_READ_REQUEST comment for details */	
	int read_was_requested;

	/** State of file */
	unsigned state;
	
	/** Used when adding this structure to lists */
	struct list_head files;

	/** Peer asociated with this file */
	struct proxyfs_peer_t *peer;
	/** Polymorphism */
	struct proxyfs_file_ops *ops;
};

#endif // _PROXYFS_FILE_H_PROTECTED
#endif //PROXYFS_FILE_PROTECTED

/**
 * @}
 */

