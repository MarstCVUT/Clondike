/**
 * @file tcmi_fdcache.h - a helper class that allows caching of used file descriptors.
 *                      and lookup of duplicate file descriptors.
 * 
 *
 *
 * Date: 05/02/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_fdcache.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#ifndef _TCMI_FDCACHE_H
#define _TCMI_FDCACHE_H

#include <asm/atomic.h>

#include <dbg.h>

/** @defgroup tcmi_fdcache_class tcmi_fdcache class 
 *
 * @ingroup tcmi_ckpt_class
 *
 * This is a helper class used by the checkpoint to keep track of
 * already open file descriptors. It allows easy lookup of a duplicate
 * file descriptor. This is needed when checkpointing open files of a
 * process.
 *
 * There is no need for locking nor reference counting as fdcache is a
 * temporary object (as well as \link tcmi_ckpt_class a checkpoint
 * object \endlink) accessed from a single thread of execution. The
 * reference counter is just to keep it consistent with current
 * software design.
 *
 * @{
 */

/** Describes an fd cache entry. */
struct tcmi_fdcache_entry {
	/** file descriptor. */
	int fd;
	/** file that references. */
	struct file *file;
};

/** Compound structure contains all fd cache entries. */
struct tcmi_fdcache {
	/** File descriptor cache - used when indentifying descriptors
	 * referencing the same file. */
	struct tcmi_fdcache_entry *entries;
	/** current files in the cache. */
	int used;
	/** actual cache size. */
	int size;
	/** Instance reference counter. */
	atomic_t ref_count;
};

/**
 * \<\<public\>\> Creates a file descriptor cache of the specified
 * size. This requires:
 * - creating new instance
 * - allocating a requested number of entries
 *
 * @param size - maximum number of entries - should match the current
 * number of open files of a process - this handles the worst case
 * when there are none duplicate descriptors but one.
 * @return fdcache instance or NULL
 */
static inline struct tcmi_fdcache* tcmi_fdcache_new(int size)
{
	struct tcmi_fdcache *fdcache;
	if (!(fdcache = kmalloc(sizeof(struct tcmi_fdcache), 
				GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate memory for TCMI fdcache");
		goto exit0;
	}
	if (!(fdcache->entries = kmalloc(sizeof(struct tcmi_fdcache_entry) * size,
					 GFP_KERNEL))) {
		mdbg(ERR3, "Can't get memory for TCMI fdcache %d entries", size);
		goto exit1;
	}
	atomic_set(&fdcache->ref_count, 1);
	fdcache->size = size;
	/* no used entries yet */
	fdcache->used = 0;

	return fdcache;
	/* error handling */
 exit1:
	kfree(fdcache);
 exit0:
	return NULL;
}

/** 
 * \<\<public\>\> Instance accessor, increments the reference counter.
 *
 * @param *self - pointer to this fdcache instance
 * @return fd cache instance
 */
static inline struct tcmi_fdcache* tcmi_fdcache_get(struct tcmi_fdcache *self)
{
	if (self) {
		atomic_inc(&self->ref_count);
	}
	return self;
}

/** 
 * \<\<public\>\> Releases the instance.
 *
 * @param *self - pointer to this fd cache instance
 */
static inline void tcmi_fdcache_put(struct tcmi_fdcache *self)
{
	if (self && atomic_dec_and_test(&self->ref_count)) {
		mdbg(INFO4, "Destroying TCMI fdcache, %p", self);
		kfree(self->entries);
		kfree(self);
	}
}

/**
 * \<\<public\>\> Adds a file descriptor - file object pair into the
 * fd cache.
 * 
 * @param *self - this fdcache instance
 * @param fd - file descriptor of the file
 * @param *file - file object - used for duplicate file descriptor lookup
 * @return -ENOMEM if the cache is full
 */
static inline int tcmi_fdcache_add(struct tcmi_fdcache *self, int fd, struct file *file)
{
	struct tcmi_fdcache_entry *entry;
	if (!self || (self->used == self->size)) {
		mdbg(ERR3, "TCMI fdcache full or not properly instantiated!");
		goto exit0;
	}
	entry = &self->entries[self->used++];
	entry->fd = fd;
	entry->file = file;
	return 0;

	/* error handling */
 exit0:
	return -ENOMEM;
}

/**
 * \<\<public\>\> Searches the file descriptor cache for a file.
 *
 * @param *self - this fdcache instance
 * @param *file - file object to be looked up
 * @return file descriptor of the file or error (< 0)
 */
static inline int tcmi_fdcache_lookup(struct tcmi_fdcache *self, struct file *file)
{
	int i;
	int fd = -EINVAL;
	/* safes dereferences */
	int used = self->used;
	struct tcmi_fdcache_entry *entry = self->entries;
	for (i = 0; i < used; i++, entry++) {
		if (entry->file == file) {
			fd = entry->fd;
			mdbg(INFO4, "Found descriptor %d in the fdcache", fd);
			break;
		}
	}

	return fd;
}

#endif /* TCMI_FDCACHE_PRIVATE */


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_FDCACHE_PRIVATE

/**
 * @}
 */

#endif /* _TCMI_FDCACHE_H */
