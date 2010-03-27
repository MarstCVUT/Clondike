/**
 * @file tcmi_ctlfs_entry.c - Implementation of an entry class in tcmifs 
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
 * $Id: tcmi_ctlfs_entry.c,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#include <linux/smp_lock.h>
#include <linux/pagemap.h>

#define TCMI_CTLFS_ENTRY_PRIVATE
#include "tcmi_ctlfs_entry.h"

#include <dbg.h>

/** 
 * \<\<public\>\> This method is used when initializing a new entry.
 * It follows these steps:
 *
 * - converts the nameformat string and the args into a regular string
 * This will be the name assigned to the dentry
 * - checks whether we have a valid parent directory
 * - check whether an entry with the same name already exists
 * in the parent directory entry(if so, an error is returned)
 * - creates a new inode with the specified super block and operations
 * - creates a new dentry associated with the inode.
 * - sets a entry_operations specific to this particular entry type 
 * This supports polymorphism.
 *
 * @param *self - pointer to this instance
 * @param *parent - pointer to the parent entry - has to be a directory
 * @param *mode - file type and access rights for the inode
 * @param *entry_ops - custom operations of the child class
 * @param *i_ops - inode operations
 * @param *f_ops - file operations(also stored in the new inode)
 * @param namefmt - nameformat string (printf style)
 * @param args - arguments to the format string
 * @return 0 upon success
 */
int tcmi_ctlfs_entry_init(struct tcmi_ctlfs_entry *self,
			  struct tcmi_ctlfs_entry *parent,
			  mode_t mode,
			  struct tcmi_ctlfs_ops *entry_ops,
			  const struct inode_operations *i_ops,
			  const struct file_operations *f_ops,
			  const char namefmt[], va_list args)
{
	struct inode *dir;
	struct inode *inode;

	vsnprintf(self->name, sizeof(self->name), namefmt, args);
	mdbg(INFO4, "Initializing a new tcmi_ctlfs_entry, '%s'(%p)", self->name, self);
	if (!parent) {
		mdbg(ERR4, "Can't initialized new entry '%s', no parent specified! ", self->name);
		goto exit0;
	}
	mdbg(INFO4, "Parent entry='%s'", tcmi_ctlfs_entry_name(parent));
	/* */
	dir = parent->dentry->d_inode;
	if (!S_ISDIR(dir->i_mode)) {
		mdbg(ERR3, "Can't initialize new entry '%s', parent entry is not a directory!", self->name);
		goto exit0;
	}


	/* check parent for an entry with 'name' */
	if (tcmi_ctlfs_entry_exists(parent, self->name)) {
		mdbg(ERR3,"Can't initialize new entry '%s', parent entry contains an entry of the same name!", 
		     self->name);
		goto exit0;
	}

	/* allocate a new inode */
	if (!(inode = tcmi_ctlfs_get_inode(dir, dir->i_sb, mode, i_ops, f_ops))) {
		mdbg(ERR3, "Failed to allocate an inode for '%s'", self->name);
		goto exit0;
	}
	/* allocate a new dentry, associated with the inode */
	if (tcmi_ctlfs_entry_alloc_dentry(self, parent, inode)) {
		mdbg(ERR3, "Failed to allocate a new dentry for %s", self->name);
		goto exit1;
	}
	/* Custom operations of the child class */
	self->entry_ops = entry_ops;
	return 0;
	/* error handling */
 exit1:
	iput(inode);
 exit0:
	return -ENOSPC;
}

/** 
 * \<\<public\>\> Initializes a root entry instance.
 *
 * @param *self - pointer to this instance
 * @param *sb - pointer to the super block of the filesystem
 * @param *mode - file type and access rights for the inode
 * @param *entry_ops - custom operations of the child class
 * @param *i_ops - inode operations
 * @param *f_ops - file operations(also stored in the new inode)
 * @return 0 upon success
 */
