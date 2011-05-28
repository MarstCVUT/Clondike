/**
 * @file proxyfs_fs.c - Proxy filesystem for file related system calls 
 * forwarding
 *                      
 * 
 *
 *
 * Date: 08/02/2007
 *
 * Author: Petr Malat
 *
 * $Id: proxyfs_fs.c,v 1.10 2008/05/02 19:59:20 stavam2 Exp $
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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/statfs.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/buffer_head.h>
#include <linux/pagemap.h> 
#include <asm/uaccess.h>
#include <asm/errno.h>


#include <dbg.h>
#include <proxyfs/buffer.h> 
#include <proxyfs/proxyfs_client.h> 
#include <proxyfs/proxyfs_proxy_file.h> 

#define PROXYFS_FS_PRIVATE
#include "proxyfs_fs.h" 
#include "proxyfs_fs_helper.h" 
#include "proxyfs_ioctl.h" 
#include "proxyfs_task.h" 

#define PROXYFS_TASK_PROTECTED
#include <proxyfs/proxyfs_task.h> 
//#include "proxyfs_fs_ioctl.h" 

/** \<\<private\>\> Fill proxyfs superblock
 *
 * @param sb     - pointer to superblock
 * @param data   - mount options etc...
 * @param silent -
 *
 * @return Zero on success
 */
static int proxyfs_fs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *rootinode;

	sb->s_blocksize      = 1024;
	sb->s_blocksize_bits = 10;
	sb->s_magic 	     = 0x28021985;
	sb->s_op   	     = &proxyfs_sops; // super block operations
	sb->s_type 	     = &proxyfs;      // file_system_type
	sb->s_fs_info	     = data;          // proxyfs_client task

	rootinode = iget_locked(sb, 0xFFFFFFFF); // allocate an inode
	rootinode->i_op   = &rootinode_iops; // set the inode ops
	//rootinode->i_fop  = &rootinode_fops;
	rootinode->i_mode = S_IFDIR | S_IRUSR | S_IXUSR;
	rootinode->i_mtime = rootinode->i_atime = rootinode->i_ctime = CURRENT_TIME;

	if( !(sb->s_root = d_alloc_root(rootinode)) ){
		iput(rootinode);
		return -ENOMEM;
	}

	unlock_new_inode(rootinode);

	return 0;
}

static int proxyfs_fs_get_sb(struct file_system_type *fs_type, 
		int flags, const char *devname, void *data, struct vfsmount *mnt) 
{
	struct proxyfs_task *task;
	mdbg(INFO3, "Mounting ProxyFS, server is on %s", devname);
	

	if ( (task = proxyfs_task_run( proxyfs_client_thread, devname )) == NULL  )
		return -EINVAL;

//	return get_sb_single(fs_type, flags, task, proxyfs_fs_fill_super, mnt);
	return get_sb_nodev(fs_type, flags, task, proxyfs_fs_fill_super, mnt);
}

/**
 * Called on unmount
 */
static void proxyfs_fs_kill_sb(struct super_block *super) 
{
	struct proxyfs_task *task = PROXYFS_TASK(super->s_fs_info); 
	mdbg(INFO3, "Unmounting ProxyFS, task %p", task);
	proxyfs_task_put(task);
	kill_anon_super(super);
}

/*
 * Super-Block Operations
 */

//static void proxyfs_fs_super_read_inode(struct inode *inode) 
//{
//	mdbg(INFO3, "Read inode i_ino=%lx Mode=%d", inode->i_ino, inode->i_mode);
//	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
//}

static int proxyfs_fs_super_write_inode(struct inode *inode, int wait) 
{
	mdbg(INFO3, "write inode i_ino=%lx %d", inode->i_ino, (int)i_size_read(inode));
	return 0;
}

/*
 * Inode Operations
 */

/** \<\<private\>\> root inode lookup 
 *
 * @param parent_inode - 
 * @param dentry       -
 * @param nameidata    - file name (Proxyfile identification number)
 *
 * @return pointer to dentry with filled inode or NULL on error
 */
