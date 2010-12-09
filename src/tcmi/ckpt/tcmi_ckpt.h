/**
 * @file tcmi_ckpt.h - a helper class that provides functionality to
 *                     create a process checkpoint
 *                      
 * 
 *
 *
 * Date: 04/30/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ckpt.h,v 1.7 2007/09/03 20:57:44 stavam2 Exp $
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
#ifndef _TCMI_CKPT_H
#define _TCMI_CKPT_H

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <asm/uaccess.h>

/** enables access to system calls from kernel */
#define __KERNEL_SYSCALLS__
#include <linux/syscalls.h> 
#include <linux/unistd.h> 

#include "tcmi_fdcache.h"

#include <arch/arch_ids.h> 
#include <dbg.h>

/** @defgroup tcmi_ckpt_class tcmi_ckpt class 
 *
 * @ingroup tcmi_ckptcom_class
 *
 * This is a helper class used by the checkpointing component that
 * provides functionality to:
 * - write current process into a checkpoint file
 * - restore a current process from a checkpoint file
 *
 * It doesn't do the whole job at once, it relies on its controlling
 * class (\link tcmi_ckptcom_class checkpointing component \endlink).
 * The checkpointing component instructs the checkpoint instance what
 * to do in each step (this applies to creating the checkpoint as
 * well as restoring the process execution).
 *
 * Implementation is Linux kernel specific.
 * 
 * @{
 */
/** Describes the checkpoint header */
struct tcmi_ckpt_hdr {
	/** magic number identifying the checkpoint file. */
	int32_t magic;
	/** total count of virtual memory areas. */
	int32_t map_count;
	/** total count of open files. */
	int32_t file_count;
	/** Architecture (hw) of the checkpointed data */
	enum arch_ids checkpoint_arch;
	/** Flag, indicating whether the checkpointed application was 32 bit application (otherwise 64 bit is assumed) */
	int8_t is_32bit_application;
	/** 1, if this is checkpoint created as a result of non-preemptive migration request (0 otherwise) */
	int8_t is_npm;
	/** Process executable name */
	char comm[TASK_COMM_LEN];
} __attribute__((__packed__));

/** Compound structure that gathers necessary process information. */
struct tcmi_ckpt {
	/** Actual file used create/restore the process checkpoint */
	struct file *file;

	/** File descriptor cache - used when indentifying descriptors
	 * referencing the same file. */
	struct tcmi_fdcache *fdcache;

	/** Checkpoint header */
	struct tcmi_ckpt_hdr hdr;

	/** Instance reference counter. */
	atomic_t ref_count;
};

/** Casts to the checkpoint  instance. */
#define TCMI_CKPT(ckpt) ((struct tcmi_ckpt*)ckpt)

/** a magic number for the header  */
#define TCMI_CKPT_MAGIC 0xdeadbeef

/** \<\<public\>\> Checkpoint constructor. */
extern struct tcmi_ckpt* tcmi_ckpt_new(struct file *file);

/** \<\<public\>\> Writes a checkpoint header. */
extern int tcmi_ckpt_write_hdr(struct tcmi_ckpt *self, int is_npm);
/** \<\<public\>\> Reads a checkpoint header. */
extern int tcmi_ckpt_read_hdr(struct tcmi_ckpt *self);

/** \<\<public\>\> Writes open files into the checkpoint. */
extern int tcmi_ckpt_write_files(struct tcmi_ckpt *self);
/** \<\<public\>\> Reads open files from the checkpoint. */
extern int tcmi_ckpt_read_files(struct tcmi_ckpt *self);

/** \<\<public\>\> Writes process memory regions into the checkpoint. */
extern int tcmi_ckpt_write_vmas(struct tcmi_ckpt *self, int heavy);
/** \<\<public\>\> Reads process memory regions from the checkpoint. */
extern int tcmi_ckpt_read_vmas(struct tcmi_ckpt *self);

/** \<\<public\>\> Writes process CPU registers into the checkpoint. */
extern int tcmi_ckpt_write_regs(struct tcmi_ckpt *self, struct pt_regs *regs);
/** \<\<public\>\> Reads process CPU registers from the checkpoint. */
extern int tcmi_ckpt_read_regs(struct tcmi_ckpt *self, struct pt_regs *regs);

