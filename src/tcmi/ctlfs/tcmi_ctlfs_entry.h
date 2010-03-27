/**
 * @file tcmi_ctlfs_entry.h - Declaration of a generic entry class in tcmifs 
 * 
 * 
 * 
 * 
 * 
 * 
 *
 * Date: 03/26/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ctlfs_entry.h,v 1.3 2007/10/07 15:54:00 stavam2 Exp $
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

#ifndef _TCMI_CTLFS_ENTRY_H
#define _TCMI_CTLFS_ENTRY_H

#include <linux/dcache.h>
#include <linux/types.h>
#include <linux/fs.h>

#include <dbg.h>

/** @defgroup tcmi_ctlfs_entry_class tcmi_ctlfs_entry class 
 * 
 * @ingroup tcmi_ctlfs_class
 *
 * This class represents a generic entry in the tcmi control file system.
 * No instances of this class can be made. The goal is to provide generic
 * functionality that can be used by specific file system entries:
 * - Initialization/deletion/freeing of instances.
 * - Instance reference counting - this is needed as we expect
 * multiple threads of execution to access one entry at a time. This
 * ensures no memory leaks will occur. The reference counting is not
 * implemented from scratch. Each entry is associated with a VFS
 * dentry. These are VFS objects that already have a reference
 * counter. Each instance registers with VFS and receives a
 * notification when the reference counter reaches 0
 * (tcmi_ctlfs_dentry_release()) and the associated dentry is to be
 * freed. This approach provides a safe access to the entry instances
 * from multiple threads of execution without writing an extra
 * reference couting code.  
 * - entry location methods - a generic method that iterates through
 * all predecessors of an entry is provided. This is used when
 * determining the depth of the entry in the filesystem tree.
 *
 *@{
 */

/** Maximum entry name length. */
#define MAX_ENTRY_NAME_LEN 256

/**
 * An entry compound structure. Contains the associated dentry.
 */
struct tcmi_ctlfs_entry {
	/** VFS directory entry(dentry) that is associated with an instance. */
	struct dentry *dentry;
	/** custom operations */
	struct tcmi_ctlfs_ops *entry_ops;
	/** current entry name */
	char name[MAX_ENTRY_NAME_LEN];
};


/** entry operations that support polymorphism */
struct tcmi_ctlfs_ops {
	/** child class method, that frees all instance resources
	 * (but keeps the instance).
	 */
	void (*entry_release)(struct tcmi_ctlfs_entry *);
};

/** \<\<public\>\> Initializes an instance. */
extern int tcmi_ctlfs_entry_init(struct tcmi_ctlfs_entry *self,
				 struct tcmi_ctlfs_entry *parent,
				 mode_t mode,
				 struct tcmi_ctlfs_ops *entry_ops,
				 const struct inode_operations *i_ops,
				 const struct file_operations *f_ops,
				 const char namefmt[], va_list args);
	
/** \<\<public\>\> Initializes a root entry instance. */
extern int tcmi_ctlfs_rootentry_init(struct tcmi_ctlfs_entry *self,
				     struct super_block *sb,
				     mode_t mode,
				     struct tcmi_ctlfs_ops *entry_ops,
				     const struct inode_operations *i_ops,
				     const struct file_operations *f_ops);

/** Callback method of an object when traversing through predecessors */
typedef void callback (struct tcmi_ctlfs_entry *, void *);
/** \<\<public\>\> Iterator through entry predecessors. */
void tcmi_ctlfs_entry_traverse_pred(struct tcmi_ctlfs_entry *self, callback *func, void *data);

/** \<\<public\>\> Calculates the current depth of the entry in the tree. */
int tcmi_ctlfs_entry_depth(struct tcmi_ctlfs_entry *entry);
/** \<\<public\>\> Calculates the length of the pathname string. */
int tcmi_ctlfs_entry_path_length(struct tcmi_ctlfs_entry *self);
/** \<\<public\>\> Fills in the entry's pathname. */
void tcmi_ctlfs_entry_fill_path(struct tcmi_ctlfs_entry *self, char *buffer, int length);


/** Extracts the entry object from the dentry */
#define TCMI_CTLFS_DENTRY_TO_ENTRY(dentry) TCMI_CTLFS_ENTRY(dentry->d_fsdata)
/** Casts a child entry to the super class*/
#define TCMI_CTLFS_ENTRY(e) ((struct tcmi_ctlfs_entry *)e)


/** 
 * \<\<public\>\> Increments the number of links referencing this
 * entry's inode.  This method is used by directory entries. When a
 * new child entry is added to the directory, the number of links in
 * the inode->i_nlink needs to be adjusted as well as its parent's
 * inode->i_nlink also needs to be incremented @param *self - pointer
 * to this instance
 *    
 */
