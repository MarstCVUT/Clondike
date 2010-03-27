/**
 * @file buffer.c - Class representing circullar buffer.
 *                      
 * 
 *
 *
 * Date: 08/12/2007
 *
 * Author: Petr Malat
 *
 * $Id: buffer.c,v 1.4 2008/05/02 19:59:20 stavam2 Exp $
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

#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <dbg.h>

#define BUFFER_PRIVATE
#include "buffer.h"

/** \<\<public\>\> Write to buffer 
 * @param self - pointer to this buffer instance
 * @param buf - pointer to data, which will be written
 * @param count - data size
 *
 * @return amount of bytes written 
 */
int circ_buf_write(struct circ_buf *self, const char *buf, unsigned count)
{
	unsigned wrs;
	wrs = circ_buf_get_write_size(self);
	if(wrs){
		if( count < wrs ){
			memcpy( circ_buf_get_write_addr(self), buf, count );
			circ_buf_adjust_write_ptr(self, count);
			return count;
		}
		else{
			memcpy( circ_buf_get_write_addr(self), buf, wrs );
			circ_buf_adjust_write_ptr(self, wrs);
			return wrs + circ_buf_write( self, buf + wrs, count - wrs );
		}
	}
	return 0;
}

/** \<\<public\>\> Write to buffer from user space 
 * @param self - pointer to this buffer instance
 * @param buf - pointer to data in userspace, which will be written
 * @param count - data size
 *
 * @return amount of bytes written 
 */
int circ_buf_write_user(struct circ_buf *self, const char *buf, unsigned count)
{
	unsigned wrs;
	void *ptr;
	wrs = circ_buf_get_write_size(self);
	if(wrs){
		if( count < wrs ){
			ptr = circ_buf_get_write_addr(self);
			if ( __copy_from_user( ptr, buf, count ) )
				return -EFAULT;
			circ_buf_adjust_write_ptr(self, count);
			return count;
		}
		else{
			ptr = circ_buf_get_write_addr(self);
			if ( __copy_from_user( ptr, buf, wrs ) )
				return -EFAULT;
			circ_buf_adjust_write_ptr(self, wrs);
			return wrs + circ_buf_write_user( self, buf + wrs, count - wrs );
		}
	}
	return 0;
}

/** \<\<public\>\> Destructively read from buffer 
 * @param self - pointer to this buffer instance
 * @param buf - pointer to memory in which will be data written
 * @param count - maximum data size. Can be actually more than current content size of buffer, in this case less bytes are read
 *
 * @return amount of bytes read 
 */
int circ_buf_read(struct circ_buf *self, char *buf, unsigned count)
{
	unsigned rs;
	unsigned maximum_size = circ_buf_count(self);
	if ( count > maximum_size )
		count = maximum_size;

	rs = circ_buf_get_read_size(self);
	if(rs){
		if( count < rs ){
			memcpy( buf, circ_buf_get_read_addr(self), count );
			circ_buf_adjust_read_ptr(self, count);
			return count;
		}
		else{
			memcpy( buf, circ_buf_get_read_addr(self), rs );
			circ_buf_adjust_read_ptr(self, rs);
			return rs + circ_buf_read( self, buf + rs, count - rs );
		}
	}
	return 0;
}

/** \<\<public\>\> Destructively read from buffer to userspace
 * @param self - pointer to this buffer instance
 * @param buf - pointer to memory in which will be data written
 * @param count - maximum data size
 *
 * @return amount of bytes read 
 */
int circ_buf_read_user(struct circ_buf *self, char *buf, unsigned count)
{
	unsigned rs;
	unsigned maximum_size = circ_buf_count(self);
	if ( count > maximum_size )
		count = maximum_size;

	rs = circ_buf_get_read_size(self);
	if(rs){
		if( count < rs ){
			if ( __copy_to_user( buf, circ_buf_get_read_addr(self), count ) )
				return -EFAULT;
			circ_buf_adjust_read_ptr(self, count);
			return count;
		}
		else{
			if ( __copy_to_user( buf, circ_buf_get_read_addr(self), rs ) )
				return -EFAULT;
			circ_buf_adjust_read_ptr(self, rs);
			return rs + circ_buf_read_user( self, buf + rs, count - rs );
		}
	}
	return 0;
}



/** \<\<public\>\> Nondestructively read from buffer at offset 
 * @param self - pointer to this buffer instance
 * @param buf - pointer to memory in which will be data written
 * @param count - maximum data size. Can be actually more than current content size of buffer, in this case less bytes are read
 * @param offset - read offset
 *
 * @return amount of bytes read 
 */
int circ_buf_look_at(struct circ_buf *self, char *buf, unsigned count, size_t offset)
{
	int rs;
	void *read = self->read;
	int full = self->full;

	circ_buf_adjust_read_ptr(self, offset);

	rs = circ_buf_read(self, buf, count);

	self->read = read;
	self->full = full;
	return rs;
}

/** \<\<public\>\> Method used for debugging purposes. Dumps content of a circular buffer to debug output */
void circ_buf_dump(const char* prefix, struct circ_buf *self) {
	if ( debug_enabled ) {
		// Size till overlap
		int read_size = circ_buf_get_read_size(self);
		// Maximum read size
		unsigned maximum_read_size = circ_buf_count(self);
		mdbg(INFO3, "%s First part content (%d) %.*s.", prefix, (int)read_size, (int)read_size, (char*)circ_buf_get_read_addr(self));
		if ( read_size < maximum_read_size ) {
			mdbg(INFO3, "%s Second part content (%d) %.*s.", prefix, (int)maximum_read_size-read_size, (int)maximum_read_size-read_size, (char*)self->buf);
		}
	}
}