/** \<\<public\>\> Writes signals related stuff into the checkpoint. */
//extern int tcmi_ckpt_sig_write(struct tcmi_ckpt *self);
/** \<\<public\>\> Reads signals related stuff from the checkpoint. */
//extern int tcmi_ckpt_sig_read(struct tcmi_ckpt *self);

/**
 * \<\<public\>\> Performs a quick check if the specified buffer
 * contains a valid magic number of the checkpoint file.
 *
 * @param *buf - buffer containing the magic number
 * @return true when valid.
 */
static inline int tcmi_ckpt_check_magic(void *buf)
{
	return ((struct tcmi_ckpt_hdr*)buf)->magic == TCMI_CKPT_MAGIC;
}

/** 
 * \<\<public\>\> Instance accessor, increments the reference counter.
 *
 * @param *self - pointer to this checkpoint instance
 * @return tcmi_ckpt instance
 */
static inline struct tcmi_ckpt* tcmi_ckpt_get(struct tcmi_ckpt *self)
{
	if (self) {
		atomic_inc(&self->ref_count);
	}
	return self;
}


/** 
 * \<\<public\>\> Releases the instance.
 *
 * @param *self - pointer to this instance
 */
static inline void tcmi_ckpt_put(struct tcmi_ckpt *self)
{
	if (self && atomic_dec_and_test(&self->ref_count)) {
		mdbg(INFO4, "Destroying TCMI ckpt, %p", self);
		fput(self->file);
		tcmi_fdcache_put(self->fdcache);
		kfree(self);
	}
}



/** Describes the signature of the VFS method (typically vfs_read or vfs_write */
typedef ssize_t vfs_method_t(struct file*, char __user*, size_t, loff_t*);

/**
 * \<\<private\>\> Executes a specified VFS method on the checkpoint
 * file.
 *
 * @param *self - this checkpoint instance
 * @param *data - buffer for the data
 * @param count - number of bytes to be processed
 * @param *vfs_method - VFS method to be executed on the checkpoint
 * and data
 * @return 0 upon successful vfs method execution or an error (< 0)
 */
static inline int tcmi_ckpt_read_write(struct tcmi_ckpt *self, 
				       void *data, int count,
				       vfs_method_t *vfs_method)
{
	mm_segment_t old_fs;
	int result = 0;

	old_fs = get_fs();
	set_fs(get_ds());
	/* The cast to a user pointer is valid due to the set_fs() */
	result = vfs_method(self->file, (void __user *)data, count, &self->file->f_pos);
	set_fs(old_fs);

	return (result == count ? 0 : -EINVAL);
}

/**
 * \<\<public\>\> Writes a specified number of bytes into a
 * checkpoint file.  
 *
 * @param *self - this checkpoint instance
 * @param *data - data to be written
 * @param count - number of bytes to be written
 * @return 0 upon successful write of count bytes
 */
static inline int tcmi_ckpt_write(struct tcmi_ckpt *self, void *data, int count)
{
	return tcmi_ckpt_read_write(self, data, count, (vfs_method_t*)vfs_write);
}

/**
 * \<\<public\>\> Reads a specified number of bytes from a checkpoint
 * file. 
 *
 * @param *self - this checkpoint instance
 * @param *data - buffer for the data
 * @param count - number of bytes to be read
 * @return 0 upon successful write of count bytes
 */
static inline int tcmi_ckpt_read(struct tcmi_ckpt *self, void *data, int count)
{
	mdbg(INFO3, "CKPT read to %p, count %d bytes", data, count);
	return tcmi_ckpt_read_write(self, data, count, (vfs_method_t*)vfs_read);
}


/**
 * \<\<public\>\> Seeks in the checkpoint file based on specified
 * offset.
 * 
 * @param *self - this checkpoint instance
 * @param *offset - relative offset
 * @param origin - 0 - offset is absolute from file start
 *                 1 - offset is relative from current file position
 *                 2 - offset is relative from end of file
 * @return new file position or error (< 0)
 */
static inline loff_t tcmi_ckpt_seek(struct tcmi_ckpt *self, loff_t offset, int origin)
{
	return vfs_llseek(self->file, offset, origin);
}