static struct dentry *proxyfs_fs_rootinode_lookup(
		struct inode *parent_inode, struct dentry *dentry, struct nameidata *nameidata) 
{
	struct inode *fileinode; 
	unsigned long inode_num;
	uint32_t file_number;
	uint32_t owner_pid;

	memory_sanity_check("Prescan");
	
	mdbg(INFO3, "filename %s", dentry->d_name.name);


	if ( sscanf(dentry->d_name.name, "%u-%u", &owner_pid, &file_number) != 2 ) 
		goto not_found; // Invalid format

	memory_sanity_check("Postscan");
	
	inode_num = (owner_pid << 16) + file_number;

	mdbg(INFO3, "rootinode lookup %lx (pid: %d, file_num: %d)\n", inode_num, owner_pid, file_number);
	fileinode = iget_locked(parent_inode->i_sb, inode_num);
	//fileinode = iget(parent_inode->i_sb, inode_num);
	
	if(!fileinode){
		mdbg(INFO3, "Cannot access proxyfs inode");
		goto not_found; // Number isn't pid of tcmi_task
	}

	//fileinode->i_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP;
	// TODO: Was reg, changed to char.. maybe we should decide based on original file`s mode?
	fileinode->i_mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP;
	fileinode->i_fop  = &fdinode_fops;
	fileinode->i_op   = &fdinode_iops;

	d_add(dentry, fileinode);

	if (fileinode->i_state & I_NEW) {
                unlock_new_inode(fileinode);
        }

	return NULL;

not_found:
	d_add(dentry, NULL);
	return ERR_PTR(-EACCES);
}

/*
 * File Operations
 */

/** \<\<private\>\> Open function for VFS
 *
 * @param *inode - pointer to inode identificating file we wanted to open
 * @param *file  - pointer to struct file, which will be representing the newly opened file
 *
 * @return zero on success
 */
static int proxyfs_fs_fdinode_open(struct inode *inode, struct file *file) 
{
	struct proxyfs_proxy_file_t *proxy_file;
	struct proxyfs_client_task *task = PROXYFS_CLIENT_TASK( inode->i_sb->s_fs_info );

	mdbg(INFO2, "Opening file %lu. Its private data are %p", inode->i_ino, file->private_data);

	proxy_file = proxyfs_client_open_proxy(task, inode->i_ino);

	if( IS_ERR(proxy_file) )
		return PTR_ERR(proxy_file);
	file->private_data = proxy_file;
	file->f_pos = 0;
	
	return 0;
}

/** \<\<private\>\> Release function for VFS (called on close)
 * @param *inode - pointer to inode identificating file we wanted to open
 * @param *file  - pointer to struct file, which will be representing the newly opened file
 *
 * @return zero on success
 */
static int proxyfs_fs_fdinode_release(struct inode *inode, struct file *file)
{
	int rtn;
	struct proxyfs_proxy_file_t *proxy_file = file->private_data;

	mdbg(INFO2, "file %p release called", proxy_file);

	rtn = proxyfs_proxy_file_close(proxy_file);
	if( rtn == 0){
		proxyfs_proxy_file_destroy(proxy_file);
		kfree(proxy_file);
	}
	return rtn;
}

/** \<\<private\>\> Fsync function for VFS 
 * @param *file    - pointer to file we wanted to fsync 
 * @param *dentry  - pointer to coresponding dentry structure
 * @param datasync - true for fdatasync system call
 *
 * @return zero on success
 */
static int proxyfs_fs_fdinode_fsync(struct file *file, struct dentry *dentry, int datasync)
{
	struct proxyfs_proxy_file_t *proxy_file = file->private_data;

	mdbg(INFO2, "file %p sync called", proxy_file);

	return proxyfs_proxy_file_fsync(proxy_file);
}


