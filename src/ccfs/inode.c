#include <linux/file.h>
#include <linux/vmalloc.h>
#include <linux/pagemap.h>
#include <linux/dcache.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/crypto.h>
#include <linux/fs_stack.h>
#include "ccfs.h"

#include <dbg.h>

static struct dentry *lock_parent(struct dentry *dentry)
{
	struct dentry *dir;

	dir = dget(dentry->d_parent);
	mutex_lock_nested(&(dir->d_inode->i_mutex), I_MUTEX_PARENT);
	return dir;
}

static void unlock_parent(struct dentry *dentry)
{
	mutex_unlock(&(dentry->d_parent->d_inode->i_mutex));
	dput(dentry->d_parent);
}

static void unlock_dir(struct dentry *dir)
{
	mutex_unlock(&dir->d_inode->i_mutex);
	dput(dir);
}

static int
ccfs_create_underlying_file(struct inode *lower_dir_inode,
				struct dentry *dentry, int mode,
				struct nameidata *nd)
{
	struct dentry *lower_dentry = ccfs_get_nested_dentry(dentry);
	struct vfsmount *lower_mnt = ccfs_dentry_to_nested_mnt(dentry);
	struct dentry *dentry_save;
	struct vfsmount *vfsmount_save;
	int rc;

	dentry_save = nd->path.dentry;
	vfsmount_save = nd->path.mnt;
	nd->path.dentry = lower_dentry;
	nd->path.mnt = lower_mnt;
	
	mdbg(INFO3,"Create file w/ lower_dentry->d_name.name = [%s] -> [%s] ", lower_dentry->d_name.name, nd->path.dentry->d_name.name);
	rc = vfs_create(lower_dir_inode, lower_dentry, mode, nd);
	nd->path.dentry = dentry_save;
	nd->path.mnt = vfsmount_save;
	return rc;
}

static int ccfs_do_create(struct inode *directory_inode,
		   struct dentry *ecryptfs_dentry, int mode,
		   struct nameidata *nd)
{
	int rc;
	struct dentry *lower_dentry;
	struct dentry *lower_dir_dentry;

	lower_dentry = ccfs_get_nested_dentry(ecryptfs_dentry);
	lower_dir_dentry = lock_parent(lower_dentry);
	if (unlikely(IS_ERR(lower_dir_dentry))) {
		mdbg(ERR1, "Error locking directory of "
				"dentry");
		rc = PTR_ERR(lower_dir_dentry);
		goto out;
	}
	rc = ccfs_create_underlying_file(lower_dir_dentry->d_inode,
					     ecryptfs_dentry, mode, nd);
	if (unlikely(rc)) {
		mdbg(ERR1,
				"Failure to create underlying file");
		goto out_lock;
	}
	rc = ccfs_interpose(lower_dentry, ecryptfs_dentry,
				directory_inode->i_sb, 0);
	if (rc) {
		mdbg(ERR1, "Failure in ccfs_interpose");
		goto out_lock;
	}
	fsstack_copy_attr_times(directory_inode, lower_dir_dentry->d_inode);
	fsstack_copy_inode_size(directory_inode, lower_dir_dentry->d_inode);
out_lock:
	unlock_dir(lower_dir_dentry);
out:
	return rc;
}

static int ccfs_create(struct inode *directory_inode, struct dentry *ccfs_dentry,
		int mode, struct nameidata *nd)
{
	int rc;

	rc = ccfs_do_create(directory_inode, ccfs_dentry, mode, nd);
	if (unlikely(rc)) {
		printk(KERN_WARNING "Failed to create file in"
				"lower filesystem\n");
		goto out;
	}

	// TODO: Initialize
	//rc = ecryptfs_initialize_file(ecryptfs_dentry);
out:
	return rc;
}

