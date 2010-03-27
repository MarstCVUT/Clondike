/**
 * @file proxyfs_pipe_real_file.c - File class for accessing pipe files
 * (used by proxyfs_server_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_pipe_real_file.c,v 1.8 2008/05/02 19:59:20 stavam2 Exp $
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

#include <asm/poll.h>

#include <dbg.h>

#define PROXYFS_PIPE_REAL_FILE_PRIVATE
#define PROXYFS_PIPE_REAL_FILE_PROTECTED
#include "proxyfs_pipe_real_file.h"

/** \<\<private\>\> Write to file from read buffer 
 * @param *self - this real_file class instance
 *
 * @return amount of bytes written
 * */
static int proxyfs_pipe_real_file_write(struct proxyfs_real_file *self)
{
	int lenght, total= 0, poll_result, nread;
	mm_segment_t old_fs;
	void *buf_addr;
	size_t buf_data;
	loff_t pos;

	mdbg(INFO3, "[File %lu] fmode %d", proxyfs_file_get_file_ident(PROXYFS_FILE(self)),
	self->file->f_mode);	

restart_write:
	nread = proxyfs_real_file_ioctl_nread(self); // TODO: Remove this, just for debugging now
	poll_result = proxyfs_real_file_poll_write(self);
	mdbg(INFO3, "[File %lu] polling result: %d (nread: %d)", proxyfs_file_get_file_ident(PROXYFS_FILE(self)), poll_result, nread);

	if ( poll_result < 1 ) // No space for writing (in pipe it means no free buffer)
		return total;

	buf_data = circ_buf_get_read_size( & PROXYFS_FILE(self)->read_buf );

	mdbg(INFO3, "[File %lu] At least %lu bytes pending in buffer", proxyfs_file_get_file_ident(PROXYFS_FILE(self)), (unsigned long)buf_data);
	circ_buf_dump("Pipe real file write", & PROXYFS_FILE(self)->read_buf);

	if( buf_data ){
		pos = self->file->f_pos;
		if( PROXYFS_PIPE_REAL_FILE(self)->write_counter == 0 ){
			if( circ_buf_look_at( &PROXYFS_FILE(self)->read_buf, 
				       (void*)&PROXYFS_PIPE_REAL_FILE(self)->write_counter, 
				       sizeof(PROXYFS_PIPE_REAL_FILE(self)->write_counter), 0) 
				== sizeof(PROXYFS_PIPE_REAL_FILE(self)->write_counter) )
			{
				mdbg(INFO3, "[File %lu] New write block size %d.", 
						proxyfs_file_get_file_ident(PROXYFS_FILE(self)), 
						PROXYFS_PIPE_REAL_FILE(self)->write_counter );

				if( PROXYFS_PIPE_REAL_FILE(self)->write_counter <= PIPE_BUF )
					PROXYFS_PIPE_REAL_FILE(self)->atomic_write = 1;
				else
					PROXYFS_PIPE_REAL_FILE(self)->atomic_write = 0;

				circ_buf_adjust_read_ptr( & PROXYFS_FILE(self)->read_buf, 
						sizeof(PROXYFS_PIPE_REAL_FILE(self)->write_counter) );

				total += sizeof(PROXYFS_PIPE_REAL_FILE(self)->write_counter);
				goto restart_write;
			}
			else
				PROXYFS_PIPE_REAL_FILE(self)->write_counter = 0;
		}
		else if( PROXYFS_PIPE_REAL_FILE(self)->atomic_write ){ // Write must be atomic
			int total_available = circ_buf_count(& PROXYFS_FILE(self)->read_buf);
			if ( total_available < PROXYFS_PIPE_REAL_FILE(self)->write_counter )
				goto done;

			if( buf_data < PROXYFS_PIPE_REAL_FILE(self)->write_counter ){ // Data overlaps
				circ_buf_look_at( &PROXYFS_FILE(self)->read_buf, 
						PROXYFS_PIPE_REAL_FILE(self)->atomic_buf, 
						PROXYFS_PIPE_REAL_FILE(self)->write_counter, 0);

				old_fs = get_fs();
				set_fs(KERNEL_DS);
		        	lenght = vfs_write( self->file, PROXYFS_PIPE_REAL_FILE(self)->atomic_buf, 
						PROXYFS_PIPE_REAL_FILE(self)->write_counter, &pos );
				set_fs(old_fs);

			}
			else{ // Data doesn't overlap
				buf_addr = circ_buf_get_read_addr( & PROXYFS_FILE(self)->read_buf );
				old_fs = get_fs();
				set_fs(KERNEL_DS);
		        	lenght = vfs_write( self->file, buf_addr, 
						PROXYFS_PIPE_REAL_FILE(self)->write_counter, &pos );
				set_fs(old_fs);
			}

			if( lenght > 0 ){ // We wrote something
				mdbg(INFO3, "[File %lu] Atomicaly written %d Bytes", 
					proxyfs_file_get_file_ident(PROXYFS_FILE(self)), lenght);

				if( lenght != PROXYFS_PIPE_REAL_FILE(self)->write_counter )
					mdbg(ERR1, "[File %lu] Fatal error in atomic write", 
							proxyfs_file_get_file_ident(PROXYFS_FILE(self)));

				PROXYFS_PIPE_REAL_FILE(self)->write_counter = 0;
		        	self->file->f_pos = pos;
				total += lenght;
				circ_buf_adjust_read_ptr( & PROXYFS_FILE(self)->read_buf, lenght );
				goto restart_write;
			}
			else
				mdbg(INFO3, "[File %lu] Write failed with %d",  
					proxyfs_file_get_file_ident(PROXYFS_FILE(self)), lenght);
		}	
		else{ // Write hasn't to be atomic
			buf_addr = circ_buf_get_read_addr( & PROXYFS_FILE(self)->read_buf );
			if( buf_data > PROXYFS_PIPE_REAL_FILE(self)->write_counter )
				buf_data = PROXYFS_PIPE_REAL_FILE(self)->write_counter;
			
			old_fs = get_fs();
			set_fs(KERNEL_DS);
        		lenght = vfs_write( self->file, buf_addr, buf_data, &pos );
			set_fs(old_fs);

			if( lenght > 0 ){
				circ_buf_adjust_read_ptr( & PROXYFS_FILE(self)->read_buf, lenght );
			        self->file->f_pos = pos;
				PROXYFS_PIPE_REAL_FILE(self)->write_counter -= lenght;
				total += lenght;
				goto restart_write;
			}
		}
	}

done:
	return total;
}