/** \<\<private\>\> Write function for VFS
 * @param *file   - pointer to file we wanted to write to
 * @param *buf    - pointer to buffer in userspace with data 
 * @param count   - size of data
 * @param *offset - pointer to file offset
 *
 * @return Amount of bytes written or error 
 */
static ssize_t proxyfs_fs_fdinode_write(struct file *file, const char *buf, size_t count, loff_t *offset)
{
	int length, total = 0;
	struct proxyfs_proxy_file_t *proxyfile;
	struct proxyfs_task *task = PROXYFS_TASK( file->f_dentry->d_inode->i_sb->s_fs_info );

	proxyfile = (struct proxyfs_proxy_file_t *) file->private_data;

	if((ssize_t) count < 0) return -EINVAL;

	mdbg(INFO2, "file %p, buf %p, count %lu, offset %llu", proxyfile, buf, (unsigned long)count, (unsigned long long)*offset);
	mdbg(INFO2, "Content %.*s", (int)count, buf);

write:	length = proxyfs_proxy_file_write_user(proxyfile, buf + total, count - total);
	
	mdbg(INFO2, "file %p, written, %d", proxyfile, (int)length);

	if( length > 0 ){
		*offset += length;
		//proxyfs_task_wakeup( task );
		proxyfs_task_notify_data_ready(task);
		total += length;
		if( total == count || file->f_flags & O_NONBLOCK )
			return total;
		else
			goto write;
	}
	else if( length == 0 ){ // Buffer is full
		if( file->f_flags & O_NONBLOCK ) {
			mdbg(INFO2, "Buffer full in non-block mode -> EAGAIN");
			return -EAGAIN;
		}
		else { // Block on file
			mdbg(INFO2, "Buffer full in block mode -> wait");
			switch( proxyfs_proxy_file_wait_interruptible(proxyfile, 
				PROXYFS_FILE_CAN_WRITE | PROXYFS_FILE_EPIPE | PROXYFS_FILE_CLOSED)){				
				case PROXYFS_FILE_EPIPE:
					return -EPIPE;  // This is seen by reading process only if it catches 
							// or ignore SIGPIPE
				case PROXYFS_FILE_CLOSED:
					return -EBADF;
				case PROXYFS_FILE_CAN_WRITE:
					goto write;
				case 0:
					return -ERESTARTSYS;
			}
		}
	}
	else if( length < -4 ){ // Writing to pipe, needs -length bytes.
		if( file->f_flags & O_NONBLOCK ) {
			mdbg(INFO2, "Returning AGAIN");
			return -EAGAIN;
		} else { // Block on file
			int file_state = proxyfs_proxy_file_wait_for_space_interruptible(proxyfile, -length);
			if ( file_state & PROXYFS_FILE_CAN_WRITE ) {
				mdbg(INFO2, "Restarting write");
				goto write;
			}

			switch( file_state){

				case PROXYFS_FILE_EPIPE:
					mdbg(INFO2, "Returning EPIPE");
					return -EPIPE;  // This is seen by reading process only if it catches 
					// or ignore SIGPIPE
				case 0:
					mdbg(INFO2, "Returning ERESTARTSYS");
					return -ERESTARTSYS;
				default:
					mdbg(INFO2, "Unknown file state: %d", file_state);
			}

		}
	}
	return -1;	
}

/** \<\<private\>\> Read function for VFS
 * @param *file   - pointer to file we wanted to read from
 * @param *buf    - pointer to buffer in userspace witch will be filled with data 
 * @param count   - size of buffer
 * @param *offset - pointer to file offset
 *
 * @return Amount of bytes read or error 
 */