static inline void tcmi_ctlfs_entry_inc_links(struct tcmi_ctlfs_entry *self)
{
	/* FIX ME: down i_sem */
	self->dentry->d_inode->i_nlink++;
	/* up i_sem */
}

/** 
 * \<\<public\>\> Entry name accessor.
 *
 * @param *self - pointer to this instance
 * @return - entry name
 */
static inline const char* tcmi_ctlfs_entry_name(struct tcmi_ctlfs_entry *self)
{
	return self->dentry->d_name.name;
}


/** 
 * \<\<public\>\> Checks whether a given entry is the root of the TCMI
 * ctlfs tree.  This is when the d_parent of the dentry of this
 * tcmi_ctlfs_entry points back to the dentry
 *
 * @param *self - pointer to this instance
 * @return - 1 if it is root
 */
static inline int tcmi_ctlfs_entry_isroot(struct tcmi_ctlfs_entry *self)
{
	struct dentry *dentry = self->dentry;
	return (dentry == dentry->d_parent);
}


/** 
 * \<\<public\>\> Instance accessor, Calls instance custom get method
 * and increments the reference counter.  The user is now guaranteed,
 * that the instance stays in memory while he is using it.
 *
 * @param *self - pointer to this instance
 * @return tcmi_ctlfs_entry instance
 */
static inline struct tcmi_ctlfs_entry* tcmi_ctlfs_entry_get(struct tcmi_ctlfs_entry *self)
{
	if (self) {
		mdbg(INFO4, "Incrementing ref. count of '%s'(%p)", 
		     tcmi_ctlfs_entry_name(self), self);
		dget(self->dentry);
	}
	return self;
}

/** 
 * \<\<public\>\> Returns parent entry, reference counter is
 * incremented.
 *
 * @param *self - pointer to this instance
 * @return - parent entry.
 */
static inline struct tcmi_ctlfs_entry* tcmi_ctlfs_entry_get_parent(struct tcmi_ctlfs_entry *self)
{
	return tcmi_ctlfs_entry_get(TCMI_CTLFS_DENTRY_TO_ENTRY(self->dentry->d_parent));
}

/** 
 * \<\<public\>\> Calls the instance custom put method(if any) and
 * Decrements the reference counter of the dentry that the instance
 * uses to carry it through out the file system. If the reference
 * count reaches 0, the dentry will be released from the dentry cache
 * and the instance of tcmi_ctlfs_entry will be freed. (VFS calls the
 * d_delete() method of the dentry that we have customized @see
 * tcmi_ctlfs_dentry_delete and d_release() that is also customized in
 * tcmi_ctlfs_dentry_release())
 *
 *@param *self - pointer to this instance
 */
static inline void tcmi_ctlfs_entry_put(struct tcmi_ctlfs_entry *self)
{
	if (self) {
		mdbg(INFO4, "Decrementing ref. count of '%s'(%p) (dentry: %p). Count before dec: %d", 
		     tcmi_ctlfs_entry_name(self), self, self->dentry, atomic_read(&self->dentry->d_count));
		dput(self->dentry);
	}
}


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CTLFS_ENTRY_PRIVATE

/** A callback method for calculation of entry depth. */
static void tcmi_ctlfs_add_depth(struct tcmi_ctlfs_entry *self, void *curr_depth);
/** A callback method for calculation of entry pathname length. */
static void tcmi_ctlfs_add_length(struct tcmi_ctlfs_entry *self, void *curr_length);
/** A callback method when building a pathname of an entry. */
static void tcmi_ctlfs_add_path(struct tcmi_ctlfs_entry *self, void *buf_data);

/** Frees all entry resources. */
/*static int tcmi_ctlfs_entry_free(struct tcmi_ctlfs_entry *self);*/

/** Checks for existing directory entry. */
static int tcmi_ctlfs_entry_exists(struct tcmi_ctlfs_entry *self, const char *name);

/** Creates a new inode. */
static struct inode* tcmi_ctlfs_get_inode(struct inode *dir, struct super_block *sb, 
					  mode_t mode,
					  const struct inode_operations *i_ops,
					  const struct file_operations *f_ops);
/** A method for dentry allocation. */
static int tcmi_ctlfs_entry_alloc_dentry(struct tcmi_ctlfs_entry *self, 
					 struct tcmi_ctlfs_entry *parent, 
					 struct inode *inode);

/** VFS dentry_delete operation. */
static int tcmi_ctlfs_dentry_delete(struct dentry *dentry);

/** VFS dentry_delete operation. */
static void tcmi_ctlfs_dentry_release(struct dentry *dentry);

/** Custom dentry delete operation for VFS. */
static struct dentry_operations tcmi_ctlfs_dentry_ops;

#endif /* TCMI_CTLFS_ENTRY_PRIVATE */

/**
 * @}
 */

#endif /* _TCMI_CTLFS_ENTRY_H */
