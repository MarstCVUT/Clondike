/**
 * @file tcmi_ctlfs.c - TCMI control file system module provides basic super
 *                      block handling and is responsible for root directory
 *                      instantiation.
 * 
 *
 *
 * Date: 03/26/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ctlfs.c,v 1.3 2007/10/07 15:54:00 stavam2 Exp $
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

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h>

#include "tcmi_ctlfs_dir.h"
#define TCMI_CTLFS_PRIVATE
#include "tcmi_ctlfs.h"


#include <dbg.h>


/** Magic number identifies the fs super block */
#define TCMI_CTLFS_MAGIC 0x20050326




/**
 * \<\<public\>\> Root entry accessor. If the filesystem has not been
 * activated yet, a kernel mount is performed. The kernel mount in
 * turn ensures instantiation of the root directory. A user might have
 * already mounted the filesystem from userspace. This is not a
 * problem as we allow only one instance of the filesystem in
 * memory(enforced by get_sb_single()), so only a reference counter of
 * the root directory and the associated superblock is
 * incremented. The last thread that releases the last reference to
 * the super block and eventually to the root directory causes the
 * whole filesystem to be released. See tcmi_ctlfs_kill_super() for
 * details on this.
 *
 * If the filesystem is already active (the vfs_mnt is valid), the
 * mount reference count is adjusted.
 *
 * It is necessary to protect the vfs_mnt variable as it can be
 * manipulated by other threads that might issue tcmi_ctlfs_put_root()
 * and release the vfs mount from under us.
 *
 * @return pointer to the root entry (reference counter adjusted by
 * tcmi_ctlfs_entry_get()) or NULL
 */
struct tcmi_ctlfs_entry* tcmi_ctlfs_get_root(void)
{
	down(&self.vfs_mnt_sem);
	/* when the VFS mount is currently inactive we have to create
	 * a new one */
	if (!self.vfs_mnt) {
		mdbg(INFO3, "Mounting");
		self.vfs_mnt = kern_mount(&self.fs_type);
		if (IS_ERR(self.vfs_mnt)) {
			minfo(ERR1, "Could not mount!");
			self.vfs_mnt = NULL;
			/* if the mount failed, the root directory
			 * instance doesn't exist too, so we don't
			 * need to explicitely delete it*/
			self.root_dir = NULL;
		}
	} 
	/* VFS mount still active, just increment its ref. counter */
	else
		mntget(self.vfs_mnt);
	up(&self.vfs_mnt_sem);

	mdbg(INFO3, "sb c_count=%d, s_active=%d", self.sb->s_count, 
	     atomic_read(&self.sb->s_active));
	mdbg(INFO3, "mount mnt_count=%d", atomic_read(&self.vfs_mnt->mnt_count));
	return tcmi_ctlfs_entry_get(self.root_dir);
}

/**
 * \<\<public\>\> Releases the root directory and then the mount
 * object.  VFS takes care of releasing the object in case their
 * reference count reaches 0. We need to take care of setting the
 * vfs_mnt to NULL.
 * 
 * The last 2 steps don't need to be semaphore protected as we are
 * working on copies of both pointers and we wanted to protect the
 * access to vfs_mnt only. If anybody tries to get a root from now on
 * a new VFS mount will be created. If the root directory has been
 * also destroyed, the read_super method takes care of instantiating a
 * new super block along with the root directory.
 */
void tcmi_ctlfs_put_root(void)
{
	struct vfsmount *cur_vfs_mnt;
	struct tcmi_ctlfs_entry *cur_root_dir;
	mdbg(INFO3, "sb c_count=%d, s_active=%d", self.sb->s_count, 
	     atomic_read(&self.sb->s_active));
	mdbg(INFO3, "mount mnt_count=%d", atomic_read(&self.vfs_mnt->mnt_count));

	/* mdbg(INFO3, "root d_count=%d", atomic_read(&self.root_dir->super.dentry->d_count));*/

	down(&self.vfs_mnt_sem);
	cur_vfs_mnt = self.vfs_mnt;
	cur_root_dir = self.root_dir;
	/* If we are the last thread accesing the kernel mount */
	if (atomic_read(&self.vfs_mnt->mnt_count) == 1) 
		self.vfs_mnt = NULL;

	up(&self.vfs_mnt_sem);

	/* no locking needed since working on copies of both
	 * pointers */
	tcmi_ctlfs_entry_put(cur_root_dir);
	mntput(cur_vfs_mnt);
}


/** @addtogroup tcmi_ctlfs_class
 *
 * @{
 */

static struct kmem_cache *ctlfs_inode_cachep;

static void init_once(void *foo) {
         struct inode *inode = (struct inode *) foo;
 
         inode_init_once(inode);
 }

/*
 * Overriden in order to perform GFP_ATOMIC allocation
 */