static struct dentry *ccfs_lookup(struct inode *dir, struct dentry *dentry,
				      struct nameidata *nd)
{
	int rc = 0;
	struct dentry *lower_dir_dentry;
	struct dentry *lower_dentry;
	struct vfsmount *lower_mnt;
	struct inode *lower_inode;	
	

	lower_dir_dentry = ccfs_get_nested_dentry(dentry->d_parent);
	dentry->d_op = &ccfs_dops;
	if ((dentry->d_name.len == 1 && !strcmp(dentry->d_name.name, "."))
	    || (dentry->d_name.len == 2
		&& !strcmp(dentry->d_name.name, ".."))) {
		d_drop(dentry);
		goto out;
	}


	mutex_lock(&lower_dir_dentry->d_inode->i_mutex);
	lower_dentry = lookup_one_len(dentry->d_name.name, lower_dir_dentry,
				      dentry->d_name.len);
	mutex_unlock(&lower_dir_dentry->d_inode->i_mutex);

	if (IS_ERR(lower_dentry)) {
		mdbg(INFO3,"Error in lower dentry lookup");
		rc = PTR_ERR(lower_dentry);
		d_drop(dentry);
		goto out;
	}

	lower_mnt = mntget(ccfs_dentry_to_nested_mnt(dentry->d_parent));

	lower_inode = lower_dentry->d_inode;

	mdbg(INFO3,"lower_dentry (lower_inode, dir_inode) = [%p] (%p, %p); lower_dentry->"
       		"d_name.name = [%s]", lower_dentry, lower_inode, dir, 
		lower_dentry->d_name.name);

	fsstack_copy_attr_atime(dir, lower_dir_dentry->d_inode);
	BUG_ON(!atomic_read(&lower_dentry->d_count));

	ccfs_set_dentry_private(dentry,
				    kmem_cache_alloc(ccfs_dentry_cache,
						     GFP_KERNEL));
	if (!ccfs_dentry_to_private(dentry)) {
		rc = -ENOMEM;
		minfo(ERR1, "Out of memory whilst attempting to allocate ccfs_dentry_info struct");
		goto out_dput;
	}
	ccfs_set_nested_dentry(dentry, lower_dentry);
	ccfs_set_dentry_nested_mnt(dentry, lower_mnt);
	if (!lower_dentry->d_inode) {
		/* We want to add because we couldn't find in lower */
		d_add(dentry, NULL);
		goto out;
	}
	rc = ccfs_interpose(lower_dentry, dentry, dir->i_sb, 1);
	if (rc) {
	  	minfo(ERR1, "Error interposing inode: %d", rc);
		goto out_dput;
	}
	if (S_ISDIR(lower_inode->i_mode)) {
		mdbg(INFO3, "Is a directory; returning");
		goto out;
	}
	if (S_ISLNK(lower_inode->i_mode)) {
		mdbg(INFO3, "Is a symlink; returning");
		goto out;
	}
	if (special_file(lower_inode->i_mode)) {
		mdbg(INFO3, "Is a special file; returning");
		goto out;
	}
	if (!nd) {
		mdbg(INFO3,"We have a NULL nd, just leave"
				"as we *think* we are about to unlink");
		goto out;
	}


	goto out;
out_dput:
	mdbg(INFO3,"lookup PUT done with res: %d (dentry inode: %p name: %s)", rc, dentry->d_inode, dentry->d_name.name);
	dput(lower_dentry);
	d_drop(dentry);
	return ERR_PTR(rc);
	
out:
	mdbg(INFO3,"lookup done with res: %d (dentry inode: %p name: %s dir cacheable: %d)", rc, dentry->d_inode, dentry->d_name.name, ccfs_inode_to_private(dir)->cacheable);
	return ERR_PTR(rc);
}