int tcmi_ctlfs_rootentry_init(struct tcmi_ctlfs_entry *self,
			      struct super_block *sb,
			      mode_t mode,
			      struct tcmi_ctlfs_ops *entry_ops,
			      const struct inode_operations *i_ops,
			      const struct file_operations *f_ops)
{
	struct dentry *root;
	struct inode *inode;

	mdbg(INFO3, "Initializing a root entry");
	if (!(inode = tcmi_ctlfs_get_inode(NULL, sb, mode, i_ops, f_ops))) {
		mdbg(ERR1, "Failed to allocate inode for root dentry!");
		goto exit0;
	}
	/* try allocating the dentry */
	if (!(root = d_alloc_root(inode))) {
		mdbg(ERR1, "Failed to allocate a root dentry!");
		goto exit1;
	}
	/* Initialize the dentry */
	root->d_fsdata = (void *)self; 
	root->d_op = &tcmi_ctlfs_dentry_ops; 
	self->dentry = root;
	sb->s_root = root;
	/* Custom operations of the child class */
	self->entry_ops = entry_ops;
	return 0;
	/* error handling*/
 exit1:
	iput(inode);
 exit0:
	return -ENOSPC;
}



/** 
 * \<\<public\>\> Iterates through all entry predecessors and calls
 * the specified method for each entry (including self). The root
 * entry is omitted. The callback function is supplied with the
 * pointer to data that the users specified as parameter.  This
 * provides a safe way of iterating through all predecessors. The
 * reference counters are used to control the access from multiple
 * threads.
 *
 * @param *self - pointer to this instance
 * @param *func - pointer to the callback function
 * @param *data - pointer to the data that is passed the callback.
 */
void tcmi_ctlfs_entry_traverse_pred(struct tcmi_ctlfs_entry *self, callback *func, void *data)
{
	struct tcmi_ctlfs_entry *parent;
	struct tcmi_ctlfs_entry *entry = self;
	/* increment the ref counter and get the instance */
	parent = tcmi_ctlfs_entry_get(entry);

	/* mdbg(INFO4, "getting %s", tcmi_ctlfs_entry_name(entry)); */
	while (!tcmi_ctlfs_entry_isroot(parent)) {
		entry = parent;
		parent = tcmi_ctlfs_entry_get_parent(entry);
		/* mdbg(INFO4, "getting %s", tcmi_ctlfs_entry_name(parent)); */
		(*func)(entry, data); /* notify the next entry */
		tcmi_ctlfs_entry_put(entry);
		/* mdbg(INFO4, "putting %s", tcmi_ctlfs_entry_name(entry)); */
	} 
	/* decrement the root's reference counter too */
	tcmi_ctlfs_entry_put(parent);
	/* mdbg(INFO4, "putting %s", tcmi_ctlfs_entry_name(parent)); */
}


/** 
 * \<\<public\>\> Calculates the depth of the entry in the tree.
 *
 * @param *self - pointer to this instance
 * @return - depth (0 denotes root entry)
 */
int tcmi_ctlfs_entry_depth(struct tcmi_ctlfs_entry *self)
{
	int depth = 0;

	tcmi_ctlfs_entry_traverse_pred(self, tcmi_ctlfs_add_depth, &depth);
	return depth;
}

/**
 * \<\<public\>\> Calculates the length of the pathname string
 * (including '/' for every path element).
 *
 * @param *self - pointer to this instance
 * @return pathname length
 */
int tcmi_ctlfs_entry_path_length(struct tcmi_ctlfs_entry *self)
{
	int length = 1;
	tcmi_ctlfs_entry_traverse_pred(self, tcmi_ctlfs_add_length, &length);
	return length;
}

/**
 * \<\<public\>\> Fills in the entry's pathname into the specified
 * buffer.  The pathname is being filled in reverse order starting
 * from the end of the buffer.
 * 
 * @param *self - pointer to this symlink instance
 * @param *buffer - buffer where store the pathname
 * @param length - size of the buffer allocated for the pathname - this
 *                 is can be > pathname length, as the user might want to add
 *                 some prefix to the pathname (e.g. ../'s)
 */
void tcmi_ctlfs_entry_fill_path(struct tcmi_ctlfs_entry *self, char *buffer, int length)
{
	struct {
		char *buffer;
		int length;
	} buf_data;

	buf_data.buffer = buffer;
	/* account for terminating \0 */
	buf_data.length = --length; 

	tcmi_ctlfs_entry_traverse_pred(self, tcmi_ctlfs_add_path, &buf_data);	
}

/** @addtogroup tcmi_ctlfs_entry_class
 *
 * @{
 */

/** 
 *  \<\<private\>\> Helper callback function when calculating entry
 *  depth.
 *
 * @param *self - pointer to this instance
 * @param *curr_depth - pointer to the current depth
 */