static ssize_t proxyfs_fs_fdinode_read(struct file *file, char *buf, size_t count, loff_t *offset)
{
	size_t length;
	struct proxyfs_proxy_file_t *proxyfile;

	proxyfile = (struct proxyfs_proxy_file_t *) file->private_data;

	if((ssize_t) count < 0) return -EINVAL;

	mdbg(INFO2, "file %p, buf %p, count %lu, offset %llu", proxyfile, buf, (unsigned long)count, (unsigned long long)*offset);

read:	length = proxyfs_proxy_file_read_user(proxyfile, buf, count);
	
	mdbg(INFO2, "Read content %.*s", (int)length, buf);

	mdbg(INFO2, "file %p, read, %d", proxyfile, (int)length);

	if( length > 0 ){
		*offset += length;
		return length;
	}
	else if( length == 0 ){ // Buffer is empty
		if( proxyfs_file_get_status( PROXYFS_FILE( proxyfile ), PROXYFS_FILE_READ_EOF ) ){
			mdbg(INFO4, "EOF");
			// The following condition is check for a situation, where we got data + eof after the read was processed
			// In that case, old read may return 0, but now there are data and in addition EOF is set
			if ( proxyfs_file_get_status( PROXYFS_FILE( proxyfile ), PROXYFS_FILE_CAN_READ) ) {
				mdbg(INFO4, "Still can read -> read buffer before eof");
				goto read;
			}
			return 0;	
		}
		if( file->f_flags & O_NONBLOCK )
			return -EAGAIN;
		else { // Block on file 
			mdbg(INFO2, "file %p, waiting for can read", proxyfile);
			switch( proxyfs_proxy_file_wait_interruptible(proxyfile, 
				PROXYFS_FILE_CAN_READ | PROXYFS_FILE_READ_EOF )){
				// Can read shall be checked before EOF. It is possible to have both can_read and EOF up. In that case,
				// we first read and eof after the buffer is empty
				case PROXYFS_FILE_CAN_READ:
					goto read;
				case PROXYFS_FILE_READ_EOF:
					return 0; // End of file
				case 0:
					return -EINTR;
			}
		}
	}
	return length;	
}

// TODO: Do we need setattr support?

/** \<\<private\>\> Getattr function for VFS
 * @param *mnt     - 
 * @param *dentry  -  
 * @param count - size of buffer
 * @param *stat - pointer to kernel stat structure, which will be filled
 *
 * @return getattr return code 
 */
static int proxyfs_fs_fdinode_getattr(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat)
{
	unsigned long i_ino = dentry->d_inode->i_ino;
	int fd = i_ino >> 16;

	mdbg(INFO3, "GETATTR fd: %d; i_ino: %08lx; stat: %p;", fd, i_ino, stat);
	generic_fillattr(dentry->d_inode, stat);

	return 0;
}

/** \<\<private\>\> Ioctl function for VFS
 * @param *file - pointer to file we wanted to call ioctl on
 * @param cmd   - ioctl command
 * @param arg   - ioctl argument
 *
 * @return ioctl return code 
 */
static long proxyfs_fs_fdinode_ioctl (struct file *file, unsigned int cmd, unsigned long arg){
	struct proxyfs_proxy_file_t *proxyfile;
	unsigned long ioctl_info;
	size_t arg_size;

	proxyfile = (struct proxyfs_proxy_file_t *) file->private_data;
	ioctl_info = ioctl_get_info(cmd);
	arg_size = IOCTL_ARG_SIZE(ioctl_info);

	mdbg(INFO3, "IOCTL file: %lu cmd: %u arg: %lx arg_size: %lu", 
			file->f_dentry->d_inode->i_ino, cmd, arg, (unsigned long)arg_size);

	if( ioctl_info )
		return proxyfs_proxy_file_ioctl_user(proxyfile, cmd, arg);
	mdbg(ERR3, "Unknown IOCTL command %x", cmd);
	return -ENOTTY;
}

/**
 * \<\<public\>\> Registr proxyfs in kernel
 *
 * @return zero on succes
 */
int proxyfs_fs_init(void)
{
	return register_filesystem(&proxyfs);
}

/**
 * \<\<public\>\> Unregistr proxyfs in kernel
 *
 */
void proxyfs_fs_exit(void)
{
	unregister_filesystem(&proxyfs);
}