/** \<\<private\>\> Reads from file and write to writte buffer 
 * @param *self - this real_file class instance
 *
 * @return amount of bytes read
 * */
static int proxyfs_pipe_real_file_read(struct proxyfs_real_file *self)
{
	int lenght = 0, total_read = 0, poll_result;
	mm_segment_t old_fs;
	void *buf_addr;
	size_t buf_data, total_buffer_size;
	loff_t pos;

	// We read from pipe ONLY in case the remote process ever used it as a read pipe..
	// Workaround for inpolite processes like Makefile.. see MSG_READ_REQUEST comments for details
	if (!proxyfs_file_was_read_requested(PROXYFS_FILE(self)) )
		return -EAGAIN;

	poll_result = proxyfs_real_file_poll_read(self);
	mdbg(INFO3, "[File %lu] polling result: %d", proxyfs_file_get_file_ident(PROXYFS_FILE(self)), poll_result);

	if ( poll_result < 1 ) // Are there some date pending?
		return -EAGAIN;

	total_buffer_size = circ_buf_free_space( &PROXYFS_FILE(self)->write_buf );
	if ( total_buffer_size == 0 ) // We do not have free space in the bufer at the moment
		return -EAGAIN;

	buf_data = circ_buf_get_write_size( & PROXYFS_FILE(self)->write_buf );

	mdbg(INFO3, "Trying read file %lu, buffer space is at least %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)), (unsigned long)buf_data);
	circ_buf_dump("Pipe real file read", & PROXYFS_FILE(self)->write_buf);

	if( buf_data ){
		buf_addr = circ_buf_get_write_addr( & PROXYFS_FILE(self)->write_buf );
		pos      = self->file->f_pos;
		old_fs   = get_fs();

		set_fs(KERNEL_DS);

        	lenght = vfs_read(self->file, buf_addr, buf_data, &pos);

		if( lenght > 0 ){
			total_read = lenght;
			mdbg(INFO3, "[File %lu] Read from pipe content %.*s.", proxyfs_file_get_file_ident(PROXYFS_FILE(self)), (int)lenght, (char*)buf_addr );


			circ_buf_adjust_write_ptr( & PROXYFS_FILE(self)->write_buf, lenght );
			// If we wrote everything, we try to write one more time, 
			// because we are using circular buffer
			if( lenght == buf_data ){ 
				buf_data = circ_buf_get_write_size( & PROXYFS_FILE(self)->write_buf );
				if( buf_data ){
					// We have to poll once more, since now there may be no more data!
					if ( !proxyfs_real_file_poll_read(self) ) {						
						self->file->f_pos = pos;
						set_fs(old_fs);					
						goto out;
					}

					buf_addr = circ_buf_get_write_addr( & PROXYFS_FILE(self)->write_buf );
        				lenght = vfs_read(self->file, buf_addr, buf_data, &pos);
					if( lenght > 0 ) {
						circ_buf_adjust_write_ptr( & PROXYFS_FILE(self)->write_buf, lenght ); 
						total_read += lenght;
					}
				}
			}
		}
	
	        self->file->f_pos = pos;
		set_fs(old_fs);
	}

out:
	return total_read;
}

/** \<\<private\>\> "Open" file and send response message 
 * @param *self - pointer to this file instance
 * @param *peer - instance of peer which is opening this file
 *
 * @return zero on success
 */
static int proxyfs_pipe_real_file_open(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer)
{
	struct proxyfs_msg *msg;
	static u_int32_t const type = PROXYFS_FILE_PIPE;

	msg = proxyfs_msg_new(MSG_OPEN_RESP_OK,	proxyfs_file_get_file_ident(PROXYFS_FILE(self)), sizeof(type), (void*)&type);


	if( msg != NULL ){
		proxyfs_peer_send_msg( peer, msg );
		//proxyfs_file_set_peer(PROXYFS_FILE(self), peer);
		return 0;
	}

	return -1;
}

/** \<\<public\>\> Initialize tty_real_file 
 * @param *self - pointer to this file instance
 *
 * @return zero on success
 * */
int proxyfs_pipe_real_file_init(struct proxyfs_pipe_real_file *self)
{
	PROXYFS_REAL_FILE(self)->ops = &pipe_real_file_ops;
	self->write_counter = 0;

	return 0;
}

/** \<\<public\>\> pipe_real_file construktor 
 * @return new proxyfs_pipe_real_file instance or NULL on error
 * */
struct proxyfs_pipe_real_file *proxyfs_pipe_real_file_new(void)
{
	struct proxyfs_pipe_real_file *self;

	self = (struct proxyfs_pipe_real_file*)kmalloc(sizeof(struct proxyfs_pipe_real_file), GFP_KERNEL);
	if( self == NULL ){
		mdbg(ERR3, "Allocating new proxyfs_pipe_real_file failed");
		goto exit0;
	}
	
	if( proxyfs_real_file_init( PROXYFS_REAL_FILE(self) ) != 0 )
		goto exit1;
	if( proxyfs_pipe_real_file_init( self ) != 0 )
		goto exit2;
	
	return self;

exit2:
	proxyfs_real_file_destroy( PROXYFS_REAL_FILE(self) );
exit1:
	kfree(self);
exit0:
	return self;
}

/** \<\<public\>\> Duplicates a proxyfs real file
 *
 * @param *self - pointer to the instance to be duplicated
 * @return Duplicated file instance
 */
static struct proxyfs_real_file* proxyfs_pipe_real_file_duplicate(struct proxyfs_real_file *self)
{

	struct proxyfs_pipe_real_file *clone;
	mdbg(INFO4, "Duplicating file %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));	
	clone = (struct proxyfs_pipe_real_file*)kmalloc(sizeof(struct proxyfs_pipe_real_file), GFP_KERNEL);	
	if ( !clone )
		return NULL;

	if( proxyfs_real_file_init( PROXYFS_REAL_FILE(clone) ) != 0 )
		goto exit1;

	if( proxyfs_pipe_real_file_init( PROXYFS_PIPE_REAL_FILE(clone) ) != 0 )
		goto exit2;

	PROXYFS_FILE(clone)->file_identifier.file_ident = PROXYFS_FILE(self)->file_identifier.file_ident;
	PROXYFS_FILE(clone)->file_identifier.file_owner_pid = PROXYFS_FILE(self)->file_identifier.file_owner_pid;
	PROXYFS_REAL_FILE(clone)->file = PROXYFS_REAL_FILE(self)->file;
	// We can do this here as we own a reference to file (the file being clonned) and we have a lock holding the file so it cannot be released
	get_file(PROXYFS_REAL_FILE(self)->file);

	return PROXYFS_REAL_FILE(clone);

exit2:
	proxyfs_real_file_destroy( PROXYFS_REAL_FILE(self) );
exit1:
	kfree(self);	
	return NULL;
}