/**
 * \<\<public\>\> Helper method that performs a memory mapping on the
 * checkpoint file at its current offset. After that the position is
 * advanced by len bytes.
 * 
 * @param *self - this checkpoint instance
 * @param addr - starting address where the 
 * @param len - length of the region
 * @param prot - access rights of the pages included in the memory
 * region
 * @param flags - memory region flags (see mm.h)
 * @return new file position or error (< 0)
 */
static inline unsigned long tcmi_ckpt_do_mmap(struct tcmi_ckpt *self, unsigned long addr,
					      unsigned long len, unsigned long prot,
					      unsigned long flags)
{

	mdbg(INFO4, "Mapping VMA start:%08lx, len %08lx, prot: %08lx, flags: %08lx, pgoff: %08Lx", 
	     addr, len, prot, flags, self->file->f_pos >> PAGE_SHIFT);

	down_write(&current->mm->mmap_sem);
	addr = do_mmap(self->file, addr, len, prot, flags, self->file->f_pos);
	up_write(&current->mm->mmap_sem);

	tcmi_ckpt_seek(self, len, 1);

	mdbg(INFO4, "Memory mapping at %08lx, new off %08Lx", addr, self->file->f_pos);
	return addr;
}

/**
 * \<\<public\>\> Aligns the position in the checkpoint file to page
 * boundary.  This is needed when reading or writing pages.
 * 
 * @param *self - this checkpoint instance
 * @return new file position or error (< 0)
 */
static inline loff_t tcmi_ckpt_page_align(struct tcmi_ckpt *self)
{
	return vfs_llseek(self->file, (self->file->f_pos + PAGE_SIZE - 1) & PAGE_MASK, 0);
}


/**
 * \<\<public\>\> Adds a file descriptor - file object pair into the
 * fd cache. This information will be used for later lookup of
 * duplicate file descriptors.
 * 
 * @param *self - this checkpoint instance
 * @param fd - file descriptor of the file
 * @param *file - file object - used for duplicate file descriptor lookup
 * @return 0 upon success
 */
static inline int tcmi_ckpt_fdcache_add(struct tcmi_ckpt *self, int fd, struct file *file)
{
	return tcmi_fdcache_add(self->fdcache, fd, file);
}

/**
 * \<\<public\>\> Searches the file descriptor cache for a file.
 *
 * @param *self - this checkpoint instance
 * @param *file - file object to be looked up
 * @return file descriptor of the file or error (< 0)
 */
static inline int tcmi_ckpt_fdcache_lookup(struct tcmi_ckpt *self, struct file *file)
{
	return tcmi_fdcache_lookup(self->fdcache, file);
}

/** 
 * \<\<public\>\> Iterates through all open files 
 *
 * @param file - current file
 * @param fd - descriptor of the file
 */
#define tcmi_ckpt_foreach_openfile(file, fd)						\
	for (fd = 0, file = current->files->fdt->fd[fd]; 					\
	     fd < current->files->fdt->max_fds; fd++, file = current->files->fdt->fd[fd])		\
		if (FD_ISSET(fd, current->files->fdt->open_fds)) 

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CKPT_PRIVATE

/** Calculates the number of memory regions of the current process. */
static int tcmi_ckpt_map_count(struct tcmi_ckpt *self);

/** Calculates the number of open files of the current process. */
static int tcmi_ckpt_file_count(struct tcmi_ckpt *self);

/**
 * Initializes filedescriptor cache for a given file count
 * 
 * @param *self - this checkpoint instance
 * @param count - maximum number of files that it will retain.
 */
static inline int tcmi_ckpt_fdcache_init(struct tcmi_ckpt *self)
{
	self->fdcache = tcmi_fdcache_new(self->hdr.file_count);
	return self->fdcache ? 0 : -ENOMEM;
}



/** Iterates through all VM areas */
#define tcmi_ckpt_foreach_vma(vma)				\
for (vma = current->mm->mmap; vma != NULL; vma = vma->vm_next) 



#endif /* TCMI_CKPT_PRIVATE */



/**
 * @}
 */

#endif /* _TCMI_CKPT_H */