static int ccfs_link(struct dentry *old_dentry, struct inode *dir,
			 struct dentry *new_dentry)
{
	struct dentry *lower_old_dentry;
	struct dentry *lower_new_dentry;
	struct dentry *lower_dir_dentry;
	u64 file_size_save;
	int rc;
	
// TODO: Update cached link
	file_size_save = i_size_read(old_dentry->d_inode);
	lower_old_dentry = ccfs_get_nested_dentry(old_dentry);
	lower_new_dentry = ccfs_get_nested_dentry(new_dentry);	
	dget(lower_old_dentry);
	dget(lower_new_dentry);
	
	mdbg(INFO3,"Link w/ lower_dentry->d_name.name = [%s] Link = [%s]", lower_old_dentry->d_name.name, lower_new_dentry->d_name.name);
	lower_dir_dentry = lock_parent(lower_new_dentry);
	rc = vfs_link(lower_old_dentry, lower_dir_dentry->d_inode,
		      lower_new_dentry);
	if (rc || !lower_new_dentry->d_inode)
		goto out_lock;
	rc = ccfs_interpose(lower_new_dentry, new_dentry, dir->i_sb, 0);
	if (rc)
		goto out_lock;
	fsstack_copy_attr_times(dir, lower_new_dentry->d_inode);
	fsstack_copy_inode_size(dir, lower_new_dentry->d_inode);
	old_dentry->d_inode->i_nlink =
		ccfs_get_nested_inode(old_dentry->d_inode)->i_nlink;
	i_size_write(new_dentry->d_inode, file_size_save);
out_lock:
	unlock_dir(lower_dir_dentry);
	dput(lower_new_dentry);
	dput(lower_old_dentry);
	d_drop(lower_old_dentry);
	d_drop(new_dentry);
	d_drop(old_dentry);
	return rc;
}

static int ccfs_unlink(struct inode *dir, struct dentry *dentry)
{
	int rc = 0;
	struct dentry *lower_dentry = ccfs_get_nested_dentry(dentry);
	struct inode *lower_dir_inode = ccfs_get_nested_inode(dir);
// TODO: Update cached link
	mdbg(INFO3,"Unlink w/ lower_dentry->d_name.name = [%s]", lower_dentry->d_name.name);
	lock_parent(lower_dentry);
	rc = vfs_unlink(lower_dir_inode, lower_dentry);
	if (rc) {
		printk(KERN_ERR "Error in vfs_unlink; rc = [%d]\n", rc);
		goto out_unlock;
	}
	fsstack_copy_attr_times(dir, lower_dir_inode);
	dentry->d_inode->i_nlink =
		ccfs_get_nested_inode(dentry->d_inode)->i_nlink;
	dentry->d_inode->i_ctime = dir->i_ctime;
	d_drop(dentry);
out_unlock:
	unlock_parent(lower_dentry);
	return rc;
}

static int ccfs_symlink(struct inode *dir, struct dentry *dentry,
			    const char *symname)
{
	int rc;
	struct dentry *lower_dentry;
	struct dentry *lower_dir_dentry;
	umode_t mode;
// TODO: Update cached link
	lower_dentry = ccfs_get_nested_dentry(dentry);		
	dget(lower_dentry);
	
	mdbg(INFO3,"Symlink w/ lower_dentry->d_name.name = [%s] Link = [%s]", lower_dentry->d_name.name, symname);
	lower_dir_dentry = lock_parent(lower_dentry);
	mode = S_IALLUGO;

	rc = vfs_symlink(lower_dir_dentry->d_inode, lower_dentry,
			 symname);
	
	if (rc || !lower_dentry->d_inode)
		goto out_lock;
	rc = ccfs_interpose(lower_dentry, dentry, dir->i_sb, 0);
	if (rc)
		goto out_lock;
	fsstack_copy_attr_times(dir, lower_dir_dentry->d_inode);
	fsstack_copy_inode_size(dir, lower_dir_dentry->d_inode);
out_lock:
	unlock_dir(lower_dir_dentry);
	dput(lower_dentry);
	if (!dentry->d_inode)
		d_drop(dentry);
	return rc;
}

