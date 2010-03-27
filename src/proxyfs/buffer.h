/**
 * @file buffer.h - Class representing circullar buffer.
 *                      
 * 
 *
 *
 * Date: 08/12/2007
 *
 * Author: Petr Malat
 *
 * $Id: buffer.h,v 1.4 2008/05/02 19:59:20 stavam2 Exp $
 *
 * This file is part of Proxy filesystem (ProxyFS)
 * Copyleft (C) 2007  Petr Malat
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
#ifndef _BUFFER_H
#define _BUFFER_H

#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

/**
 * @defgroup circ_buf_clas circ_buf class
 * @ingroup proxyfs_module_class
 *
 * This class represent circular buffer used as read/write file buffers in ProxyFS
 *
 * @{
 */

#define CIRC_BUF_USE_VMALLOC ( 4 << 10 )

/** Circular buffer structure */
struct circ_buf {
	/** Pointer to memory which is used for buffering */
	void *buf;
	/** Write pointer */
	void *write;
	/** Read pointer */
	void *read;
	/** True if buffer is full */
	int full;
	/** Memory size */
	unsigned size;
};

/** \<\<public\>\> Initialize buffer structure 
 * @param self - pointer to this buffer instance
 * @param size - buffer size
 *
 * @return zero on success
 */

/** \<\<public\>\> Write to buffer */
int circ_buf_write(struct circ_buf *self, const char *buf, unsigned count);

/** \<\<public\>\> Write to buffer from user space  */
int circ_buf_write_user(struct circ_buf *self, const char *buf, unsigned count);

/** \<\<public\>\> Destructively read from buffer */
int circ_buf_read(struct circ_buf *self, char *buf, unsigned count);

/** \<\<public\>\> Destructively read from buffer to userspace */
int circ_buf_read_user(struct circ_buf *self, char *buf, unsigned count);

static inline int circ_buf_init(struct circ_buf *self, unsigned size)
{
	if( size >= CIRC_BUF_USE_VMALLOC )
		self->buf = vmalloc( size );
	else
		self->buf = kmalloc( size, GFP_KERNEL );

	if( self->buf == NULL ) return -1;

	self->size = size;
	self->read = self->buf;
	self->write = self->buf;
	self->full = 0;
	return 0;
}

/** \<\<public\>\> Opossite function to circ_buf_init
 * @param self - pointer to this buffer instance
 */
static inline void circ_buf_destroy(struct circ_buf *self)
{
	if( self->size >= CIRC_BUF_USE_VMALLOC )
	 	vfree(self->buf);
	else
		kfree(self->buf);
	self->size = 0;
	self->read = NULL;
	self->write = NULL;
}

/** \<\<public\>\> Returns maximum amount of data which  are continuos - 
 * can by copied with one memcpy call
 * @param self - pointer to this buffer instance
 *
 * @return the amount of data
 */
static inline unsigned circ_buf_get_read_size(const struct circ_buf *self)
{
	if( self->write > self->read )
		return self->write - self->read;
	else if( self->write < self->read || self->full )
		return (self->buf + self->size) - self->read;
	return 0;
}

/** \<\<public\>\> Returns base address for reading - 
 * @param self - pointer to this buffer instance
 *
 * @return address from which read should start 
 */
static inline void *circ_buf_get_read_addr(const struct circ_buf *self)
{
	return self->read;
}

/** \<\<public\>\> Returns maximum amount of free space which is continuos - 
 * can by filled with one memcpy call
 * @param self - pointer to this buffer instance
 *
 * @return the amount of data
 */
static inline unsigned circ_buf_get_write_size(const struct circ_buf *self)
{
	if( self->write > self->read )
		return self->buf + self->size - self->write;
	else if( self->write < self->read )
		return self->read - self->write;
	else if( self->full )
		return 0;
	// buf is empty
	return self->buf + self->size - self->write;
}

/** \<\<public\>\> Returns base address for writting - 
 * @param self - pointer to this buffer instance
 *
 * @return address from which write should start 
 */
static inline void *circ_buf_get_write_addr(const struct circ_buf *self)
{
	return self->write;
}

/** \<\<public\>\> Adjust read pointer
 * @param self - pointer to this buffer instance
 * @param count - how much adjust <b>WARNING: count must be between 0 and data total in buffer</b>
 */
static inline void circ_buf_adjust_read_ptr(struct circ_buf *self, const unsigned count)
{
	if( count > 0 ){
		self->full = 0;
		self->read += count;
		if( self->read >= self->buf + self->size || self->read < self->buf )
			self->read -= self->size;		
	}
}

/** \<\<public\>\> Adjust write pointer
 * @param self - pointer to this buffer instance
 * @param count - how much adjust <b>WARNING: count must be between 0 and free space in buffer</b>
 */
static inline void circ_buf_adjust_write_ptr(struct circ_buf *self, const unsigned count)
{
	if( count > 0 ){
		self->write += count;
		if( self->write >= self->buf + self->size || self->write < self->buf )
			self->write -= self->size;
		if( self->write == self->read ) self->full = 1;
	}
}

/** \<\<public\>\> get total free space in buffer
 * @param self - pointer to this buffer instance
 *
 * @return amount of free bytes in buffer
 */
static inline unsigned circ_buf_free_space(struct circ_buf *self)
{
	if( self->write > self->read )
		return (self->size - (self->write - self->read));
	else if( self->write < self->read )
		return self->read - self->write;
	else if( self->full )
		return 0;
	// buf is empty
	return self->size;
}

/** \<\<public\>\> get count of total data contained in the buffer
 * @param self - pointer to this buffer instance
 *
 * @return total bytes contained by the buffer
 */
static inline unsigned circ_buf_count(struct circ_buf *self)
{
	return self->size - circ_buf_free_space(self);
}

/** \<\<public\>\> Nondestructively read from buffer at offset */
int circ_buf_look_at(struct circ_buf *self, char *buf, unsigned count, size_t offset);

/** \<\<public\>\> Method used for debugging purposes. Dumps content of a circular buffer to debug output */
void circ_buf_dump(const char* prefix, struct circ_buf *self);

/**
 * @}
 */


#endif // _BUFER_H
