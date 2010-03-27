/**
 * @file proxyfs_generic_real_file.c - File class for accessing generic files
 * (used by proxyfs_server_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_generic_real_file.c,v 1.6 2008/01/17 14:36:44 stavam2 Exp $
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


#include <linux/fs.h>
#include <linux/smp_lock.h>
#include <linux/vfs.h>

#include <dbg.h>

#include <asm/poll.h>

#define PROXYFS_GENERIC_REAL_FILE_PRIVATE
#define PROXYFS_GENERIC_REAL_FILE_PROTECTED
#include "proxyfs_generic_real_file.h"

/** \<\<private\>\> Write to file from read buffer 
 * @param *self - this real_file class instance
 *
 * @return amount of bytes written
 * */
static int proxyfs_generic_real_file_write(struct proxyfs_real_file *self)
{
	int lenght = 0, total= 0;
	mm_segment_t old_fs;
	void *buf_addr;
	size_t buf_data;
	loff_t pos;

	buf_data = circ_buf_get_read_size( & PROXYFS_FILE(self)->read_buf );

	mdbg(INFO3, "File %lu has at least %lu bytes pending in buffer.", proxyfs_file_get_file_ident(PROXYFS_FILE(self)), (unsigned long)buf_data );

	if( buf_data ){
		buf_addr = circ_buf_get_read_addr( & PROXYFS_FILE(self)->read_buf );
		pos      = self->file->f_pos;
		old_fs   = get_fs();

		set_fs(KERNEL_DS);

        	lenght = vfs_write( self->file, buf_addr, buf_data, &pos );
		mdbg(INFO3, "vfs_write returned %d", lenght );

		if( lenght > 0 ){
			circ_buf_adjust_read_ptr( & PROXYFS_FILE(self)->read_buf, lenght );
			total = lenght;
			// If we wrote everything, we try to write one more time,
			// because we are using circular buffer
			if( lenght == buf_data ){ 
				buf_data = circ_buf_get_read_size( & PROXYFS_FILE(self)->read_buf );
				if( buf_data ){
					buf_addr = circ_buf_get_read_addr( & PROXYFS_FILE(self)->read_buf );
        				lenght   = vfs_write(self->file, buf_addr, buf_data, &pos);
					mdbg(INFO3, "vfs_write returned %d", lenght );
					if( lenght > 0 ){
						circ_buf_adjust_read_ptr( & PROXYFS_FILE(self)->read_buf, lenght );
						total += lenght;
					}
				}
			}
		}
		else
	
	        self->file->f_pos = pos;
		set_fs(old_fs);
	}
	return total;
}


/** \<\<private\>\> Reads from file and write to writte buffer 
 * @param *self - this real_file class instance
 *
 * @return amount of bytes read
 * */
static int proxyfs_generic_real_file_read(struct proxyfs_real_file *self)
{
	int lenght = 0;
	mm_segment_t old_fs;
	void *buf_addr;
	size_t buf_data;
	loff_t pos;
        
	buf_data = circ_buf_get_write_size( & PROXYFS_FILE(self)->write_buf );
	mdbg(INFO3, "Trying read file %lu, buffer space is at least %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)), (unsigned long)buf_data );

	if( buf_data ){
		buf_addr = circ_buf_get_write_addr( & PROXYFS_FILE(self)->write_buf );
		pos      = self->file->f_pos;
		old_fs   = get_fs();

		set_fs(KERNEL_DS);

        	lenght = vfs_read(self->file, buf_addr, buf_data, &pos);

		if( lenght > 0 ){
			circ_buf_adjust_write_ptr( & PROXYFS_FILE(self)->write_buf, lenght );
			// If we wrote everything, we try to write one more time, 
			// because we are using circular buffer
			if( lenght == buf_data ){ 
				buf_data = circ_buf_get_write_size( & PROXYFS_FILE(self)->write_buf );
				if( buf_data ){
					buf_addr = circ_buf_get_write_addr( & PROXYFS_FILE(self)->write_buf );
        				lenght = vfs_read(self->file, buf_addr, buf_data, &pos);
					if( lenght > 0 ) 
						circ_buf_adjust_write_ptr( & PROXYFS_FILE(self)->write_buf, lenght ); 
				}
			}
		}
	
	        self->file->f_pos = pos;
		set_fs(old_fs);
	}

	return lenght; // FIXME
}

/** \<\<private\>\> "Open" file and send response message 
 * @param *self - pointer to this file instance
 * @param *peer - instance of peer which is opening this file
 *
 * @return zero on success
 */
static int proxyfs_generic_real_file_open(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer)
{
	struct proxyfs_msg *msg;
	static u_int32_t const type = PROXYFS_FILE_GENERIC;

	msg = proxyfs_msg_new(MSG_OPEN_RESP_OK,	proxyfs_file_get_file_ident(PROXYFS_FILE(self)), sizeof(type), (void*)&type);

	if( msg != NULL ){
		proxyfs_peer_send_msg( peer, msg );
		proxyfs_file_set_peer(PROXYFS_FILE(self), peer);
		return 0;
	}

	return -1;
}

/** \<\<public\>\> Initialize tty_real_file 
 * @param *self - pointer to this file instance
 *
 * @return zero on success
 * */
int proxyfs_generic_real_file_init(struct proxyfs_generic_real_file *self)
{
	PROXYFS_REAL_FILE(self)->ops = &generic_real_file_ops;
	return 0;
}

/** \<\<public\>\> generic_real_file construktor 
 * @return new proxyfs_generic_real_file instance or NULL on error
 * */
struct proxyfs_generic_real_file *proxyfs_generic_real_file_new(void)
{
	struct proxyfs_generic_real_file *self;

	self = (struct proxyfs_generic_real_file*)kmalloc(sizeof(struct proxyfs_generic_real_file), GFP_KERNEL);
	if( self == NULL ){
		mdbg(ERR3, "Allocating new proxyfs_generic_real_file failed");
		goto exit0;
	}
	
	if( proxyfs_real_file_init( PROXYFS_REAL_FILE(self) ) != 0 )
		goto exit1;
	if( proxyfs_generic_real_file_init( self ) != 0 )
		goto exit2;
	
	return self;

exit2:
	proxyfs_real_file_destroy( PROXYFS_REAL_FILE(self) );
exit1:
	kfree(self);
exit0:
	return self;
}