static void tcmi_ctlfs_add_depth(struct tcmi_ctlfs_entry *self, void *curr_depth)
{
	(*((int *)curr_depth))++;
}


/** 
 *  \<\<private\>\> Helper callback function when calculating entry
 *  pathname length.
 *
 * @param *self - pointer to this instance
 * @param *curr_length - pointer to the current length
 */
static void tcmi_ctlfs_add_length(struct tcmi_ctlfs_entry *self, void *curr_length)
{
	/* + 1 accounts for '/' */
	(*((int *)curr_length)) += strlen(tcmi_ctlfs_entry_name(self)) + 1;
}


/** 
 *  \<\<private\>\> Helper callback method when filling the entry's
 * pathname The entry fills in its name and the '/' as prefix to the
 * current end of the buffer.
 *
 * @param *self - pointer to this instance
 * @param *buf_data - buffer data that contain pointer to the buffer and
 *                    its current length.
 */
static void tcmi_ctlfs_add_path(struct tcmi_ctlfs_entry *self, void *buf_data)
{
	struct {
		char *buffer;
		int length;
	} *buf = buf_data;

	int cur = strlen(tcmi_ctlfs_entry_name(self));
	mdbg(INFO4, "Adding.. %s, buffer length=%d", tcmi_ctlfs_entry_name(self), buf->length);
		
	/* back up enough to print entry name with '/' prefix */
	buf->length -= cur;
	strncpy(buf->buffer + buf->length, tcmi_ctlfs_entry_name(self), cur);
	*(buf->buffer + --(buf->length)) = '/';

	mdbg(INFO4, "New buffer length=%d", buf->length);
}


/** 
 *  \<\<private\>\> This private method is responsible for freeing any
 * resources the entry is currently using(currently nothing). When
 * possible the child class is also notified.  The instance is
 * eventually destroyed.
 *
 * @param *self - pointer to this instance 
 * @return 0
 */
static inline void tcmi_ctlfs_entry_release(struct tcmi_ctlfs_entry *self)
{
	mdbg(INFO4, "entry '%s' is being freed(%p)", 
	     tcmi_ctlfs_entry_name(self), self);
	/* free child class instance data? */
	if (self->entry_ops && self->entry_ops->entry_release) {
		self->entry_ops->entry_release(self);	
	}

	kfree(self);
}


/** 
 *  \<\<private\>\> A method for inode allocation.  Currently the uid
 * and gid is assigned based on the current's fsuid and fsgid. Block
 * size is not relevant, set to page size.  If the user doesn't
 * specify any inode or file operations, the default(empty operations,
 * but not NULL) assigned by the VFS will be kept.
 *
 * If the user specified a parent directory inode, it's access rights
 * are investigated and inherited by the new inode as follows:
 * - if the parent has the SGID bit set, parent's gid is assigned to
 * the new inode.
 * - a parent SGID bit is set on the new inode, if the new inode is a 
 * directory
 * 
 *
 * @param *dir - inode of the parent directory for the new inode
 * @param *sb - super block of the filesystem where the inode is
 * to be allocated
 * @param *mode - file type and access rights for the inode
 * @param *i_ops - inode operations
 * @param *f_ops - file operations (also stored in the new inode)
 * @return a pointer to the new inode or NULL
 */
static struct inode* tcmi_ctlfs_get_inode(struct inode *dir, 
					  struct super_block *sb, 
					  mode_t mode,
					  const struct inode_operations *i_ops,
					  const struct file_operations *f_ops)
{
	struct inode *inode = new_inode(sb);

	if (inode) {
		inode->i_mode = mode;
		inode->i_uid = current_fsuid();
		inode->i_gid = current_fsgid();
		inode->i_blocks = 0;
		inode->i_op  = (i_ops ? i_ops : inode->i_op);
		inode->i_fop = (f_ops ? f_ops : inode->i_fop);
		inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
		mdbg(INFO4, "New inode allocated, inode number = %ld", inode->i_ino);
		/* check parent inode's(if any) access rights */
		if (dir && (dir->i_mode & S_ISGID)) {
			inode->i_gid = dir->i_gid;
			if (S_ISDIR(mode))
				inode->i_mode |= S_ISGID;
		}

	}
	else
		minfo(ERR3, "Failed to allocate a new inode");

	return inode;
}

