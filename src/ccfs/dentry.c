#include <linux/dcache.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/fs_stack.h>
#include "ccfs.h"

#include <dbg.h>

static int ccfs_d_revalidate(struct dentry *dentry, struct nameidata *nd)
{
	struct dentry *lower_dentry = ccfs_get_nested_dentry(dentry);
	struct vfsmount *lower_mnt = ccfs_dentry_to_nested_mnt(dentry);
	struct dentry *dentry_save;
	struct vfsmount *vfsmount_save;
	int rc = 1;	

	mdbg(INFO3,"Revalidating dentry: (%s) %p", dentry->d_iname, dentry);
	
	// Do not cache negative dentries or uncachable dentries
	if ( !dentry->d_inode ) {		
		mdbg(INFO3,"Inodeless dentry -> invalid");
		return 0;
	}
	

	if (!lower_dentry->d_op || !lower_dentry->d_op->d_revalidate)
		goto out;

	dentry_save = nd->path.dentry;
	vfsmount_save = nd->path.mnt;
	nd->path.dentry = lower_dentry;
	nd->path.mnt = lower_mnt;
	rc = lower_dentry->d_op->d_revalidate(lower_dentry, nd);
	nd->path.dentry = dentry_save;
	nd->path.mnt = vfsmount_save;
	if (dentry->d_inode) {
		struct inode *lower_inode =
			ccfs_get_nested_inode(dentry->d_inode);

		fsstack_copy_attr_all(dentry->d_inode, lower_inode);
	}
out:

	
/*      
    No special handling of non-cacheable nodes.. if the dentry is still in cache, we just let lower FS to revalidate the dentry. If we return 0 here, some operations may break as some
    FS actions would failed even though they are valid.
    if ( !ccfs_inode_to_private(dentry->d_inode)->cacheable ) {
	    mdbg(INFO3,"Non-cacheable inode -> invalid");
	    return 0;
	}
*/	
		
	mdbg(INFO3,"Revalidation result: %d", rc);
	
	return rc;
}

struct kmem_cache *ccfs_dentry_cache;

static void ccfs_d_release(struct dentry *dentry)
{
	mdbg(INFO3,"Releasing dentry: (%s) %p", dentry->d_iname, dentry);
	if (ccfs_dentry_to_private(dentry)) {		
		if (ccfs_get_nested_dentry(dentry)) {
			mntput(ccfs_dentry_to_nested_mnt(dentry));
			dput(ccfs_get_nested_dentry(dentry));
		}
		kmem_cache_free(ccfs_dentry_cache,
				ccfs_dentry_to_private(dentry));
	}
	mdbg(INFO3,"Releasing dentry done");
	return;
}

static int ccfs_d_delete(struct dentry *dentry)
{
	
	if (dentry->d_inode) {
		mdbg(INFO3,"Delete dentry: (%s) %d", dentry->d_iname, ccfs_is_inode_cacheable(dentry->d_inode));
		return !ccfs_is_inode_cacheable(dentry->d_inode);
	}

	return 1;
}


struct dentry_operations ccfs_dops = {
	.d_delete = ccfs_d_delete,
	.d_revalidate = ccfs_d_revalidate,
	.d_release = ccfs_d_release,
};
