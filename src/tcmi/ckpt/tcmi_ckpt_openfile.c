/**
 * @file tcmi_ckpt_openfile.c - a helper class that provides functionality to
 *                              store/restore information about an open file in the checkpoint
 *                      
 * 
 *
 *
 * Date: 04/30/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ckpt_openfile.c,v 1.10 2007/11/05 19:38:28 stavam2 Exp $
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

#include "tcmi_ckpt.h"
#include <clondike/tcmi/tcmi_struct.h>

#define TCMI_CKPT_OPENFILE_PRIVATE
#include "tcmi_ckpt_openfile.h"

#include <proxyfs/proxyfs_server.h>
#include <proxyfs/proxyfs_helper.h>
#include <proxyfs/proxyfs_file.h>

#include <linux/syscalls.h>

/** 
 * \<\<public\>\> Writes an open file information into the
 * checkpoint file.
 * Writing the information about an open file requires:
 * - checking if there is the same file object already stored
 * in the file descriptor cache of the checkpoint. This indicates
 * that such file has already been written into the checkpoint.
 * In that case we delegate the work to write_dupfd.
 * - othewise we delegate the work to write_newfd method that
 * stores the full file information in about the file.
 *
 * @param *ckpt - checkpoint instance, that we are writing into
 * @param fd - file descriptor of associated with the file object
 * @param *file - file whose information is be written
 * @return 0 upon succes
 */
int tcmi_ckpt_openfile_write(struct tcmi_ckpt *ckpt, int fd, struct file *file)
{
	int dup_fd;
	int err;

	/* search for the file descriptor in the cache first */
	if ( (dup_fd = tcmi_ckpt_fdcache_lookup(ckpt, file)) >= 0)
		err = tcmi_ckpt_openfile_write_dupfd(ckpt, fd, dup_fd);
	/* not found, write the whole file details. */
	else
		err = tcmi_ckpt_openfile_write_newfd(ckpt, fd, file);

	return err;
}

/** 
 * \<\<public\>\> Reads and restores an open file from the checkpoint
 * file. Restoring the open file requires:
 * - detecting whether the file to be restored requires only
 * duplication of an existing file descriptor or reopening the file
 * from scratch.
 * - after reading the header, the work is delegated to corresponding
 * method.
 *
 * @param *ckpt - checkpoint instance, that we are restoring from
 * @return 0 upon succes
 */
int tcmi_ckpt_openfile_read(struct tcmi_ckpt *ckpt)
{
	int err = -EINVAL;
	struct tcmi_ckpt_openfile_generic_hdr hdr;
	if (tcmi_ckpt_read(ckpt, &hdr, sizeof(hdr)) < 0) {
			mdbg(ERR3, "Error reading file generic header");
			err = -EINVAL;
			goto exit0;
	}
	mdbg(INFO3, "Read generic file header restore type: %08x", hdr.restore_type);

	switch (hdr.restore_type) {
		case TCMI_CKPT_OPENFILE_NEWFILE:
			err = tcmi_ckpt_openfile_read_newfd(ckpt);
			break;
		case TCMI_CKPT_OPENFILE_DUPFILE:
			err = tcmi_ckpt_openfile_read_dupfd(ckpt);
			break;
		default:
			mdbg(ERR3, "Unknown restore type %x", hdr.restore_type);
			err = -EINVAL;
			break;
	}
	return err;

	/* error handling */
 exit0:
	return -EINVAL;
}


/** @addtogroup tcmi_ckpt_openfile_class
 *
 * @{
 */

/**
 *  \<\<private\>\> Writes a duplicate file descriptor chunk into the
 *  checkpoint file.
 * 
 * @param *ckpt - this checkpoint instance
 * @param fd - original file descriptor
 * @param dup_fd - duplicate file descriptor that references the same file
 * as fds 
 * @return 0 upon succes
 */
static int tcmi_ckpt_openfile_write_dupfd(struct tcmi_ckpt *ckpt, int fd, int dup_fd)
{
	struct tcmi_ckpt_openfile_generic_hdr gen_hdr;
	struct tcmi_ckpt_openfile_dupfile_hdr dup_hdr;

	gen_hdr.restore_type = TCMI_CKPT_OPENFILE_DUPFILE;
	/* restore type identifier header */
	if (tcmi_ckpt_write(ckpt, &gen_hdr, sizeof(gen_hdr)) < 0) {
		mdbg(ERR3, "Error writing file generic header");
		goto exit0;
	}
	dup_hdr.fd = fd;
	dup_hdr.dup_fd = dup_fd;
	if (tcmi_ckpt_write(ckpt, &dup_hdr, sizeof(dup_hdr)) < 0) {
		mdbg(ERR3, "Error writing dupfd chunk");
		goto exit0;
	}
	mdbg(INFO3, "Written dupfd  fd=%d, dup_fd=%d", fd, dup_fd);
	return 0;
	/* error handling */
 exit0:
	return -EINVAL;
}