/** 
 *  \<\<private\>\>   The new dentry is
 * allocated based on the parent's dentry and the current name of the
 * ctlfs entry. The dentry is associated with the specified inode. The
 * dentry is assigned our custom operations vector, so that we are
 * informed about dentry destruction.
 *
 * @param *self - pointer to this instance
 * @param *parent - pointer to the parent entry - has to be a directory
 * @param *inode - pointer to the inode, that is to be associated with
 * the new dentry.
 * @return 0 upon success.
 */
static int tcmi_ctlfs_entry_alloc_dentry(struct tcmi_ctlfs_entry *self, 
					 struct tcmi_ctlfs_entry *parent, 
					 struct inode *inode)
{
	struct dentry *dentry;
	int error = 1;
	/* try allocating the dentry */
	if ((dentry = d_alloc_name(parent->dentry, self->name))) {
		d_add(dentry, inode); /* enters the dentry into hash queues */
		dentry->d_fsdata = (void *)self; 
		dentry->d_op = &tcmi_ctlfs_dentry_ops; 
		self->dentry = dentry;
		/* dget(dentry); Extra count - pin the dentry in core - not needed */
		error = 0;
	}
	else 
		mdbg(ERR3, "Failed to allocate a new dentry for %s", self->name);
	return error;
}

/**
 * \<\<private\>\> Checks whether a directory entry contains entry of
 * the specified name
 * 
 * @param *self - pointer to this instance
 * @param *name - pointer to the name to be checked
 * @return 1 - if the entry of the specified name exists
 */
static int tcmi_ctlfs_entry_exists(struct tcmi_ctlfs_entry *self, const char *name)
{
	struct dentry *dentry;
	struct qstr q;
	int exists = 0;

	q.name = name;
	q.len = strlen(name);
	q.hash = full_name_hash(q.name, q.len);
	dentry = d_lookup(self->dentry, &q);
	/* dentry already exists, decrement its reference counter since lookup 
	 incremented it */
	if (dentry) {
		dput(dentry);
	        exists = 1;
	}
	return exists;
}
/**
 *  \<\<private\>\> This class method is a custom implementation of a
 * VFS delete_dentry operation.  The delete_dentry is called by VFS
 * when destroying the dentry associated with a particular
 * tcmi_ctlfs_entry object. This happens when the dentry's reference
 * counter reaches 0.
 *
 * @param *dentry - pointer to the dentry that is to be deleted
 * @return - always 1 since we don't want the VFS to put the dentry
 * into the dentry_unused. This way the dentry will be unhashed and
 * removed right away
 */
static int tcmi_ctlfs_dentry_delete(struct dentry *dentry)
{
	return 1;
}

/**
 *  \<\<private\>\> This class method is a custom implementation of a
 * VFS release_dentry operation.  The release_dentry is called by VFS
 * when releasing the dentry associated with a particular
 * tcmi_ctlfs_entry object. This happens when the dentry's reference
 * counter reaches 0.
 *
 * Unlike previous VFS method, we are allowed to sleep as the dcache
 * lock and dentry lock are not held anymore (see fs/dcache.c)
 *
 * @param *dentry - pointer to the dentry that is to be deleted
 * @return - always 1 since we don't want the VFS to put the dentry
 * into the dentry_unused. This way the dentry will be unhashed and
 * removed right away
 */
static void tcmi_ctlfs_dentry_release(struct dentry *dentry)
{
	/* get the instance first */
	struct tcmi_ctlfs_entry *self = TCMI_CTLFS_DENTRY_TO_ENTRY(dentry);
	if (self)
		tcmi_ctlfs_entry_release(self);
	else
		minfo(ERR2, "VFS issued dentry_release on a dentry without tcmi_ctlfs_instance!!");
}

/** Custom dentry delete operation for VFS. */
static struct dentry_operations tcmi_ctlfs_dentry_ops =
{
	.d_delete	= tcmi_ctlfs_dentry_delete,
	.d_release      = tcmi_ctlfs_dentry_release,
};

/**
 * @}
 */

EXPORT_SYMBOL_GPL(tcmi_ctlfs_entry_depth);
EXPORT_SYMBOL_GPL(tcmi_ctlfs_entry_traverse_pred);
