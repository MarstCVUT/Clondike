/**
 * @file tcmi_ctlfs_dir.c - Implementation of a class that represents directory
 *                          class in tcmifs. This class extends tcmi_ctlfs_entry
 * 
 *
 *
 * Date: 03/26/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ctlfs_dir.c,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#include <linux/fs.h>
#include <linux/dcache.h>

#include "tcmi_ctlfs_dir.h"

#include <dbg.h>

/** 
 * \<\<public\>\> Creates a new directory entry in the tcmi_ctlfs
 * tree.  Handles variable number of arguments and delegates work to
 * tcmi_ctlfs_dir_vnew()
 *
 * @param *parent - pointer to the parent entry - has to be a directory
 * @param *mode - access rights for the inode
 * @param namefmt - nameformat string (printf style)
 * @return pointer to the new directory or NULL
 */
struct tcmi_ctlfs_entry* tcmi_ctlfs_dir_new(struct tcmi_ctlfs_entry *parent,
					    mode_t mode, const char namefmt[], ...)
{
	struct tcmi_ctlfs_entry *dir;
	va_list args;
	va_start(args, namefmt);
	dir = tcmi_ctlfs_dir_vnew(parent, mode, namefmt, args);
	va_end(args);
	return dir;
}

/** 
 * \<\<public\>\> Creates a new directory entry in the tcmi_ctlfs tree.
 * The instance is:
 * - created
 * - initialized (delegated to the parent class)
 * - link counts of parent and new directory need to be adjusted
 *
 * @param *parent - pointer to the parent entry - has to be a directory
 * @param *mode - access rights for the inode
 * @param namefmt - nameformat string (printf style)
 * @param args - arguments to the format string
 * @return pointer to the new directory or NULL
 */
struct tcmi_ctlfs_entry* tcmi_ctlfs_dir_vnew(struct tcmi_ctlfs_entry *parent,
					     mode_t mode, const char namefmt[], 
					     va_list args)
{
	struct tcmi_ctlfs_dir *dir;

	mdbg(INFO4, "Creating new directory");
	if (!(dir = kmalloc(sizeof(struct tcmi_ctlfs_dir), GFP_ATOMIC))) {
		mdbg(ERR3, "Failed to allocate memory for directory");
		goto exit0;
	}
        /* directory file operations are taken from fs/libfs.c */
	if (tcmi_ctlfs_entry_init(TCMI_CTLFS_ENTRY(dir), 
				  TCMI_CTLFS_ENTRY(parent), 
				  S_IFDIR | mode, 
				  NULL, 
				  &simple_dir_inode_operations, 
				  &simple_dir_operations, namefmt, args)) {
		mdbg(ERR3, "Failed to create the directory"); 
		goto exit1;
	}
	/* success, append the directory to its parent (account for '..') and 
	   increment the link count for '.' */
	tcmi_ctlfs_entry_inc_links(TCMI_CTLFS_ENTRY(parent));
	tcmi_ctlfs_entry_inc_links(TCMI_CTLFS_ENTRY(dir));
	return TCMI_CTLFS_ENTRY(dir);

	/* error handling */
 exit1:
	kfree(dir);
 exit0:
	return NULL;
}

/** 
 * \<\<public\>\> Root directory is instantiated by delegating all
 * work to the parent class.
 *
 * @param *sb - super block of the file system
 * @return pointer to the new root directory or NULL
 */
struct tcmi_ctlfs_entry* tcmi_ctlfs_rootdir_new(struct super_block *sb)
{
	struct tcmi_ctlfs_dir *root;
		
	mdbg(INFO4, "Creating root directory");
	if (!(root = kmalloc(sizeof(struct tcmi_ctlfs_dir), GFP_ATOMIC))) {
		mdbg(ERR3, "Failed to allocate memory for root directory");
		goto exit0;
	}
	/* directory file operations are taken from fs/libfs.c */
	if (tcmi_ctlfs_rootentry_init(TCMI_CTLFS_ENTRY(root), sb, 
				      S_IFDIR | 0755, 
				      NULL, 
				      &simple_dir_inode_operations, 
				      &simple_dir_operations)) {
		mdbg(ERR3, "Failed to create root directory");
		goto exit1;
	}

	/* account for '.' */
	tcmi_ctlfs_entry_inc_links(TCMI_CTLFS_ENTRY(root));
	return TCMI_CTLFS_ENTRY(root);

	/* error handling */
 exit1:
	kfree(root);
 exit0:
	return NULL;
}
	
EXPORT_SYMBOL_GPL(tcmi_ctlfs_dir_new);
EXPORT_SYMBOL_GPL(tcmi_ctlfs_dir_vnew);
