/**
 * @file proxyfs_tty_real_file.c - File class sepcialized on tty access 
 * (used by proxyfs_server_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_tty_real_file.c,v 1.9 2008/05/02 19:59:20 stavam2 Exp $
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
#include <linux/tty.h>
#include <linux/sched.h>

#include <dbg.h>

#include <asm/poll.h>

#define PROXYFS_TTY_REAL_FILE_PROTECTED
#define PROXYFS_TTY_REAL_FILE_PRIVATE
#include "proxyfs_tty_real_file.h"

/** \<\<private\>\> Write to file from read buffer 
 * @param *self - this real_file class instance
 *
 * @return amount of bytes written
 * */
static int proxyfs_tty_real_file_write(struct proxyfs_real_file *self)
{
	int lenght = 0, total= 0;
	mm_segment_t old_fs;
	void *buf_addr;
	size_t buf_data;
	loff_t pos;

	buf_data = circ_buf_get_read_size( & PROXYFS_FILE(self)->read_buf );

	mdbg(INFO3, "File %lu has at least %lu bytes pending in buffer", proxyfs_file_get_file_ident(PROXYFS_FILE(self)), (unsigned long)buf_data);

	circ_buf_dump("Tty real file write", & PROXYFS_FILE(self)->read_buf);

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
static int proxyfs_tty_real_file_read(struct proxyfs_real_file *self)
{
	int length = 0, total_read = 0;
	mm_segment_t old_fs;
	void *buf_addr;
	size_t buf_data;
	loff_t pos;
	
	if ( proxyfs_real_file_poll_read(self) < 1 ) // Are there some date pending?
		return -EAGAIN;

	buf_data = circ_buf_get_write_size( & PROXYFS_FILE(self)->write_buf );
	mdbg(INFO3, "Trying read file %lu, buffer space is at least %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)), (unsigned long)buf_data );
	circ_buf_dump("Tty real file read", & PROXYFS_FILE(self)->write_buf);

	if( buf_data ){
		buf_addr = circ_buf_get_write_addr( & PROXYFS_FILE(self)->write_buf );
		pos      = self->file->f_pos;
		old_fs   = get_fs();

		set_fs(KERNEL_DS);

        	length = proxyfs_tty_real_file_tty_read(self->file, buf_addr, buf_data, &pos);
		if( length > 0 ){
			total_read = length;
			circ_buf_adjust_write_ptr( & PROXYFS_FILE(self)->write_buf, length );
			// If we wrote everything, we try to write one more time, 
			// because we are using circular buffer
			if( length == buf_data ){ 
				buf_data = circ_buf_get_write_size( & PROXYFS_FILE(self)->write_buf );
				if( buf_data ){
					buf_addr = circ_buf_get_write_addr( & PROXYFS_FILE(self)->write_buf );
        				length = proxyfs_tty_real_file_tty_read(self->file, buf_addr, buf_data, &pos);
					if( length > 0 )  {
						circ_buf_adjust_write_ptr( & PROXYFS_FILE(self)->write_buf, length ); 
						total_read += length;
					}
				}
			}
		}

	        self->file->f_pos = pos;
		set_fs(old_fs);
	}

	return total_read;
}

/** \<\<private\>\> "Open" file and send response message 
 * @param *self - pointer to this file instance
 * @param *peer - instance of peer which is opening this file
 *
 * @return zero on success
 */
static int proxyfs_tty_real_file_open(struct proxyfs_real_file *self, struct proxyfs_peer_t *peer)
{
	struct proxyfs_msg *msg;
	static u_int32_t const type = PROXYFS_FILE_TTY;

	msg = proxyfs_msg_new(MSG_OPEN_RESP_OK,	proxyfs_file_get_file_ident(PROXYFS_FILE(self)), sizeof(type), (void*)&type);

	if( msg != NULL ){
		proxyfs_peer_send_msg( peer, msg );		
		return 0;
	}

	return -1;
}

/** \<\<private\>\> Read function for TTY
 * @TODO: Why is this copied here from kernel?
 *
 * @param *file   - pointer to file we wanted to read from 
 * @param *buf    - pointer to buffer in kernelspace witch will be filled with data 
 * @param count   - buffer size
 * @param *ppos - pointer to file offset
 *
 * @return Amount of bytes read or error 
 */
static ssize_t proxyfs_tty_real_file_tty_read(struct file * file, char *buf, size_t count, loff_t *ppos)
{
         int i;
         struct tty_struct * tty;
         struct inode *inode;
         struct tty_ldisc *ld = NULL;
	 int state;
 
         tty = (struct tty_struct *)file->private_data;
         inode = file->f_path.dentry->d_inode;
         //if (tty_paranoia_check(tty, inode, "tty_read"))
         //        return -EIO;
         if (!tty || (test_bit(TTY_IO_ERROR, &tty->flags)))
                 return -EIO;
 
         /* We not want to wait for the line discipline to sort out in this
            situation */
         ld = tty_ldisc_ref(tty);
	 if(ld){
	        lock_kernel();
        	if (ld->ops->read){
			state = current->state;
                	i = (ld->ops->read)(tty,file,buf,count);
			current->state = state;
		}
	        else
        	         i = -EIO;
	        tty_ldisc_deref(ld);
        	unlock_kernel();
         	if (i > 0)
                 	inode->i_atime = current_fs_time(inode->i_sb);
		return i;
	 }
	 else
		 return -EAGAIN;
}

/** \<\<public\>\> Initialize tty_real_file 
 * @param *self - pointer to this file instance
 *
 * @return zero on success
 * */
int proxyfs_tty_real_file_init(struct proxyfs_tty_real_file *self)
{
	int rtn = proxyfs_real_file_init( PROXYFS_REAL_FILE(self) );

	if( rtn == 0 )
		PROXYFS_REAL_FILE(self)->ops = &tty_real_file_ops;
	return rtn;
}

/** \<\<public\>\> Deinitialize tty_real_file 
 * @param *self - pointer to this file instance
 * */
void proxyfs_tty_real_file_destroy(struct proxyfs_tty_real_file *self)
{
	mdbg(INFO4, "Destroying tty file %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));
	proxyfs_real_file_destroy( PROXYFS_REAL_FILE(self) );
}

/** \<\<public\>\> tty_real_file constructor 
 * @return new proxyfs_tty_real_file instance or NULL on error
 * */
struct proxyfs_tty_real_file *proxyfs_tty_real_file_new(void)
{
	struct proxyfs_tty_real_file *self;

	self = (struct proxyfs_tty_real_file*)kmalloc(sizeof(struct proxyfs_tty_real_file), GFP_KERNEL);
	if( self == NULL ){
		mdbg(ERR3, "Allocating new proxyfs_tty_real_file failed");
		goto exit0;
	}
	if( proxyfs_tty_real_file_init( self ) != 0 )
		goto exit1;
	
	return self;

exit1:
	kfree(self);	
exit0:
	return NULL;
}

/** \<\<public\>\> Duplicates a proxyfs real file
 *
 * @param *self - pointer to the instance to be duplicated
 * @return Duplicated file instance
 */
static struct proxyfs_real_file* proxyfs_tty_real_file_duplicate(struct proxyfs_real_file *self)
{

	struct proxyfs_tty_real_file *clone;
	mdbg(INFO4, "Duplicating file %lu", proxyfs_file_get_file_ident(PROXYFS_FILE(self)));	
	clone = (struct proxyfs_tty_real_file*)kmalloc(sizeof(struct proxyfs_tty_real_file), GFP_KERNEL);	
	if ( !clone )
		return NULL;

	if( proxyfs_tty_real_file_init( PROXYFS_TTY_REAL_FILE(clone) ) != 0 )
		goto exit;

	PROXYFS_FILE(clone)->file_identifier.file_ident = PROXYFS_FILE(self)->file_identifier.file_ident;
	PROXYFS_FILE(clone)->file_identifier.file_owner_pid = PROXYFS_FILE(self)->file_identifier.file_owner_pid;
	PROXYFS_REAL_FILE(clone)->file = PROXYFS_REAL_FILE(self)->file;
	// We can do this here as we own a reference to file (the file being clonned) and we have a lock holding the file so it cannot be released
	get_file(PROXYFS_REAL_FILE(self)->file);

	return PROXYFS_REAL_FILE(clone);
exit:
	kfree(self);	
	return NULL;
}

