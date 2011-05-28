/**
 * @file tcmi_ckpt.c - a helper class that provides functionality to
 *                     create a process checkpoint
 *                      
 * 
 *
 *
 * Date: 04/30/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ckpt.c,v 1.7 2007/11/05 19:38:28 stavam2 Exp $
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

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/fdtable.h>


#define TCMI_CKPT_PRIVATE
#include "tcmi_ckpt.h"

#include "tcmi_ckpt_openfile.h"
#include "tcmi_ckpt_vm_area.h"

#include <arch/arch_ids.h>
#include <arch/current/regs.h>
#include <arch/current/vma.h>

#include <linux/personality.h>


/** 
 * \<\<public\>\> Checkpoint constructor. 
 * - allocates a new instance
 * - resets the byte counter
 * - reserves an extra reference for the file
 * - reset position in the file to the beginning
 * 
 *
 * @param *file - points to the file where the checkpoint is to be
 * written/read
 * @return tcmi_ckpt instance or NULL
 */
struct tcmi_ckpt* tcmi_ckpt_new(struct file *file)
{
	struct tcmi_ckpt *ckpt;
	loff_t seek_res;

	if (!(ckpt = TCMI_CKPT(kmalloc(sizeof(struct tcmi_ckpt), 
				       GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate memory for TCMI ckpt");
		goto exit0;
	}
	get_file(file);
	ckpt->file = file;
	ckpt->fdcache = NULL;
	atomic_set(&ckpt->ref_count, 1);
	mdbg(INFO2, "Created new checkpoint %p", ckpt);
	/* reset the file position */
	seek_res = tcmi_ckpt_seek(ckpt, 0, 0);

	if ( seek_res < 0 ) {
		kfree(ckpt);
		return ERR_PTR(seek_res);
	}

	return ckpt;

	/* error handling */
 exit0:
	return NULL;

}

/** 
 * \<\<public\>\> Writes a checkpoint header.
 * The checkpoint header consists of:
 * - a magic number 
 * - total VM area count
 * - total file count
 *
 * @param *self - this checkpoint instance
 * @param is_npm - Is this a non-preemptive checkpoint (otherwise preemptive is assumed)
 * @return 0 upon success
 */
int tcmi_ckpt_write_hdr(struct tcmi_ckpt *self, int is_npm)
{
	self->hdr.checkpoint_arch = ARCH_CURRENT;
	self->hdr.is_32bit_application = check_is_application_32bit();
	self->hdr.magic = TCMI_CKPT_MAGIC;
	self->hdr.is_npm = is_npm;
	if ((self->hdr.map_count = tcmi_ckpt_map_count(self)) < 0) {
		mdbg(ERR3, "Failed counting mmap's");
		goto exit0;
	}
	if ((self->hdr.file_count = tcmi_ckpt_file_count(self)) < 0) {
		mdbg(ERR3, "Failed counting open files");
		goto exit0;
	}
	
	if (tcmi_ckpt_fdcache_init(self) < 0) {
		mdbg(ERR3, "Error initializing file descriptor cache!!");
		goto exit0;
	}
	// No need to lock, exclusive access
	strncpy(self->hdr.comm, current->comm, sizeof(current->comm));

	mdbg(INFO3, "Writing header of size: %lu Address: %p First 4 bytes: %8x", (unsigned long)sizeof(self->hdr), &self->hdr, ((uint32_t*)(&self->hdr))[0]);
	
	return tcmi_ckpt_write(self, &self->hdr, sizeof(self->hdr));
	
	/* error handling */
 exit0:
	return -EINVAL;
}

/** 
 * \<\<public\>\> Reads the checkpoint header.  
 * The magic number has to match TCMI_CKPT_MAGIC.
 *
 * @param *self - this checkpoint instance
 * @return 0 upon success
 */
int tcmi_ckpt_read_hdr(struct tcmi_ckpt *self)
{
	int err;

	if ((err = tcmi_ckpt_read(self, &self->hdr, sizeof(self->hdr))) < 0) {
		mdbg(ERR3, "Can't read checkpoint header %d", err);
		goto exit0;
	}
	if (self->hdr.magic != TCMI_CKPT_MAGIC) {
		mdbg(ERR3, "Invalid checkpoint magic: %08x, expected: %08x",
		     self->hdr.magic, TCMI_CKPT_MAGIC);
		goto exit0;
	}
	mdbg(INFO4, "Read ckpt header - magic: %08x, map_count: %d, files: %d (Header size: %lu)", 
	     self->hdr.magic, self->hdr.map_count, self->hdr.file_count, (unsigned long)sizeof(self->hdr));

	if (tcmi_ckpt_fdcache_init(self) < 0) {
		mdbg(ERR3, "Error initializing file descriptor cache!!");
		goto exit0;
	}
	tcmi_ckpt_file_count(self);
	
	strncpy(current->comm, self->hdr.comm, sizeof(current->comm));

	/* TODO: To some arch file ? */
	#if defined(__x86_64__) 
	if ( self->hdr.is_32bit_application )
		set_thread_flag(TIF_IA32); /* This flag must be set for 32bit applications running on 64 bit kernel */		

	if ( self->hdr.checkpoint_arch == ARCH_I386 )
		set_personality(PER_LINUX32_3GB); /* Address space must be limited by 3GB, so that we are possibly able to migrate back */
	#endif

	mdbg(INFO3, "Read header. Architecture of checkpoint %d Is 32 bit app: %d", self->hdr.checkpoint_arch, self->hdr.is_32bit_application);
	return 0;

	/* error handling */
 exit0:
	return -EINVAL;
}

/** 
 * \<\<public\>\> Writes open files into the checkpoint.  Iterates through all open
 * files of the current process and writes each file details into the
 * checkpoint. Since some files might be just a duplicate file
 * descriptor, we always check with the file descriptor cache.
 * For duplicate file descriptors only simplified record is needed.
 * 
 * @param *self - this checkpoint instance
 * @return 0 upon success
 */
int tcmi_ckpt_write_files(struct tcmi_ckpt *self)
{
	int fd = 0;
	int err = 0;
	struct file *file;

	/* Check each open file */
	tcmi_ckpt_foreach_openfile(file, fd) {
		mdbg(INFO4, "Processing '%s' fd=%d, type=%0o flags %08o, mode %08o, %p", 
		     file->f_dentry->d_name.name, fd, file->f_dentry->d_inode->i_mode & S_IFMT, 
		     file->f_flags, file->f_mode, file);
		if ( (err = tcmi_ckpt_openfile_write(self, fd, file)) < 0) {
			mdbg(INFO3, "Error writing file into checkpoint. Error %d", err);
			goto exit0;
		}
	}
	return 0;

	/* error handling */
 exit0:
	return err;
}

/** 
 * \<\<public\>\> Reads open files from the checkpoint. 
 *
 * @param *self - this checkpoint instance
 * @return 0 upon success
 */
int tcmi_ckpt_read_files(struct tcmi_ckpt *self)
{
	int err = 0;
	int i = 0;
	while ((i < self->hdr.file_count) && !err) {
		err = tcmi_ckpt_openfile_read(self);
		i++;
	}
	tcmi_ckpt_file_count(self);
	return err;
}

/** 
 * \<\<public\>\> Writes all process memory regions into the checkpoint. 
 *
 * @param *self - this checkpoint instance
 * @param heavy - if set a full checkpoint of all memory areas
 * is made(see tcmi_ckptcom.c::tcmi_ckptcom_checkpoint()).
 * @return 0 upon success
 */
int tcmi_ckpt_write_vmas(struct tcmi_ckpt *self, int heavy)
{
	struct vm_area_struct *vma;
	int err = 0;
	tcmi_ckpt_vm_area_t type;
	/* select the checkpoint type*/
	type = (heavy ? TCMI_CKPT_VM_AREA_HEAVY : TCMI_CKPT_VM_AREA_LIGHT);

	tcmi_ckpt_foreach_vma(vma) {
		if ( vma_ignore(self, vma) )
			continue;

		if ((err = tcmi_ckpt_vm_area_write(self, vma, type)) < 0) {
		mdbg(ERR3, "Can't write vm area start:%08lx, end: %08lx, flags: %08lx, pageprot %08lx", 
		     vma->vm_start, vma->vm_end, vma->vm_flags, vma->vm_page_prot.pgprot);
		goto exit0;
		}
	}

	return 0;

	/* error handling */
 exit0:
	return -EINVAL;
}

/** 
 * \<\<public\>\> Reads process pages from the checkpoint.
 *
 * @param *self - this checkpoint instance
 * @return 0 upon success
 */
int tcmi_ckpt_read_vmas(struct tcmi_ckpt *self)
{
	int err = 0;
	int i = 0;
	while ((i < self->hdr.map_count) && !err) {
		err = tcmi_ckpt_vm_area_read(self);
		i++;
	}

	tcmi_ckpt_map_count(self);
	
	return err;
}

/** @addtogroup tcmi_ckpt_class
 *
 * @{
 */

/**
 * \<\<private\>\> Calculates the number of memory regions of the
 * current process.  We can't simply return current->map_count. We
 * need to check for special regions that would prevent creating a
 * valid checkpoint
 * 
 * For now, we have to verify that the memory region is not:
 * - VM_IO - I/O mapped devices 
 * - VM_SHARED - shared memory - this will be included later on,
 * when we support checkpointing and migration of LWP's
 * - VM_RESERVED - special mappings  
 * 
 * @param *self - this checkpoint instance
 * @return number of memory regions, or error (< 0)
 */
static int tcmi_ckpt_map_count(struct tcmi_ckpt *self)
{
	struct vm_area_struct *vma;
	int count = 0;
	int err = 0;

	tcmi_ckpt_foreach_vma(vma) {
		if ( vma_ignore(self, vma) )
			continue;

		count++;
		/* Do not dump I/O mapped devices, shared memory, or special mappings */
		if (vma->vm_flags & (VM_IO | VM_SHARED | VM_RESERVED))
			err = -EINVAL;
		mdbg(INFO4, "Scanning area start:%08lx, end: %08lx, flags: %08lx, pageprot %08lx", 
		     vma->vm_start, vma->vm_end, vma->vm_flags, vma->vm_page_prot.pgprot);
	}
	mdbg(INFO4, "Found %d areas, mm indicates: %d", count, current->mm->map_count);
	return (err ? err : count);
}

/**
 * \<\<private\>\> Calculates the number of open files of the current process.
 * For each file check if the file is supported if so,
 * adjust the counter.
 *
 * An error is indicated if any of the files is:
 * - socket
 * - pipe
 * - symlink
 * - special file
 * 
 * @param *self - this checkpoint instance
 * @return number of open files, or error (< 0)
 */
static int tcmi_ckpt_file_count(struct tcmi_ckpt *self)
{
	/* file descriptor count */
	int fd;
	int count = 0;
	int err = 0;
	struct file *file;
	struct inode *inode;

	/* Check each open file */
	tcmi_ckpt_foreach_openfile(file, fd) {
		inode = file->f_dentry->d_inode;
		if (!tcmi_ckpt_openfile_supported(inode)) {
			mdbg(ERR3, "Unsupported file mode detected %o [FD: %d]", inode->i_mode, fd);
			err = -EINVAL;
			goto exit0;
		}
		count++;
		/*mdbg(INFO4, "Scanned file fd=%d, type=%0o '%s'", fd, file->f_flags, 
		  file->f_dentry->d_name.name); */
	}

	mdbg(INFO4, "Found %d files, err=%d", count, err);
	return count;

	/* error handling */
 exit0:
	return -EINVAL;
}

/**
 * @}
 */
