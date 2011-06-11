#include <linux/dcache.h>
#include <linux/file.h>
#include <linux/module.h>
#include <linux/namei.h>
#include <linux/skbuff.h>
#include <linux/crypto.h>
#include <linux/netlink.h>
#include <linux/mount.h>
#include <linux/pagemap.h>
#include <linux/key.h>
#include <linux/parser.h>
#include <linux/fs_stack.h>
#include "ccfs.h"
#include "prefetcher.h"

#include <dbg.h>

static int ccfs_init_persistent_file_unlocked(struct dentry *ccfsdentry, struct inode* ccfsinode) {
  	struct ccfs_inode *inode_info =
		ccfs_inode_to_private(ccfsinode);
	int rc = 0;

  	if (!inode_info->lower_file) {
		struct dentry *lower_dentry;
		struct vfsmount *lower_mnt =
			ccfs_dentry_to_nested_mnt(ccfsdentry);
		int try_write_mode = 0;

		lower_dentry = ccfs_get_nested_dentry(ccfsdentry);

		dget(lower_dentry);
		mntget(lower_mnt);

		// TODO: Write mode is now attempted only for non-cacheable files
		try_write_mode = !ccfs_is_inode_cacheable(ccfsinode);
		inode_info->lower_file = dentry_open(lower_dentry,
						     lower_mnt,
						     try_write_mode ? (O_RDWR | O_LARGEFILE) : (O_RDONLY | O_LARGEFILE),
						     current_cred()		
						     );
		if (IS_ERR(inode_info->lower_file)) {
			mdbg(INFO3, "Error opening lower persistent file for lower_dentry [0x%p] and lower_mnt [0x%p] in write mode. Will try read only mode.",
			       lower_dentry, lower_mnt);
			dget(lower_dentry);
			mntget(lower_mnt);
			inode_info->lower_file = dentry_open(lower_dentry,
							     lower_mnt,
							     (O_RDONLY
							      | O_LARGEFILE), current_cred());
		}
		mdbg(INFO3, "Inode %p (%p) initialized lower file: %p (%ld) - write mode: %d", ccfsdentry->d_inode, ccfsinode, inode_info->lower_file, atomic_long_read(&inode_info->lower_file->f_count), try_write_mode);
		if (IS_ERR(inode_info->lower_file)) {
			mdbg(INFO3, "Error opening lower persistent file for lower_dentry [0x%p] and lower_mnt [0x%p]",
			       lower_dentry, lower_mnt);
			rc = PTR_ERR(inode_info->lower_file);
			inode_info->lower_file = NULL;
		}
	} else {
		mdbg(INFO3, "Inode %p (%p) was already initialized initialized lower file: %p (%ld)", ccfsdentry->d_inode, ccfsinode, inode_info->lower_file, atomic_long_read(&inode_info->lower_file->f_count));
	}
	
	return rc;
}

static int ccfs_init_persistent_file(struct dentry *ccfsdentry, struct inode* ccfsinode)
{
	struct ccfs_inode *inode_info =
		ccfs_inode_to_private(ccfsinode);
	int rc = 0;

	mutex_lock(&inode_info->lower_file_mutex);
	rc = ccfs_init_persistent_file_unlocked(ccfsdentry, ccfsinode);
	mutex_unlock(&inode_info->lower_file_mutex);
	return rc;
}

int ccfs_reopen_persistent_file(struct dentry *ccfsdentry, struct inode* ccfsinode) {
	struct ccfs_inode *inode_info;
	int rc = 0;

	mdbg(INFO3, "Inode %p reopen requested", ccfsdentry->d_inode);

	if ( !ccfsdentry || !ccfsinode )
		return -EINVAL;

	inode_info = ccfs_inode_to_private(ccfsinode);

	mdbg(INFO3, "Inode %p (%p) with lower file: %p being reopened", ccfsdentry->d_inode, ccfsinode, inode_info->lower_file);

	mutex_lock(&inode_info->lower_file_mutex);
	if ( inode_info->lower_file )	{
		mdbg(INFO3, "Inode %p file release (%p -> %ld)", ccfsdentry->d_inode, inode_info->lower_file, atomic_long_read(&inode_info->lower_file->f_count));
		fput(inode_info->lower_file);		
		mdbg(INFO3, "Inode file release done");
		inode_info->lower_file = NULL;		
	}
	
	rc = ccfs_init_persistent_file_unlocked(ccfsdentry, ccfsinode);
	mutex_unlock(&inode_info->lower_file_mutex);

	return rc;
}

