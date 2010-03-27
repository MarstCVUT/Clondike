/**
 * @file tcmi_ctlfs_symlink.h - Declaration of a class that represents
 * symbolic link in TCMI control file system.  This class extends
 * tcmi_ctlfs_entry.
 *
 * Date: 03/30/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ctlfs_symlink.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#ifndef TCMI_CTLFS_SYMLINK_H
#define TCMI_CTLFS_SYMLINK_H



#include "tcmi_ctlfs_dir.h"


/** @defgroup tcmi_ctlfs_symlink_class tcmi_ctlfs_symlink class 
 * 
 * @ingroup tcmi_ctlfs_entry_class
 *
 * This class provides a robust solution for symbolic link creation.
 * A symbolic link can be made to any entry within the TCMI control
 * file system. The link instantiator provides only the target entry
 * object that the link should point to. The class itself then takes
 * care of creating the pathname to the target on fly as follow link
 * requests come. There are three VFS inode operations that the link
 * must provide:
 *
 * - follow_link - supplies the target object pathname to the VFS
 * - put_link - a notification form VFS that the target pathname is not
 * needed anymore after follow_link operation. 
 * - readlink - copies the target pathname to the buffer supplied by VFS
 *
 * This class implements only the first 2 operations
 * (tcmi_ctlfs_symlink_follow_link() and
 * tcmi_ctlfs_symlink_put_link()). The third one uses VFS generic_readlink()
 * that in turn invokes the custom tcmi_ctlfs_symlink_follow_link()).
 *
 * Each symbolic link instance retains a reference to the target entry.
 * This reference is released upon symlink destruction.
 * 
 * @{
 */

/**
 * A symbolic link compound structure, extends the parent entry class.
 * Contains a reference to the target entry.
 */
struct tcmi_ctlfs_symlink {
	/** parent class instance. */
	struct tcmi_ctlfs_entry super;

	/** target entry, that the symbolic link is pointing to. */
	struct tcmi_ctlfs_entry *target_entry;

};


/** \<\<public\>\> Creates a new symbolic link instance. */
extern struct tcmi_ctlfs_entry* tcmi_ctlfs_symlink_new(struct tcmi_ctlfs_entry *parent,
						       struct tcmi_ctlfs_entry *target,
						       const char namefmt[], ...);



/** Casts an entry to a symlink */
#define TCMI_CTLFS_SYMLINK(e) ((struct tcmi_ctlfs_symlink *)e)


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CTLFS_SYMLINK_PRIVATE

/** A forward declaration, frees all file resources. */
static void tcmi_ctlfs_symlink_release(struct tcmi_ctlfs_entry *entry);


/** Builds the path that points from a symlink entry to the target entry. */
static int tcmi_ctlfs_symlink_get_link(struct tcmi_ctlfs_symlink *self, 
				       char *path, int length);

/** VFS operation to follow the symlink. */
static void* tcmi_ctlfs_symlink_follow_link(struct dentry *dentry, struct nameidata *nd);

/** VFS operation to release symlink's pathname. */
static void tcmi_ctlfs_symlink_put_link(struct dentry *dentry, struct nameidata *nd, void * vd);

/** A forward declaration, special operations for tcmi_ctlfs entries. */
static struct tcmi_ctlfs_ops tcmi_ctlfs_symlink_ops;

/** A forward declaration, inode operations for VFS. */
static struct inode_operations tcmi_ctlfs_symlink_inode_operations;

#endif /* TCMI_CTLFS_SYMLINK_PRIVATE */

/**
 * @}
 */

#endif /* TCMI_CTLFS_SYMLINK_H */

