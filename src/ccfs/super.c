#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/key.h>
#include <linux/seq_file.h>
#include <linux/file.h>
#include <linux/crypto.h>
#include "ccfs.h"

#include <dbg.h>

struct kmem_cache *ccfs_inode_cache;

static struct inode *ccfs_alloc_inode(struct super_block *sb)
{
	struct ccfs_inode *c_inode;
	struct inode *inode = NULL;

	c_inode = kmem_cache_alloc(ccfs_inode_cache, GFP_KERNEL);
	mdbg(INFO3,  "Allocated inode: %p", c_inode);
	if (unlikely(!c_inode))
		goto out;

	mutex_init(&c_inode->lower_file_mutex);
	c_inode->lower_file = NULL;
	inode = &c_inode->vfs_inode;
	c_inode->link = NULL;
	c_inode->stat = NULL;
out:
	return inode;
}

static void ccfs_destroy_inode(struct inode *inode)
{
	struct ccfs_inode *c_inode;

	c_inode = ccfs_inode_to_private(inode);
	mdbg(INFO3,  "Destroying inode: %p Lower file: %p", c_inode, c_inode->lower_file);
	mutex_lock(&c_inode->lower_file_mutex);
	if (c_inode->lower_file) {
		struct dentry *lower_dentry =
			c_inode->lower_file->f_dentry;

		BUG_ON(!lower_dentry);
		mdbg(INFO3,  "Destroying nested dentry: %s", lower_dentry->d_iname);
		if (lower_dentry->d_inode) {
			mdbg(INFO3,  "Puting lower file: %p (%ld)", c_inode->lower_file, atomic_long_read(&c_inode->lower_file->f_count));
			fput(c_inode->lower_file);
			c_inode->lower_file = NULL;
			//d_drop(lower_dentry); This is likely not neccesary, but worse it is a race as the lower dentry is not held at this time
		}

		kfree(c_inode->link);
		kfree(c_inode->stat);
	}
	mutex_unlock(&c_inode->lower_file_mutex);
	kmem_cache_free(ccfs_inode_cache, c_inode);
	mdbg(INFO3, "Destroy done");
}

void ccfs_init_inode(struct inode *inode, struct inode *lower_inode)
{
	mdbg(INFO3, "Initializing inode: %p with lower inode: %p", inode, lower_inode);
	ccfs_set_nested_inode(inode, lower_inode);
	inode->i_ino = lower_inode->i_ino;
	inode->i_version++;
	inode->i_op = &ccfs_main_iops;
	inode->i_fop = &ccfs_main_fops;
	inode->i_mapping->a_ops = &ccfs_aops;
	ccfs_inode_to_private(inode)->cacheable = 1;	
}

static void ccfs_put_super(struct super_block *sb)
{
	struct ccfs_super *sb_info = ccfs_superblock_to_private(sb);

	destroy_filter(sb_info->cacheable_filter);
	kmem_cache_free(ccfs_sb_cache, sb_info);
	ccfs_set_superblock_private(sb, NULL);
}

static int ccfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	return vfs_statfs(ccfs_get_nested_dentry(dentry), buf);
}

static void ccfs_clear_inode(struct inode *inode)
{
	mdbg(INFO3, "Clearing inode: %p", inode);
	iput(ccfs_get_nested_inode(inode));
}

static void ccfs_drop_inode(struct inode *inode)
{
	if ( !ccfs_is_inode_cacheable(inode) ) {
		mdbg(INFO3, "Dropping non-cacheable inode: %p", inode);
		generic_delete_inode(inode);		
	} else {
		generic_drop_inode(inode);		
	} 	
		
}

static int ccfs_show_options(struct seq_file *m, struct vfsmount *mnt)
{
	struct super_block *sb = mnt->mnt_sb;
	struct dentry *lower_root_dentry = ccfs_get_nested_dentry(sb->s_root);
	struct vfsmount *lower_mnt = ccfs_dentry_to_nested_mnt(sb->s_root);
	struct path tmp_path;
	char *tmp_page;
	char *path;
	int rc = 0;

	tmp_path.dentry = lower_root_dentry;
	tmp_path.mnt = lower_mnt;

	tmp_page = (char *)__get_free_page(GFP_KERNEL);
	if (!tmp_page) {
		rc = -ENOMEM;
		goto out;
	}
	path = d_path(&tmp_path, tmp_page, PAGE_SIZE);
	if (IS_ERR(path)) {
		rc = PTR_ERR(path);
		goto out;
	}
	seq_printf(m, ",dir=%s", path);
	free_page((unsigned long)tmp_page);
out:
	return rc;
}

const struct super_operations ccfs_sops = {
	.alloc_inode = ccfs_alloc_inode,
	.destroy_inode = ccfs_destroy_inode,
	.drop_inode = ccfs_drop_inode, // generic_delete_inode,
	.put_super = ccfs_put_super,
	.statfs = ccfs_statfs,
	.remount_fs = NULL,
	.clear_inode = ccfs_clear_inode,
	.show_options = ccfs_show_options
};