int ccfs_interpose(struct dentry *lower_dentry, struct dentry *dentry,
		       struct super_block *sb, int flag)
{
	struct inode *lower_inode;
	struct inode *inode;
	int rc = 0;
	int matches;
	struct filter* filter;

	mdbg(INFO3, "Inode %p being interposed", dentry->d_inode);

	lower_inode = lower_dentry->d_inode;
	if (lower_inode->i_sb != ccfs_superblock_to_lower(sb)) {
		mdbg(ERR3, "Dentry %s EXDEV", dentry->d_name.name);
		rc = -EXDEV;
		goto out;
	}
	if (!igrab(lower_inode)) {
		mdbg(ERR3, "Dentry %s STALE", dentry->d_name.name);
		rc = -ESTALE;
		goto out;
	}
	inode = iget5_locked(sb, (unsigned long)lower_inode,
			     ccfs_inode_test, ccfs_inode_set,
			     lower_inode);
	if (!inode) {
	      mdbg(ERR3, "Dentry %s No access", dentry->d_name.name);
		rc = -EACCES;
		iput(lower_inode);
		goto out;
	}

	if (S_ISLNK(lower_inode->i_mode))
		inode->i_op = &ccfs_symlink_iops;
	else if (S_ISDIR(lower_inode->i_mode))
		inode->i_op = &ccfs_dir_iops;
	if (S_ISDIR(lower_inode->i_mode))
		inode->i_fop = &ccfs_dir_fops;
	if (special_file(lower_inode->i_mode))
		init_special_inode(inode, lower_inode->i_mode,
				   lower_inode->i_rdev);
	dentry->d_op = &ccfs_dops;
	fsstack_copy_attr_all(inode, lower_inode);
	fsstack_copy_inode_size(inode, lower_inode);

	inode->i_uid = lower_inode->i_uid;
	inode->i_gid = lower_inode->i_gid;

	// TODO: Only first time!
	filter = ccfs_superblock_to_private(inode->i_sb)->cacheable_filter;	
	matches = filter_matches(filter, dentry->d_name.name, dentry->d_name.len, &inode->i_ctime, inode->i_mode);
	if ( !matches )
		ccfs_inode_to_private(inode)->cacheable = 0;	
	
	mdbg(INFO3, "Dentry %s matches pattern: %d. Cacheable: %d", dentry->d_name.name, matches, ccfs_inode_to_private(inode)->cacheable);

	
	rc = ccfs_init_persistent_file(dentry, inode);

	// Must be done AFTER the inner file is initialized! 
	if (flag)
		d_add(dentry, inode);
	else
		d_instantiate(dentry, inode);

	if (rc) {
		mdbg(INFO3, "%s: Error attempting to initialize the "
		       "persistent file for the dentry with name [%s]; "
		       "rc = [%d]", __FUNCTION__, dentry->d_name.name, rc);
		goto out; // TODO: Release something?
	}

	// Must be done AFTER the inner file is initialized! 
	if (inode->i_state & I_NEW)
		unlock_new_inode(inode);
	else 
		iput(lower_inode); // TODO: Comment why release only on new
	
out:
	return rc;
}

struct kmem_cache *ccfs_sb_cache;

