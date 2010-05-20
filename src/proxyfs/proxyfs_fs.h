/**
 * @file proxyfs_fs.h - Proxy filesystem for file related system calls 
 * forwarding
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_fs.h,v 1.5 2007/10/20 14:24:20 stavam2 Exp $
 *
 * This file is part of Proxy filesystem (ProxyFS)
 * Copyleft (C) 2007 Petr Malat
 * 
 * ProxyFS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * ProxyFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _PROXYFS_FS_H
#define _PROXYFS_FS_H

/** @defgroup proxyfs_fs_class proxyfs_fs class 
 * @ingroup proxyfs_module_class
 *
 * @{
 */

/** \<\<public\>\> Registr proxyfs in kernel */
int proxyfs_fs_init(void);

/** \<\<public\>\> Unregistr proxyfs in kernel */
void proxyfs_fs_exit(void);

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef PROXYFS_FS_PRIVATE

/* \<\<private\>\> Returns superblock */
static int proxyfs_fs_get_sb(struct file_system_type *, int, const char *, void *, struct vfsmount*);
/* \<\<private\>\> Frees superblock */
static void proxyfs_fs_kill_sb(struct super_block *);



/* super_operations */
/** \<\<private\>\> Called when reading superinode */
//static void proxyfs_fs_super_read_inode(struct inode *);
/** \<\<private\>\> Called when writing inode */
static int proxyfs_fs_super_write_inode(struct inode *, int);



/* inode_operations */
/** \<\<private\>\> lookup inode */
static struct dentry *proxyfs_fs_rootinode_lookup(struct inode *, struct dentry *, struct nameidata *);

/** \<\<private\>\> getattr */
static int proxyfs_fs_fdinode_getattr(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat);

/** \<\<private\>\> release */
static int proxyfs_fs_fdinode_release(struct inode *inode, struct file *file);

/** \<\<private\>\> Sequentional read */
static ssize_t proxyfs_fs_fdinode_read(struct file *file, char __user *buf, size_t count, loff_t *offset);

/** \<\<private\>\> Sequentional write */
static ssize_t proxyfs_fs_fdinode_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

/** \<\<private\>\> Open file */
static int proxyfs_fs_fdinode_open(struct inode *, struct file *);

/** \<\<private\>\> ioctl */
static long proxyfs_fs_fdinode_ioctl (struct file *file, unsigned int cmd, unsigned long arg);

/** \<\<private\>\> Fsync function for VFS */
static int proxyfs_fs_fdinode_fsync(struct file *file, struct dentry *dentry, int datasync);

/** \<\<private\>\> poll */
#define proxyfs_fs_fdinode_poll proxyfs_file_poll

/*
 * Structures with operations
 */

/** \<\<private\>\> Superblock operations */
static struct super_operations proxyfs_sops = {
	//.read_inode 	= proxyfs_fs_super_read_inode,
	.statfs 	= simple_statfs, /* handler from libfs */
	.write_inode 	= proxyfs_fs_super_write_inode
};

/** \<\<private\>\> Rootinode inode operations */
static struct inode_operations rootinode_iops = {
	.lookup = proxyfs_fs_rootinode_lookup
};

/** \<\<private\>\> fdinode file operations */
static struct file_operations fdinode_fops = {
	.open 	 	= proxyfs_fs_fdinode_open,
	.read 	 	= proxyfs_fs_fdinode_read,
	.write 	 	= proxyfs_fs_fdinode_write,
	.unlocked_ioctl = proxyfs_fs_fdinode_ioctl,
	.release 	= proxyfs_fs_fdinode_release,
	.fsync 	 	= proxyfs_fs_fdinode_fsync,
};
/** \<\<private\>\> Inode operations  */
static struct inode_operations fdinode_iops = {
	.getattr 	= proxyfs_fs_fdinode_getattr,
};

/** \<\<private\>\> filesystem type structure */
static struct file_system_type proxyfs = {
	.name 	 = "proxyfs",
	.get_sb  = proxyfs_fs_get_sb,
	.kill_sb = proxyfs_fs_kill_sb,
	.owner 	 = THIS_MODULE
};

/** \<\<private\>\> Pointer to root inode */
//static struct inode *rootinode;
int file_size = 42;
#endif /* PROXYFS_FS_PRIVATE */

/**
 * @}
 */

#endif /* _PROXYFS_FS_H */