static int ccfs_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
	int rc;
	struct dentry *lower_dentry;
	struct dentry *lower_dir_dentry;

	lower_dentry = ccfs_get_nested_dentry(dentry);
	lower_dir_dentry = lock_parent(lower_dentry);
	
	mdbg(INFO3,"MKDIR w/ lower_dentry->d_name.name = [%s] DIR = [%s]", lower_dentry->d_name.name, lower_dir_dentry->d_name.name);
	rc = vfs_mkdir(lower_dir_dentry->d_inode, lower_dentry, mode);
	if (rc || !lower_dentry->d_inode)
		goto out;
	rc = ccfs_interpose(lower_dentry, dentry, dir->i_sb, 0);
	if (rc)
		goto out;
	fsstack_copy_attr_times(dir, lower_dir_dentry->d_inode);
	fsstack_copy_inode_size(dir, lower_dir_dentry->d_inode);
	dir->i_nlink = lower_dir_dentry->d_inode->i_nlink;
out:
	unlock_dir(lower_dir_dentry);
	if (!dentry->d_inode)
		d_drop(dentry);
	return rc;
}

static int ccfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	struct dentry *lower_dentry;
	struct dentry *lower_dir_dentry;
	int rc;

	lower_dentry = ccfs_get_nested_dentry(dentry);
	
	mdbg(INFO3,"RMDIR w/ lower_dentry->d_name.name = [%s]", lower_dentry->d_name.name);
	
	dget(dentry);
	lower_dir_dentry = lock_parent(lower_dentry);
	dget(lower_dentry);
	rc = vfs_rmdir(lower_dir_dentry->d_inode, lower_dentry);
	dput(lower_dentry);
	if (!rc)
		d_delete(lower_dentry);
	fsstack_copy_attr_times(dir, lower_dir_dentry->d_inode);
	dir->i_nlink = lower_dir_dentry->d_inode->i_nlink;
	unlock_dir(lower_dir_dentry);
	if (!rc)
		d_drop(dentry);
	dput(dentry);
	return rc;
}

static int ccfs_mknod(struct inode *dir, struct dentry *dentry, int mode, dev_t dev)
{
	int rc;
	struct dentry *lower_dentry;
	struct dentry *lower_dir_dentry;
		
	lower_dentry = ccfs_get_nested_dentry(dentry);
	lower_dir_dentry = lock_parent(lower_dentry);
	
	mdbg(INFO3,"MKNOD w/ lower_dentry->d_name.name = [%s]", lower_dentry->d_name.name);
	
	rc = vfs_mknod(lower_dir_dentry->d_inode, lower_dentry, mode, dev);
	if (rc || !lower_dentry->d_inode)
		goto out;
	rc = ccfs_interpose(lower_dentry, dentry, dir->i_sb, 0);
	if (rc)
		goto out;
	fsstack_copy_attr_times(dir, lower_dir_dentry->d_inode);
	fsstack_copy_inode_size(dir, lower_dir_dentry->d_inode);
out:
	unlock_dir(lower_dir_dentry);
	if (!dentry->d_inode)
		d_drop(dentry);
	return rc;
}

static int ccfs_rename(struct inode *old_dir, struct dentry *old_dentry,
		struct inode *new_dir, struct dentry *new_dentry)
{
	int rc;
	struct dentry *lower_old_dentry;
	struct dentry *lower_new_dentry;
	struct dentry *lower_old_dir_dentry;
	struct dentry *lower_new_dir_dentry;