#if 0
static struct inode *tcmi_ctlfs_alloc_inode(struct super_block *sb)
{
	struct inode* res;
	mdbg(INFO4, "TMPDBG: BEFORE ALLOC");
	res = kmem_cache_alloc(ctlfs_inode_cachep, GFP_ATOMIC);
	mdbg(INFO4, "TMPDBG: AFTER ALLOC: %p", res);

	if (!res) {
		return NULL;
	}

	return res;
}

/** Must also override corresponding destroy method */
static void tcmi_ctlfs_destroy_inode(struct inode *inode) {
	mdbg(INFO4, "TMPDBG: BEFORE FREE %p", inode);
	kmem_cache_free(ctlfs_inode_cachep, inode);
	mdbg(INFO4, "TMPDBG: AFTER FREE");
}
#endif

static int init_inodecache(void)
{
	ctlfs_inode_cachep = kmem_cache_create("ctlfs_inode_cache",
					     sizeof(struct inode),
					     0, (SLAB_RECLAIM_ACCOUNT|
						SLAB_MEM_SPREAD),
					     init_once);
	if (ctlfs_inode_cachep == NULL)
		return -ENOMEM;

	return 0;
}

static void destroy_inodecache(void)
{
	kmem_cache_destroy(ctlfs_inode_cachep);
}


static struct super_operations s_ops = {
	.statfs = simple_statfs,
//	.alloc_inode = tcmi_ctlfs_alloc_inode,
	//.destroy_inode = tcmi_ctlfs_destroy_inode,
};

/**
 * \<\<private\>\> Sets up super block and instantiates the root
 * directory. This method is called only once through out the super
 * block life.
 *
 * @param sb - pointer to the super block
 * @param *data - custom data supplied when reading the superblock(not used)
 * @param silent - required by VFS, no idea what it means.
 * 
 * @return 0 upon success
 */
static int tcmi_ctlfs_fill_super(struct super_block *sb, void *data, int silent)
{
	sb->s_blocksize = PAGE_CACHE_SIZE;
	sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
	sb->s_magic = TCMI_CTLFS_MAGIC;
	sb->s_op = &s_ops;
	self.sb = sb;
mdbg(INFO4, "TMPDBG: GOING TO REQUEST ROOT");
	self.root_dir = tcmi_ctlfs_rootdir_new(sb);
mdbg(INFO4, "TMPDBG: ROOT SUCCEEDED");
	return 0;
}

/**
 * \<\<private\>\> Super block as assigned a nonexistent device and
 * the super block is obtained via get_sb_single(). The kernel
 * function get_sb_single() ensures that only 1 instance of the
 * filesystem can be mounted at a time.  Custom method to fill the
 * super block is passed as parameter.
 *
 * @param *fst - points to the file system type, that is being mounted
 * @param flags - 
 * @param *devname - device name, on which the file system resides (it has no meaning here)
 * @param *data - pointer to private filesystem data - not used
 *
 * @return - pointer to the super block object that has been created
 */
static int tcmi_ctlfs_get_super(struct file_system_type *fst,
						int flags, const char *devname, void *data, struct vfsmount *mnt)
{
	return get_sb_single(fst, flags, data, tcmi_ctlfs_fill_super, mnt);
}

/**
 * Module initialization
 *
 * @return - status of the register_filesystem() call
 */
static int __init tcmi_ctlfs_init(void)
{
	int error;
	minfo(INFO3, "Registering TCMI control filesystem");
	error = init_inodecache();
	if (error)
		return error;

	error = register_filesystem(&self.fs_type);

	return error;
}

/**
 * module cleanup
 */
static void __exit tcmi_ctlfs_exit(void)
{
	minfo(INFO3, "Unregistering TCMI control filesystem");
	destroy_inodecache();
	unregister_filesystem(&self.fs_type);
}



/** \<\<singleton\>\> instance initialization */
static struct tcmi_ctlfs self = {
	/** Root directory of the TCMI control filesystem. */
	.root_dir = NULL,
	/** This semaphore protects the vfs_mount value */
	.vfs_mnt_sem = __SEMAPHORE_INITIALIZER( self.vfs_mnt_sem, 1 ),
	/** Filesystem mount object */
	.vfs_mnt = NULL,
	/** Filesystem super block object */
	.sb = NULL,
	/** File system type descriptor, only get_sb method is needed
	 * to be customized
	 */
	.fs_type = {
		.owner          = THIS_MODULE,
		.name           = "tcmi_ctlfs",
		.get_sb         = tcmi_ctlfs_get_super,
		.kill_sb        = kill_litter_super, 
	},
};

/**
 * @}
 */

module_init(tcmi_ctlfs_init);
module_exit(tcmi_ctlfs_exit);

EXPORT_SYMBOL_GPL(tcmi_ctlfs_get_root);
EXPORT_SYMBOL_GPL(tcmi_ctlfs_put_root);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