/**
 *  \<\<private\>\> Writes a new file chunk into the checkpoint file.
 * This includes:
 * - file descriptor
 * - current position in the file
 * - length of the file pathname
 * - filepathname
 *
 * @param *ckpt - this checkpoint instance
 * @param fd - file descriptor
 * @param *file - file object to be written into the checkpoint
 * @return 0 upon succes
 */
static int tcmi_ckpt_openfile_write_newfd(struct tcmi_ckpt *ckpt, int fd, struct file *file)
{
	struct tcmi_ckpt_openfile_generic_hdr gen_hdr;
	struct tcmi_ckpt_openfile_newfile_hdr new_hdr;
	/* page for the filepathname */
	unsigned long page;
	char *pathname;

	gen_hdr.restore_type = TCMI_CKPT_OPENFILE_NEWFILE;

	/* write the generic header first */
	if (tcmi_ckpt_write(ckpt, &gen_hdr, sizeof(gen_hdr)) < 0) {
		mdbg(ERR3, "Error writing file generic header");
		goto exit0;
	}

	new_hdr.fd = fd;
	new_hdr.pos = file->f_pos;
	new_hdr.flags = file->f_flags;
	new_hdr.mode = file->f_mode;
	new_hdr.type = file->f_dentry->d_inode->i_mode;
	new_hdr.pathname_size = 0;
	mdbg(INFO3, "File pos: %llu", (unsigned long long)file->f_pos);
	mdbg(INFO3, "Write: Fd: %ld, Pos %llu, File name size: %lu, Flags: %08lo, Mode: %08lo", (long)new_hdr.fd, (unsigned long long)new_hdr.pos, (unsigned long)new_hdr.pathname_size, (unsigned long)new_hdr.flags, (unsigned long)new_hdr.mode);

	/* resolve the path name. */
	if (!(page = __get_free_page(GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate page for file pathname!");
		goto exit0;
	}

	// By default files are simply reopened
	// WARNING: Very important! Buffer length is returned as a PAGE_SIZE - 1000. THe d_path fills in path from the end of the buffer
	// and if we return full buffer size, we are not able to append anything to the string, but we want to do that for the proxy files!
	if (IS_ERR(pathname = d_path(&file->f_path, (char*)page, PAGE_SIZE - 1000))) {
		mdbg(ERR3, "Can't resolve pathname for '%s'", file->f_dentry->d_name.name);
		goto exit1;
	}

	if ( !is_proxyfs_file(pathname) && ((S_ISCHR(new_hdr.type) || S_ISFIFO(new_hdr.type))) ){ // Special file, use proxyfs
		// Overtake only for shadow tasks and exclude dev-null
		if( current->tcmi.tcmi_task && current->tcmi.task_type == shadow && !is_dev_null(pathname) ){
			struct proxyfs_file_identifier* fileident = proxyfs_server_overtake_file(fd);

			if( fileident == NULL ){
				mdbg(ERR3, "Registering file in proxyfs_server failed!");
				goto exit1;
			}

			mdbg(INFO3, "BEFORE PROXY FS NAME: %s", pathname);
			create_proxyfs_name(current, pathname, fileident->file_ident);
			mdbg(INFO3, "AFTER PROXY FS NAME: %s", pathname);
		}
	}

	new_hdr.pathname_size = strlen(pathname) + 1;
	/* write the header and the pathname into the checkpoint */
	if (tcmi_ckpt_write(ckpt, &new_hdr, sizeof(new_hdr)) < 0) {
		mdbg(ERR3, "Error writing newfd chunk");
		goto exit1;
	}
	if (tcmi_ckpt_write(ckpt, pathname, new_hdr.pathname_size) < 0) {
		mdbg(ERR3, "Error writing file pathname chunk");
		goto exit1;
	}
	tcmi_ckpt_fdcache_add(ckpt, fd, file);
	mdbg(INFO4, "Written new file descriptor fd=%d, type=%0o '%s'", fd, new_hdr.type, pathname);	
	free_page(page);	
	return 0;

	/* error handling */
 exit1:
	free_page(page);
 exit0:
	return -EINVAL;
}

/**
 * \<\<private\>\> Reads a duplicate file descriptor chunk from the
 * checkpoint file.  The original file descriptor (dup_fd) is
 * duplicated into the file descriptor of this chunk (fd). Both then
 * refer to the same file.
 * 
 * @param *ckpt - checkpoint instance
 * @return 0 upon succes
 */
static int tcmi_ckpt_openfile_read_dupfd(struct tcmi_ckpt *ckpt)
{
	struct tcmi_ckpt_openfile_dupfile_hdr dup_hdr;

	int err = 0;

	if (tcmi_ckpt_read(ckpt, &dup_hdr, sizeof(dup_hdr)) < 0) {
		mdbg(ERR3, "Error reading dupfd chunk");
		goto exit0;
	}
	if ((err = sys_dup2(dup_hdr.dup_fd, dup_hdr.fd)) != dup_hdr.fd) {
		mdbg(ERR3, "Error duplicating file descriptor %d into %d, error=%d!", 
		     dup_hdr.dup_fd, dup_hdr.fd, err);
		goto exit0;
	}
	mdbg(INFO4, "Restored duplicate file descriptor fd=%d, dup_fd=%d", dup_hdr.fd, dup_hdr.dup_fd);
	return 0;

	/* error handling */
 exit0:
	return -EINVAL;
}

/**
 *  \<\<private\>\> Reads a new file chunk from the checkpoint file.
 * This includes:
 * - file descriptor
 * - current position in the file
 * - length of the file pathname
 * - filepathname
 * Opens the file with the same descriptor(might require using dup2 system call)
 * and seeks to the specified position. Since the stdin/stdout/stderr might
 * appear in the checkpoint image, we skip those for now - they are
 * of type character device.
 *
 * @param *ckpt - checkpoint instance
 */
static int tcmi_ckpt_openfile_read_newfd(struct tcmi_ckpt *ckpt)
{
	struct tcmi_ckpt_openfile_newfile_hdr new_hdr;
	mm_segment_t old_fs;
	int fd = -1;
	/* page for the filepathname */
	unsigned long page;


	old_fs = get_fs();
	set_fs(get_ds());

	if (tcmi_ckpt_read(ckpt, &new_hdr, sizeof(new_hdr)) < 0) {
		mdbg(ERR3, "Error reading newfd chunk");
		goto exit0;
	}
	mdbg(INFO3, "Read: Fd: %ld, Pos %llu, File name size: %lu, Flags: %08lo, Mode: %08lo", (long)new_hdr.fd, (unsigned long long)new_hdr.pos, (unsigned long)new_hdr.pathname_size, (unsigned long)new_hdr.flags, (unsigned long)new_hdr.mode);

	if ( new_hdr.pathname_size >= PAGE_SIZE )
		goto exit0;

	/* resolve the path name. */
	if (!(page = __get_free_page(GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate page for file pathname!");
		goto exit0;
	}
	if (tcmi_ckpt_read(ckpt, (char *)page, new_hdr.pathname_size) < 0) {
		mdbg(ERR3, "Error reading file pathname chunk");
		goto exit1;
	}

	/* reopen directory and regular files  */
	if ((S_ISDIR(new_hdr.type) || S_ISREG(new_hdr.type) || S_ISCHR(new_hdr.type) || S_ISFIFO(new_hdr.type))) {
		/* open */
		if ((fd = do_sys_open(AT_FDCWD, (char*)page, (int)new_hdr.flags, (int)new_hdr.mode)) < 0) {
			mdbg(ERR3, "Error opening file '%s', err=%d", (char*)page, fd);
			goto exit1;
		}
		/* setup the file position */
		if (new_hdr.pos != vfs_llseek(current->files->fdt->fd[fd], new_hdr.pos, 0)) {
			mdbg(ERR3, "Failed to set position %Lx in file '%s'", new_hdr.pos, (char*)page);
			goto exit1;
		}
	}

	/*
	if ( S_ISCHR(new_hdr.type) || S_ISFIFO(new_hdr.type) ){ // Forward operations via proxyfile
		mdbg(INFO3, "File '%s' fd = %d will be accesed trought proxyfile", (char*)page, new_hdr.fd);
		tcmi_ckpt_proxy_path((char*)page, (char*)page);
		mdbg(INFO3, "Opening file '%s' fd = %d", (char*)page, new_hdr.fd);
		if ((fd = sys_open((char*)page, new_hdr.flags, new_hdr.mode)) < 0) {
			mdbg(ERR3, "Error opening file '%s', err=%d", (char*)page, fd);
			goto exit1;
		}
	}
	*/

	/* set the original descriptor of the file */
	if (new_hdr.fd != fd) {
		mdbg(INFO3, "Duplicating file descriptor %d into %d!", fd, new_hdr.fd);		
		if (sys_dup2(fd, new_hdr.fd) != new_hdr.fd) {
			mdbg(ERR3, "Error duplicating file descriptor %d into %d!", fd, new_hdr.fd);
			goto exit1;
		}
		sys_close(fd);
	}

	mdbg(INFO4, "Restored new file descriptor fd=%d, pos=%Lx, type=%0o, "
	     "pathname_size=%d, pathname='%s'",
	     new_hdr.fd, new_hdr.pos, new_hdr.type, new_hdr.pathname_size, (char*)page);

	free_page(page);
	set_fs(old_fs);

	return 0;

	/* error handling */

 exit1:
	free_page(page);
 exit0:
	set_fs(old_fs);
	return -EINVAL;
}

/**
 * @}
 */