	lower_old_dentry = ccfs_get_nested_dentry(old_dentry);
	lower_new_dentry = ccfs_get_nested_dentry(new_dentry);
	dget(lower_old_dentry);
	dget(lower_new_dentry);
	lower_old_dir_dentry = dget_parent(lower_old_dentry);
	lower_new_dir_dentry = dget_parent(lower_new_dentry);
	lock_rename(lower_old_dir_dentry, lower_new_dir_dentry);
	rc = vfs_rename(lower_old_dir_dentry->d_inode, lower_old_dentry,
			lower_new_dir_dentry->d_inode, lower_new_dentry);
	if (rc)
		goto out_lock;
	fsstack_copy_attr_all(new_dir, lower_new_dir_dentry->d_inode);
	if (new_dir != old_dir)
		fsstack_copy_attr_all(old_dir, lower_old_dir_dentry->d_inode);
out_lock:
	unlock_rename(lower_old_dir_dentry, lower_new_dir_dentry);
	dput(lower_new_dentry->d_parent);
	dput(lower_old_dentry->d_parent);
	dput(lower_new_dentry);
	dput(lower_old_dentry);
	return rc;
}

static int ccfs_readlink(struct dentry *dentry, char __user * buf, int bufsiz)
{
	int rc = 0;
	struct dentry *lower_dentry;
	char *lower_buf;
	mm_segment_t old_fs;

	struct ccfs_inode* inode = ccfs_inode_to_private(dentry->d_inode);


	lower_dentry = ccfs_get_nested_dentry(dentry);
	if (!lower_dentry->d_inode->i_op ||
	    !lower_dentry->d_inode->i_op->readlink) {
		rc = -EINVAL;
		goto out;
	}

	// Do we have link cached?
	if ( inode->link ) {
		rc = strlen(inode->link);
		if (copy_to_user(buf, inode->link, rc)) {
			rc = -EFAULT;
		}
		fsstack_copy_attr_atime(dentry->d_inode,
					lower_dentry->d_inode);

		mdbg(INFO3, "Resolved cached link => [%s:%d]", inode->link, rc);

		goto out;
	}


	/* Released in this function */
	lower_buf = kmalloc(bufsiz, GFP_KERNEL);
	if (lower_buf == NULL) {
		printk(KERN_ERR "Out of memory\n");
		rc = -ENOMEM;
		goto out;
	}

	old_fs = get_fs();
	set_fs(get_ds());
	mdbg(INFO3,"Calling readlink w/ "
			"lower_dentry->d_name.name = [%s]",
			lower_dentry->d_name.name);
	rc = lower_dentry->d_inode->i_op->readlink(lower_dentry,
						   (char __user *)lower_buf,
						   bufsiz);
	set_fs(old_fs);
	if (rc >= 0) {	
		// Cache symlinks
		if ( !inode->link ) {
			char* link = kmalloc(rc + 1, GFP_KERNEL);			
			if ( link ) { // Ignore no memory.. we simply do not cache in that case
				memcpy(link, lower_buf, rc);
				inode->link = link;
				inode->link[rc] = '\0';
			}
		}

		if (copy_to_user(buf, lower_buf, rc)) {
			rc = -EFAULT;
		}
		fsstack_copy_attr_atime(dentry->d_inode,
					lower_dentry->d_inode);
	}

	kfree(lower_buf);
out:
	return rc;
}

