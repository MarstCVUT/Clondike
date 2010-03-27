#ifndef CCFS_H
#define CCFS_H

#include <linux/fs.h>
#include <linux/fs_stack.h>
#include <linux/namei.h>
#include <linux/scatterlist.h>
#include <linux/hash.h>

#include "filter.h"

/** Super block */
struct ccfs_super {
	struct super_block* nested_superblock;
	/** Is aggressive file prefech enabled? */
	int prefetch;
	/** Filter of cacheable files. If null, all files are cacheable */
	struct filter* cacheable_filter;
};

/** File struct */
struct ccfs_file {
	/** File in the backing store */
	struct file* nested_file;
};

struct ccfs_inode {
	/** This inode data */
	struct inode vfs_inode;
	/** Reference to nested inode */
	struct inode *nested_inode;
	struct file *lower_file;
	struct mutex lower_file_mutex;	
	/** Is this inode cacheable? */
	int cacheable;
	/** Cached link pointer of this inode (in case it is a symlink) */
	char* link;
	/** Cached stats of file */
	struct kstat* stat;
};

/** Dentry */
struct ccfs_dentry {
	/** Lower dentry & its path */
	struct path lower_path;
};

/*********** File ops *****************/
static inline void ccfs_set_nested_file(struct file* ccfsfile, struct file* nested_file) {
	((struct ccfs_file*)ccfsfile->private_data)->nested_file = nested_file;
}

static inline struct file* ccfs_get_nested_file(struct file* ccfsfile) {
	return ((struct ccfs_file*)ccfsfile->private_data)->nested_file;
}

static inline void ccfs_set_file_private(struct file* ccfsfile, struct ccfs_file* priv) {
	ccfsfile->private_data = priv;
}

static inline struct ccfs_file* ccfs_get_file_private(struct file* ccfsfile) {
	return (struct ccfs_file*)ccfsfile->private_data;
}

/*********** Dentry ops *******************/
static inline struct ccfs_dentry *
ccfs_dentry_to_private(struct dentry *dentry)
{
	return (struct ccfs_dentry *)dentry->d_fsdata;
}

static inline void
ccfs_set_dentry_private(struct dentry *dentry,
			    struct ccfs_dentry *dentry_priv)
{
	dentry->d_fsdata = dentry_priv;
}

static inline struct dentry *
ccfs_get_nested_dentry(struct dentry *dentry)
{
	return ((struct ccfs_dentry *)dentry->d_fsdata)->lower_path.dentry;
}

static inline void
ccfs_set_nested_dentry(struct dentry *dentry, struct dentry *lower_dentry)
{
	((struct ccfs_dentry *)dentry->d_fsdata)->lower_path.dentry =
		lower_dentry;
}

static inline struct vfsmount *
ccfs_dentry_to_nested_mnt(struct dentry *dentry)
{
	return ((struct ccfs_dentry *)dentry->d_fsdata)->lower_path.mnt;
}

static inline void
ccfs_set_dentry_nested_mnt(struct dentry *dentry, struct vfsmount *lower_mnt)
{
	((struct ccfs_dentry *)dentry->d_fsdata)->lower_path.mnt =
		lower_mnt;
}

/************* Inode ops ****************/
static inline struct ccfs_inode * ccfs_inode_to_private(struct inode *inode)
{
	return container_of(inode, struct ccfs_inode, vfs_inode);
}

static inline struct inode *ccfs_get_nested_inode(struct inode *inode)
{
	return ccfs_inode_to_private(inode)->nested_inode;
}

static inline void
ccfs_set_nested_inode(struct inode *inode, struct inode *lower_inode)
{
	ccfs_inode_to_private(inode)->nested_inode = lower_inode;
}

static inline int ccfs_is_inode_cacheable(struct inode* inode) {
	return ccfs_inode_to_private(inode)->cacheable;
}


/************ Superblock ops ************/

static inline struct ccfs_super * ccfs_superblock_to_private(struct super_block *sb)
{
	return (struct ccfs_super *)sb->s_fs_info;
}

static inline void ccfs_set_superblock_private(struct super_block *sb,
				struct ccfs_super *sb_info)
{
	sb->s_fs_info = sb_info;
}

static inline struct super_block * ccfs_superblock_to_lower(struct super_block *sb)
{
	return ((struct ccfs_super *)sb->s_fs_info)->nested_superblock;
}

static inline void ccfs_set_superblock_lower(struct super_block *sb,
			      struct super_block *lower_sb)
{
	((struct ccfs_super *)sb->s_fs_info)->nested_superblock = lower_sb;
}

/************* VFS ops *************************/
extern const struct file_operations ccfs_main_fops;
extern const struct file_operations ccfs_dir_fops;
extern const struct inode_operations ccfs_main_iops;
extern const struct inode_operations ccfs_dir_iops;
extern const struct inode_operations ccfs_symlink_iops;
extern const struct super_operations ccfs_sops;
extern struct dentry_operations ccfs_dops;
extern struct address_space_operations ccfs_aops;

/************** Caches *************************/
extern struct kmem_cache *ccfs_file_cache;
extern struct kmem_cache *ccfs_dentry_cache;
extern struct kmem_cache *ccfs_inode_cache;
extern struct kmem_cache *ccfs_sb_cache;
extern struct kmem_cache *ccfs_lower_page_cache;



/********* General funcs ***********************/
int ccfs_interpose(struct dentry *lower_dentry, struct dentry *dentry,
		       struct super_block *sb, int flag);

int ccfs_reopen_persistent_file(struct dentry *ccfsdentry, struct inode* ccfsinode);

void ccfs_init_inode(struct inode *inode, struct inode *lower_inode);

int ccfs_read_lower_page_segment(struct page *page_for_ccfs,
				     pgoff_t page_index,
				     size_t offset_in_page, size_t size,
				     struct inode *ccfsinode);

int ccfs_write_lower_page_segment(struct inode *ccfsinode,
				      struct page *page_for_lower,
				      size_t offset_in_page, size_t size);

int ccfs_inode_test(struct inode *inode, void *candidate_lower_inode);

int ccfs_inode_set(struct inode *inode, void *lower_inode);
#endif
