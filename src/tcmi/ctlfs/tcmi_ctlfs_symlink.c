/**
 * @file tcmi_ctlfs_symlink.c - Definition of a class that represents
 * symbolic link in TCMI control file system.  This class extends
 * tcmi_ctlfs_entry.
 *
 * Date: 03/30/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ctlfs_symlink.c,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/dcache.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <asm/page.h>

#define TCMI_CTLFS_SYMLINK_PRIVATE
#include "tcmi_ctlfs_symlink.h"

#include <dbg.h>


/**
 * \<\<public\>\> Symbolic link is created as a regular ctlfs
 * entry. In addition, it retains an extra reference to the target
 * object. 
 *
 * @param *parent - pointer to the parent entry - has to be a directory
 * @param *target - target entry to which link should point to
 * @param namefmt - nameformat string (printf style)
 * @return new symlink instance or NULL
 */
struct tcmi_ctlfs_entry* tcmi_ctlfs_symlink_new(struct tcmi_ctlfs_entry *parent,
						struct tcmi_ctlfs_entry *target,
						const char namefmt[], ...)
{
	struct tcmi_ctlfs_symlink *symlink;
	va_list args;
	va_start(args, namefmt);
	mdbg(INFO4, "Creating new symlink");

	if (!(symlink = kmalloc(sizeof(struct tcmi_ctlfs_symlink), GFP_ATOMIC))) {
		mdbg(ERR3, "Failed to allocate memory for symlink");
		goto exit0;
	}
	if (tcmi_ctlfs_entry_init(TCMI_CTLFS_ENTRY(symlink), 
				  TCMI_CTLFS_ENTRY(parent), 
				  S_IFLNK|S_IRWXUGO,
				  &tcmi_ctlfs_symlink_ops,
				  &tcmi_ctlfs_symlink_inode_operations,
				  NULL, namefmt, args)) {
		mdbg(ERR3, "Failed to initialize the symlink");
		goto exit1;
	}
	symlink->target_entry = tcmi_ctlfs_entry_get(target);
	va_end(args);

	return TCMI_CTLFS_ENTRY(symlink);

	/* error handling */
 exit1:
	kfree(symlink);
 exit0:
	va_end(args);
	return NULL;
}

/** @addtogroup tcmi_ctlfs_symlink_class
 *
 * @{
 */

/** 
 * \<\<private\>\> This method is used to free the data associated with the
 * symlink. The target entry's reference is decremented as
 * the symlink doesn't need the target entry anymore.
 * 
 * @param *entry - points to the symlink that is to be freed
 * @return 0 upon success
 */
static void tcmi_ctlfs_symlink_release(struct tcmi_ctlfs_entry *entry)
{
	struct tcmi_ctlfs_symlink *symlink = TCMI_CTLFS_SYMLINK(entry);
	mdbg(INFO3, "Target entry '%s' released", tcmi_ctlfs_entry_name(symlink->target_entry));
	tcmi_ctlfs_entry_put(symlink->target_entry);
}



/**
 * \<\<private\>\> The path is built as follows:
 * - The path needs to be traversed to the root directory first '../', the number
 * of these elements is determined by the depth of the symlink in the tree.
 * - the minimum required buffer length is calculated based on:
 *        - symlink depth (see above)
 *        - pathname length of the target entry
 * - the buffer is then filled with ../'s
 * - the path of the target entry is then filled in
 * 
 * Should the target entry be invalid, the function terminates with an error
 * access to the target entry is serialized by the lock. 
 *
 * @param *self - pointer to this instance
 * @param *path - storage buffer for the path
 * @param length - maximum path length (storage buffer size)
 * @return 0 upon success
 */
static int tcmi_ctlfs_symlink_get_link(struct tcmi_ctlfs_symlink *self, 
				       char *path, int length)
{
	char * s;
	int depth, size;
	int err = -EINVAL;

	depth = tcmi_ctlfs_entry_depth(TCMI_CTLFS_ENTRY(self));
	/* we are interested in the depth of the parent entry, this is safe since
	 a minimum symlink depth is 1 (when it resides in the root directory) */
	depth--;
	size = tcmi_ctlfs_entry_path_length(self->target_entry) + depth * 3 - 1;
	if (size > length) {
		err = -ENAMETOOLONG;
		goto exit0;
	}
	mdbg(INFO4,"depth = %d, size = %d", depth, size);

	for (s = path; depth--; s += 3)
		strcpy(s,"../");

	if (self->target_entry)
		tcmi_ctlfs_entry_fill_path(self->target_entry, path, size);

	mdbg(INFO3, "path = '%s'\n", path);

	return 0;

	/* error handling */
 exit0:
/*   tcmi_ctlfs_symlink_unlock(self); */
	return err;
}

/**
 * \<\<private\>\> Provides symlink resolution. 
 * Supplies the pathname of the target entry that the symlink refers to.
 * This code is adapted from sysfs/symlink.c.
 * - allocates a whole page for the pathname (faster than kmalloc)
 * - extracts the symlink from the dentry
 * - asks the symlink for path resolution
 *
 * @param *dentry - points to the dentry of the symlink that is to be
 * resolved
 * @param *nd - where the pathname is to be stored
 * @return NULL upon success
 */
static void* tcmi_ctlfs_symlink_follow_link(struct dentry *dentry, struct nameidata *nd)
{
	int error = -ENOMEM;
	unsigned long page = get_zeroed_page(GFP_ATOMIC);
	struct tcmi_ctlfs_symlink *symlink;
	if (page) {
		symlink = TCMI_CTLFS_SYMLINK(TCMI_CTLFS_DENTRY_TO_ENTRY(dentry));
		error = tcmi_ctlfs_symlink_get_link(symlink, (char *) page, PAGE_SIZE); 
	}
	nd_set_link(nd, error ? ERR_PTR(error) : (char *)page);
	return NULL;
}

/**
 * \<\<private\>\> This VFS operation is called when the symlink pathname is not needed anymore by
 * VFS. It allows us to release the memory needed for the pathname.
 *
 * @param *dentry - points to the dentry of the symlink
 * @param *nd - where the pathname resides
 */
static void tcmi_ctlfs_symlink_put_link(struct dentry *dentry, struct nameidata *nd, void * vd)
{
	char *page = nd_get_link(nd);
	mdbg(INFO4, "Destroying pathname related data");
	if (!IS_ERR(page)) {
		mdbg(INFO4, "Freeing pathname buffer");
		free_page((unsigned long)page);
	}
}

/** 
 * A symlink requires special entry operations to free all
 * resources. 
 */
static struct tcmi_ctlfs_ops tcmi_ctlfs_symlink_ops = {
	.entry_release = tcmi_ctlfs_symlink_release
}; 

/** 
 * Custom VFS operations. Readlink operation is reused from VFS
 * (generic_readlink). The generic version in turn uses the
 * follow_symlink() to get the link pathname.
 */
static struct inode_operations tcmi_ctlfs_symlink_inode_operations = {
	.readlink = generic_readlink,
	.follow_link = tcmi_ctlfs_symlink_follow_link,
	.put_link = tcmi_ctlfs_symlink_put_link,
};

/**
 * @}
 */


EXPORT_SYMBOL_GPL(tcmi_ctlfs_symlink_new);