static void *ccfs_follow_link(struct dentry *dentry, struct nameidata *nd)
{
	char *buf;
	int len = PAGE_SIZE, rc;
	mm_segment_t old_fs;

	/* Released in ecryptfs_put_link(); only release here on error */
	buf = kmalloc(len, GFP_KERNEL);
	if (!buf) {
		rc = -ENOMEM;
		goto out;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	mdbg(INFO3, "Calling followlink w/ "
			"dentry->d_name.name = [%s]", dentry->d_name.name);
	rc = dentry->d_inode->i_op->readlink(dentry, (char __user *)buf, len);
	buf[rc] = '\0';
	set_fs(old_fs);
	if (rc < 0)
		goto out_free;
	rc = 0;
	nd_set_link(nd, buf);
	goto out;
out_free:
	kfree(buf);
out:
	return ERR_PTR(rc);
}

static void ccfs_put_link(struct dentry *dentry, struct nameidata *nd, void *ptr)
{
	/* Free the char* */
	kfree(nd_get_link(nd));
}


int ccfs_truncate(struct dentry *dentry, loff_t new_length)
{
	int rc = 0;
	struct inode *inode = dentry->d_inode;
	struct dentry *lower_dentry;
	loff_t i_size = i_size_read(inode);

	if (unlikely((new_length == i_size)))
		goto out;

	lower_dentry = ccfs_get_nested_dentry(dentry);

	vmtruncate(inode, new_length);
	vmtruncate(lower_dentry->d_inode,
			new_length);
out:
	return rc;
}

static int ccfs_permission(struct inode *inode, int mask)
{
	int rc = inode_permission(ccfs_get_nested_inode(inode), mask);

	mdbg(INFO3,"Permission check for inode %p [%ld] returned [%d]", inode, inode->i_ino, rc);

	return rc;
}

static int ccfs_setattr(struct dentry *dentry, struct iattr *ia)
{
	int rc = 0;
	struct dentry *lower_dentry;
	struct inode *inode;
	struct inode *lower_inode;

	inode = dentry->d_inode;
	lower_inode = ccfs_get_nested_inode(inode);
	lower_dentry = ccfs_get_nested_dentry(dentry);
	
	if (ia->ia_valid & ATTR_SIZE) {
		mdbg(INFO3,
				"ia->ia_valid = [0x%x] ATTR_SIZE" " = [0x%x]",
				ia->ia_valid, ATTR_SIZE);
		rc = ccfs_truncate(dentry, ia->ia_size);
		ia->ia_valid &= ~ATTR_SIZE;
		mdbg(INFO3,"ia->ia_valid = [%x]",
				ia->ia_valid);
		if (rc < 0)
			goto out;
	}

	/*
	 * mode change is for clearing setuid/setgid bits. Allow lower fs
	 * to interpret this in its own way.
	 */
	if (ia->ia_valid & (ATTR_KILL_SUID | ATTR_KILL_SGID))
		ia->ia_valid &= ~ATTR_MODE;

	rc = notify_change(lower_dentry, ia);
out:
	fsstack_copy_attr_all(inode, lower_inode);
	return rc;
}

static int ccfs_getattr(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat)
{
	int rc = 0;
	struct dentry *lower_dentry;	
	struct ccfs_inode* inode = ccfs_inode_to_private(dentry->d_inode);
	struct vfsmount *lower_mnt = ccfs_dentry_to_nested_mnt(dentry);
	lower_dentry = ccfs_get_nested_dentry(dentry);

	if ( inode->stat ) {
		*stat = *inode->stat;
		return 0;
	}

	mutex_lock(&lower_dentry->d_inode->i_mutex);
	rc = vfs_getattr(lower_mnt, lower_dentry, stat);
	mutex_unlock(&lower_dentry->d_inode->i_mutex);

	if ( !rc && inode->cacheable) {
		inode->stat = kmalloc(sizeof(struct kstat), GFP_KERNEL);
		if ( inode->stat ) {
			*(inode->stat) = *stat;
		}
	}

	mdbg(INFO3, "Get stat returned res: %d uid %d gid %d", rc, stat->uid, stat->gid);

	return rc;

}

int ccfs_setxattr(struct dentry *dentry, const char *name, const void *value,
		  size_t size, int flags)
{
	int rc = 0;
	struct dentry *lower_dentry;

	lower_dentry = ccfs_get_nested_dentry(dentry);
	if (!lower_dentry->d_inode->i_op->setxattr) {
		rc = -EOPNOTSUPP;
		goto out;
	}
	mutex_lock(&lower_dentry->d_inode->i_mutex);
	rc = lower_dentry->d_inode->i_op->setxattr(lower_dentry, name, value,
						   size, flags);
	mutex_unlock(&lower_dentry->d_inode->i_mutex);
out:
	return rc;
}

ssize_t
ccfs_getxattr_lower(struct dentry *lower_dentry, const char *name,
			void *value, size_t size)
{
	int rc = 0;

	if (!lower_dentry->d_inode->i_op->getxattr) {
		rc = -EOPNOTSUPP;
		goto out;
	}
	mutex_lock(&lower_dentry->d_inode->i_mutex);
	rc = lower_dentry->d_inode->i_op->getxattr(lower_dentry, name, value,
						   size);
	mutex_unlock(&lower_dentry->d_inode->i_mutex);
out:
	return rc;
}

ssize_t
ccfs_getxattr(struct dentry *dentry, const char *name, void *value,
		  size_t size)
{
	return ccfs_getxattr_lower(ccfs_get_nested_dentry(dentry), name,
				       value, size);
}

static ssize_t
ccfs_listxattr(struct dentry *dentry, char *list, size_t size)
{
	int rc = 0;
	struct dentry *lower_dentry;

	lower_dentry = ccfs_get_nested_dentry(dentry);
	if (!lower_dentry->d_inode->i_op->listxattr) {
		rc = -EOPNOTSUPP;
		goto out;
	}
	mutex_lock(&lower_dentry->d_inode->i_mutex);
	rc = lower_dentry->d_inode->i_op->listxattr(lower_dentry, list, size);
	mutex_unlock(&lower_dentry->d_inode->i_mutex);
out:
	return rc;
}

static int ccfs_removexattr(struct dentry *dentry, const char *name)
{
	int rc = 0;
	struct dentry *lower_dentry;

	lower_dentry = ccfs_get_nested_dentry(dentry);
	if (!lower_dentry->d_inode->i_op->removexattr) {
		rc = -EOPNOTSUPP;
		goto out;
	}
	mutex_lock(&lower_dentry->d_inode->i_mutex);
	rc = lower_dentry->d_inode->i_op->removexattr(lower_dentry, name);
	mutex_unlock(&lower_dentry->d_inode->i_mutex);
out:
	return rc;
}

int ccfs_inode_test(struct inode *inode, void *candidate_lower_inode)
{
	if ((ccfs_get_nested_inode(inode)
	     == (struct inode *)candidate_lower_inode)) {
		mdbg(INFO3,"Search matched: %p -> %p", inode, candidate_lower_inode);
		return 1;
	} else {
		return 0;
	}
}

int ccfs_inode_set(struct inode *inode, void *lower_inode)
{
	ccfs_init_inode(inode, (struct inode *)lower_inode);
	return 0;
}



const struct inode_operations ccfs_symlink_iops = {
	.readlink = ccfs_readlink,
	.follow_link = ccfs_follow_link,
	.put_link = ccfs_put_link,
	.permission = ccfs_permission,
	.setattr = ccfs_setattr,
	.getattr = ccfs_getattr,
	.setxattr = ccfs_setxattr,
	.getxattr = ccfs_getxattr,
	.listxattr = ccfs_listxattr,
	.removexattr = ccfs_removexattr
};

const struct inode_operations ccfs_dir_iops = {
	.create = ccfs_create,
	.lookup = ccfs_lookup,
	.link = ccfs_link,
	.unlink = ccfs_unlink,
	.symlink = ccfs_symlink,
	.mkdir = ccfs_mkdir,
	.rmdir = ccfs_rmdir,
	.mknod = ccfs_mknod,
	.rename = ccfs_rename,
	.permission = ccfs_permission,
	.setattr = ccfs_setattr,
	.getattr = ccfs_getattr,
	.setxattr = ccfs_setxattr,
	.getxattr = ccfs_getxattr,
	.listxattr = ccfs_listxattr,
	.removexattr = ccfs_removexattr
};

const struct inode_operations ccfs_main_iops = {
	.permission = ccfs_permission,
	.setattr = ccfs_setattr,
	.getattr = ccfs_getattr,
	.setxattr = ccfs_setxattr,
	.getxattr = ccfs_getxattr,
	.listxattr = ccfs_listxattr,
	.removexattr = ccfs_removexattr
};