static int ccfs_fill_super(struct super_block *sb, void *raw_data, int silent)
{
	int rc = 0;

	ccfs_set_superblock_private(sb,
					kmem_cache_zalloc(ccfs_sb_cache,
							 GFP_KERNEL));
	if (!ccfs_superblock_to_private(sb)) {
		printk(KERN_WARNING "Out of memory\n");
		rc = -ENOMEM;
		goto out;
	}
	sb->s_op = &ccfs_sops;
	/* Released through deactivate_super(sb) from get_sb_nodev */
	sb->s_root = d_alloc(NULL, &(const struct qstr) {
			     .hash = 0,.name = "/",.len = 1});
	if (!sb->s_root) {
		printk(KERN_ERR "d_alloc failed\n");
		rc = -ENOMEM;
		goto out;
	}
	sb->s_root->d_op = &ccfs_dops;
	sb->s_root->d_sb = sb;
	sb->s_root->d_parent = sb->s_root;
	/* Released in d_release when dput(sb->s_root) is called */
	/* through deactivate_super(sb) from get_sb_nodev() */
	ccfs_set_dentry_private(sb->s_root,
				    kmem_cache_zalloc(ccfs_dentry_cache,
						     GFP_KERNEL));
	if (!ccfs_dentry_to_private(sb->s_root)) {
		printk(KERN_ERR
				"dentry_info_cache alloc failed\n");
		rc = -ENOMEM;
		goto out;
	}
	rc = 0;
out:
	/* Should be able to rely on deactivate_super called from
	 * get_sb_nodev */
	return rc;
}

static int ccfs_read_super(struct super_block *sb, const char *dev_name)
{
	int rc;
	struct nameidata nd;
	struct dentry *lower_root;
	struct vfsmount *lower_mnt;

	memset(&nd, 0, sizeof(struct nameidata));
	rc = path_lookup(dev_name, LOOKUP_FOLLOW | LOOKUP_DIRECTORY, &nd);
	if (rc) {
		printk(KERN_WARNING "path_lookup() failed\n");
		goto out;
	}
	lower_root = nd.path.dentry;
	lower_mnt = nd.path.mnt;
	ccfs_set_superblock_lower(sb, lower_root->d_sb);
	sb->s_maxbytes = lower_root->d_sb->s_maxbytes;
	sb->s_blocksize = lower_root->d_sb->s_blocksize;
	ccfs_set_nested_dentry(sb->s_root, lower_root);
	ccfs_set_dentry_nested_mnt(sb->s_root, lower_mnt);
	rc = ccfs_interpose(lower_root, sb->s_root, sb, 0);
	if (rc)
		goto out_free;
	rc = 0;
	goto out;
out_free:
	path_put(&nd.path);
out:
	return rc;
}

enum { ccfs_opt_prefetch, ccfs_opt_cacheable_filter, ccfs_opt_err };

static match_table_t tokens = {
	{ccfs_opt_prefetch, "prefetch"},
	{ccfs_opt_cacheable_filter, "cache_filter=%s"},
	{ccfs_opt_err, NULL},
};

static int ccfs_parse_options(struct super_block *sb, char *options) {
	int rc = 0;
	substring_t args[MAX_OPT_ARGS];
	int token;
	char* p;
	char tmp[1000];
	struct ccfs_super* ccfssuper = ccfs_superblock_to_private(sb);

	ccfssuper->prefetch = 0;
	ccfssuper->cacheable_filter = NULL;

	if (!options) {
		goto out;
	}

	while ((p = strsep(&options, ",")) != NULL) {
		if (!*p)
			continue;
		token = match_token(p, tokens, args);
		switch (token) {
			case ccfs_opt_prefetch:
				ccfssuper->prefetch = 1;
				break;
			case ccfs_opt_cacheable_filter:
				ccfssuper->cacheable_filter = create_filter();
				match_strlcpy(tmp, &args[0], PATH_MAX);
				parse_filter(ccfssuper->cacheable_filter, tmp);
				break;
			default:
				printk(KERN_WARNING
					"ccfs: unrecognized option '%s'\n",
					p);
		}

	}
	
out:
	return rc;
}

static int ccfs_get_sb(struct file_system_type *fs_type, int flags,
			const char *dev_name, void *raw_data,
			struct vfsmount *mnt)
{
	int rc;
	struct super_block *sb;

