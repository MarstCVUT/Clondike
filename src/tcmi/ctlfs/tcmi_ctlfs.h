/**
 * @file tcmi_ctlfs.h - TCMI control file system module declaration
 *                      exports the root directory entry
 * 
 *
 *
 * Date: 03/26/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ctlfs.h,v 1.3 2008/05/02 19:59:20 stavam2 Exp $
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
#ifndef TCMI_CTLFS_H
#define TCMI_CTLFS_H

#include <linux/mount.h>
#include <linux/semaphore.h>

#include "tcmi_ctlfs_dir.h"


/** @defgroup tcmi_ctlfs_class control filesystem
 * 
 * @ingroup tcmi_module_class
 *
 * This \<\<singleton\>\> class represents TCMI control filesystem.
 * The idea behind the filesystem is very similar to what we have in
 * sysfs. The reason why a separate filesystem has been created is
 * that TCMI doesn't fit among regular device drivers which is the
 * primary use of sysfs.
 *
 * Similarly to sysfs, we provide a mechanism that allows any object
 * in TCMI to create an interface presented to user space. The user
 * is able to notify the object via reading/writing a file. 
 *
 * An entity in TCMI ctlfs is called an 'entry'. Currently there are
 * entries that represent \link tcmi_ctlfs_file a file \endlink, \link
 * tcmi_ctlfs_dir a directory \endlink and \link tcmi_ctlfs_symlink a
 * symbolic link \endlink.
 *
 *@{
 */

/**
 * A compound structure that contains all internal data of TCMI ctlfs.
 * This is for internal use only.
 */
struct tcmi_ctlfs {
	/** forward declaration of the root directory of the TCMI control filesystem */
	struct tcmi_ctlfs_entry *root_dir;
	/** a semaphore that protects the vfs_mount variable against
	 * concurrent accesses. This is important when issuing
	 * tcmi_ctlfs_get_root/tcmi_ctlfs_put_root
	 */
	struct semaphore vfs_mnt_sem;
	/** forward declaration of the mount object of TCMI control filesystem */
	struct vfsmount *vfs_mnt;
	/** Filesystem super block object */
	struct super_block *sb;
	/** A forward declaration describing this filesystem type */
	struct file_system_type fs_type;
};

/** \<\<public\>\> Root entry accessor. */
extern struct tcmi_ctlfs_entry* tcmi_ctlfs_get_root(void);
/** \<\<public\>\> Releases the root directory. */
extern void tcmi_ctlfs_put_root(void);

/** Preferred directory rights */
#define TCMI_PERMS_DIR    (S_IRWXU | S_IRGRP | S_IXGRP)
/** Preferred write file rights */
#define TCMI_PERMS_FILE_W (S_IWUSR)
/** Preferred read file rights */
#define TCMI_PERMS_FILE_R (S_IRUSR | S_IRGRP)
/** Preferred read/write file rights */
#define TCMI_PERMS_FILE_RW (TCMI_PERMS_FILE_R | TCMI_PERMS_FILE_W)

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CTLFS_PRIVATE
/** TCMI ctlfs \<\<singleton\>\> instance. */
static struct tcmi_ctlfs self;
#endif

/**
 * @}
 */


#endif /* TCMI_CTLFS_H */
