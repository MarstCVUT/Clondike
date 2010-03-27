/**
 * @file proxyfs_file.c - Main file class (used by proxyfs_task)
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_file.c,v 1.6 2008/05/02 19:59:20 stavam2 Exp $
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
#include <dbg.h>

#define PROXYFS_FILE_PRIVATE
#define PROXYFS_FILE_PROTECTED // Own header
#include "proxyfs_file.h"
#include "proxyfs_peer.h"

#include "buffer.h"
#include "virtual.h"

/** \<\<public\>\> Get file status 
 * @param self - pointer to this file instance
 * @param status - status we asking for
 *
 * @return true if status is set for file
 * */
unsigned proxyfs_file_get_status(struct proxyfs_file_t* self, unsigned status)
{
	unsigned rtn = 0;
	if( status & PROXYFS_FILE_RO_STATUS ){
		if( status & PROXYFS_FILE_CAN_WRITE ){
			if( circ_buf_get_write_size( & self->write_buf ) )
				rtn |= PROXYFS_FILE_CAN_WRITE;
		}
		if( status & PROXYFS_FILE_CAN_READ ){
			if( circ_buf_count( & self->read_buf ) )
				rtn |= PROXYFS_FILE_CAN_READ;
		}
		if( status & PROXYFS_FILE_ALL_WRITTEN ){
			mdbg(INFO3, "file_get_status[%lu] all_written.. %d",proxyfs_file_get_file_ident(PROXYFS_FILE(self)),circ_buf_get_read_size(&self->write_buf));
			if( circ_buf_get_read_size( & self->write_buf ) == 0 )
				rtn |= PROXYFS_FILE_ALL_WRITTEN;
		}
		if( status & PROXYFS_FILE_ALL_READ ){
			mdbg(INFO3, "file_get_status[%lu] all_read.. %d",proxyfs_file_get_file_ident(PROXYFS_FILE(self)),circ_buf_get_read_size( & self->read_buf ));
			if( circ_buf_get_read_size( & self->read_buf ) == 0 )
				rtn |= PROXYFS_FILE_ALL_READ;
		}
	}
	if( status & PROXYFS_FILE_RW_STATUS ){
		rtn |= self->state & (status & PROXYFS_FILE_RW_STATUS);
	}
	return rtn;
}

/** \<\<public\>\> Get file identifier */
unsigned long proxyfs_file_get_file_ident(struct proxyfs_file_t* self) {
	unsigned long res = ((uint32_t)self->file_identifier.file_owner_pid << 16) + self->file_identifier.file_ident;
	return res;
}

/** \<\<public\>\> Set file identifier 
 * @param self - File instance pointer
 * @param encuded_identifier - File identifier encoded via get_file_ident method
 */
void proxyfs_file_set_file_ident(struct proxyfs_file_t* self, unsigned long encoded_identifier) {
	self->file_identifier.file_ident = 0xffff & encoded_identifier;
	self->file_identifier.file_owner_pid = (encoded_identifier - self->file_identifier.file_ident) >> 16;
	mdbg(INFO3, "Set identifier: %d Pid: %d (encoded: %lu)", (int)self->file_identifier.file_ident, (int)self->file_identifier.file_owner_pid, encoded_identifier);
}


/** \<\<public\>\> Set file status 
 * @param self - pointer to this file instance
 * @param status - staus we asking for
 *
 * */
void proxyfs_file_set_status(struct proxyfs_file_t* self, unsigned status)
{
	self->state |= status;
}

/** \<\<public\>\> Set file status 
 * @param self - pointer to this file instance
 * @param status - staus we asking for
 *
 * */
void proxyfs_file_unset_status(struct proxyfs_file_t* self, unsigned status)
{
	self->state &= ~status;
}

/** \<\<public\>\> Set file status 
 * @param ident - Unique file identification number
 * @param start - List in which we are searching
 *
 * @return pointer to proxyfs_file or NULL
 * */
struct proxyfs_file_t* proxyfs_file_find_in_list(unsigned long ident, struct list_head *start)
{
	struct proxyfs_file_t *file = NULL; 
	struct list_head *l;               

	list_for_each( l, start ){  
		file = list_entry(l, struct proxyfs_file_t, files);
		if( proxyfs_file_get_file_ident(file) == ident )
			return file;
	}
	return NULL;	
}	

/** \<\<public\>\> Initialize proxyfs_file struct
 * @param self - pointer to this file instance
 *
 * @return zero on success 
 * */
int proxyfs_file_init(struct proxyfs_file_t* self)
{
	if( circ_buf_init( & self->write_buf, WRITE_BUF_SIZE ) != 0 ){
		mdbg(ERR3, "Allocating write buffer for proxy_file failed");
		goto exit0;
	}
	if( circ_buf_init( & self->read_buf, READ_BUF_SIZE ) != 0 ){
		mdbg(ERR3, "Allocating read buffer for proxy_file failed");
		goto exit1;
	}

	self->state = 0;
	self->write_buf_unconfirmed = 0;
	self->read_was_requested = 0;
	self->peer = NULL;
	return 0;
exit1:
	circ_buf_destroy( & self->write_buf ); 
exit0:
	return -1;
}

/** \<\<public\>\> Deinitialize proxyfs_file struct
 * @param self - pointer to this file instance
 *
 * */
void proxyfs_file_destroy(struct proxyfs_file_t* self)
{
	circ_buf_destroy( & self->write_buf ); 
	circ_buf_destroy( & self->read_buf ); 
	proxyfs_peer_put(self->peer);
}

/** \<\<public\>\> Writes to file from peer - writes to read buffer */
int proxyfs_file_write_from_peer(struct proxyfs_file_t *self, const char *buf, size_t count)
{
	VIRTUAL_FUNC(write_from_peer, buf, count);
}

/** \<\<public\>\> Reads from file to peer - reads from write buffer */
int proxyfs_file_read_to_peer(struct proxyfs_file_t *self, char *buf, size_t count)
{
	VIRTUAL_FUNC(read_to_peer, buf, count);
}

/** \<\<public\>\> Submit that data was written to file on other side */
void proxyfs_file_submit_read_to_peer(struct proxyfs_file_t *self, size_t count)
{
	VIRTUAL_FUNC_VOID(submit_read_to_peer, count);
}

/** \<\<public\>\> Get maximum amount and address of data, that can be read at one time */
void proxyfs_file_write_buf_info(struct proxyfs_file_t *self, size_t* total_size, size_t* size, void** addr)
{
	VIRTUAL_FUNC_VOID(write_buf_info, total_size, size, addr);
}

/** \<\<public\>\> Sets peer and aquires its reference. If there was some other peer set before, releases its reference */
void proxyfs_file_set_peer(struct proxyfs_file_t *self, struct proxyfs_peer_t *peer) {
	if ( self->peer ) 
		mdbg(INFO3, "Switching from peer %p to peer %p", self->peer, peer);		
	proxyfs_peer_get(peer); // Aquire ref of a new peer
	proxyfs_peer_put(self->peer); // Release current peer
	self->peer = peer;
}

/** \<\<public\>\> Checks, whether read was requested for this file */
int proxyfs_file_was_read_requested(struct proxyfs_file_t* self) {
	return self->read_was_requested;
}

/** \<\<public\>\> Marks the file as read requested */
void proxyfs_file_mark_read_requested(struct proxyfs_file_t* self) {
	self->read_was_requested = 1;
}