	rc = get_sb_nodev(fs_type, flags, raw_data, ccfs_fill_super, mnt);
	if (rc < 0) {
		printk(KERN_ERR "Getting sb failed; rc = [%d]\n", rc);
		goto out;
	}
	sb = mnt->mnt_sb;
	rc = ccfs_parse_options(sb, raw_data);
	if (rc) {
		printk(KERN_ERR "Error parsing options; rc = [%d]\n", rc);
		goto out_abort;
	}

	rc = ccfs_read_super(sb, dev_name);
	if (rc) {
		printk(KERN_ERR "Reading sb failed; rc = [%d]\n", rc);
		goto out_abort;
	}
	goto out;
out_abort:
	dput(sb->s_root);
	up_write(&sb->s_umount);
	deactivate_super(sb);
out:
	return rc;
}

static void ccfs_kill_block_super(struct super_block *sb)
{
	generic_shutdown_super(sb);
}


static struct file_system_type ccfs_fs_type = {
	.owner = THIS_MODULE,
	.name = "ccfs",
	.get_sb = ccfs_get_sb,
	.kill_sb = ccfs_kill_block_super,
	.fs_flags = 0,
};

static void
ccfs_inode_init_once(void *vptr)
{
	struct ccfs_inode *ccfs_inode = (struct ccfs_inode*)vptr;

	inode_init_once(&ccfs_inode->vfs_inode);
}

static struct ccfs_cache {
	struct kmem_cache **cache;
	const char *name;
	size_t size;
	//void (*ctor)(struct kmem_cache *cache, void *obj);
	void (*ctor)(void*);
} ccfs_caches[] = {
	{
		.cache = &ccfs_file_cache,
		.name = "ccfs_file_cache",
		.size = sizeof(struct ccfs_file),
	},
	{
		.cache = &ccfs_dentry_cache,
		.name = "ccfs_dentry_cache",
		.size = sizeof(struct ccfs_dentry),
	},
	{
		.cache = &ccfs_inode_cache,
		.name = "ccfs_inode_cache",
		.size = sizeof(struct ccfs_inode),
		.ctor = ccfs_inode_init_once,
	},
	{
		.cache = &ccfs_sb_cache,
		.name = "ccfs_sb_cache",
		.size = sizeof(struct ccfs_super),
	},
};

static void ccfs_free_kmem_caches(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ccfs_caches); i++) {
		struct ccfs_cache *info;

		info = &ccfs_caches[i];
		if (*(info->cache))
			kmem_cache_destroy(*(info->cache));
	}
}

static int ccfs_init_kmem_caches(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ccfs_caches); i++) {
		struct ccfs_cache *info;

		info = &ccfs_caches[i];
		*(info->cache) = kmem_cache_create(info->name, info->size,
				0, SLAB_HWCACHE_ALIGN, info->ctor);
		if (!*(info->cache)) {
			ccfs_free_kmem_caches();
			printk(KERN_WARNING "%s: "
					"kmem_cache_create failed\n",
					info->name);
			return -ENOMEM;
		}
	}
	return 0;
}


static int __init ccfs_init(void)
{
	int rc;

	rc = ccfs_init_kmem_caches();
	if (rc) {
		printk(KERN_ERR
		       "Failed to allocate one or more kmem_cache objects\n");
		goto out;
	}
	rc = register_filesystem(&ccfs_fs_type);
	if (rc) {
		printk(KERN_ERR "Failed to register filesystem\n");
		goto out_free_kmem_caches;
	}

	rc = initialize_prefetcher();
	if (rc) {
		printk(KERN_ERR "Failed to start prefetcher\n");
		goto out_free_kmem_caches;
	}

	goto out;
	
out_free_kmem_caches:
	ccfs_free_kmem_caches();
out:
	return rc;
}

static void __exit ccfs_exit(void)
{
	finalize_prefetcher();
	unregister_filesystem(&ccfs_fs_type);
	ccfs_free_kmem_caches();
}

MODULE_DESCRIPTION("ccfs");

MODULE_LICENSE("GPL");

module_init(ccfs_init)
module_exit(ccfs_exit)
