#include <linux/file.h>
#include <linux/poll.h>
#include <linux/mount.h>
#include <linux/pagemap.h>
#include <linux/security.h>
#include <linux/compat.h>
#include <linux/fs_stack.h>
#include "ccfs.h"
#include "prefetcher.h"

#include <dbg.h>

struct kmem_cache *ccfs_file_cache;

int ccfs_readdir (struct file *filp, void *dirent, filldir_t filldir) {
	int rc;
	struct file *lower_file;
	struct inode *inode;

	lower_file = ccfs_get_nested_file(filp);
	mdbg(INFO3, "Read filp %p with lower file: %p Pos: %lu (Upper pos: %lu)", filp, lower_file, (unsigned long)lower_file->f_pos, (unsigned long)filp->f_pos);
	lower_file->f_pos = filp->f_pos;
	inode = filp->f_path.dentry->d_inode;
	rc = vfs_readdir(lower_file, filldir, dirent);
	mdbg(INFO3, "Read return code: %d", rc);
	filp->f_pos = lower_file->f_pos;	
	if (rc >= 0)
		fsstack_copy_attr_atime(inode, lower_file->f_path.dentry->d_inode);

	// The following does not seem to be true at least with kernel 2.6.33.1, so we leave dirs cacheable again
	// We need to disable file caching after readdir, as some filesystems (9P) are not able to repear readdirs on a same file!
	//ccfs_inode_to_private(inode)->cacheable = 0;

	return rc;
}

static int ccfs_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	       unsigned long arg)
{
	int rc = 0;
	struct file *lower_file = NULL;

	if (ccfs_get_nested_file(file))
		lower_file = ccfs_get_nested_file(file);
	if (lower_file && lower_file->f_op && lower_file->f_op->ioctl)
		rc = lower_file->f_op->ioctl(ccfs_get_nested_inode(inode),
					     lower_file, cmd, arg);
	else
		rc = -ENOTTY;
	return rc;
}


int ccfs_close_lower_file(struct file *lower_file)
{
	mdbg(INFO3,"Closing lower file file %p", lower_file);
	fput(lower_file);
	return 0;
}

static int ccfs_open(struct inode *inode, struct file *file)
{
	struct file *lower_file = NULL;	
	struct ccfs_file* file_info = NULL;
	int rc = 0;
	struct ccfs_super* sb_info = ccfs_superblock_to_private(inode->i_sb);

	file_info = kmem_cache_zalloc(ccfs_file_cache, GFP_KERNEL);
	if ( !file_info ) {
		rc = -ENOMEM;
		goto out;
	}
	
	ccfs_set_file_private(file, file_info);

	if ( !ccfs_inode_to_private(inode)->cacheable ) {
		// Haven't found a reason why would we reopen file here, we just wait let the file close and invalidate when nobody is using it. It is not 100% correct thing in cases of races
		// but since this is just a toy FS for experimental measurements, we can leave it as it is (unless we want to measure something with races in FS;)
	  
		// TODO: This is a hack to reopen "root" dentry, it is not required for other entries. Moreover, we can now reset cacheable flag, but do we need to?
		// TODO: The comment above is really cryptic and not sure what I was trying to say :(... Generally, the files are refreshed when they are non-cacheable and
		// last reference to them is dropped which seems like a safe behaviour.. my suspition is, the comment was saying it does not happen for root node, which is always
		// held, but not sure.. for now disable due to caused races and if I find what is the real reason for this call I'll update this code/comment
		// ccfs_reopen_persistent_file(file->f_path.dentry, inode);
		
//		if ( !invalidate_inode_pages2(inode->i_mapping) ) {
//		    minfo(ERR3, "Cache invalidation has failed");
//		}
	}	

	lower_file = ccfs_inode_to_private(inode)->lower_file;
	mdbg(INFO3, "Opening inode %p (file %p) associated with lower file: %p (dname: [%s]) - cacheable: %d", inode, file, lower_file, file->f_dentry->d_name.name, ccfs_inode_to_private(inode)->cacheable);
	if ( !lower_file ) {
		minfo(ERR3, "Assertion failed! Lower file does not exist!");
		kmem_cache_free(ccfs_file_cache, file_info);
		rc = -ENOENT;
		goto out;		
	}

	ccfs_set_nested_file(file, lower_file);

	if ( sb_info->prefetch && S_ISREG(inode->i_mode) ) { // TODO: Schedule this only on first open of file?
		mdbg(INFO3,"Scheduling prefetch for inode %p", inode);
		// TODO: Uncommented as there seems to be some problem with prefetcher.. on freeing pages we usually got 
		// General protection fault.. it is probably caused by allocation of pages in context of a different proces?
		
		//atomic_inc(&file->f_count); // Get reference that'll be freed by prefetcher after the prefetch is done
		//submit_for_prefetch(file);
	}
out:
	return rc;	
}

static int ccfs_release(struct inode *inode, struct file *file)
{
	struct ccfs_file *file_info = ccfs_get_file_private(file);

	mdbg(INFO3,"Release file %p", file);
	ccfs_set_nested_file(file, NULL); // Improve error catching ;)

	kmem_cache_free(ccfs_file_cache, file_info);
	return 0;
}

static int ccfs_flush(struct file *file, fl_owner_t td)
{
	int rc = 0;
	struct file *lower_file = NULL;

	mdbg(INFO3,"Flush file %p", file);
	lower_file = ccfs_get_nested_file(file);
	mdbg(INFO3,"Flush lower file %p (%ld)", lower_file, atomic_long_read(&lower_file->f_count));
	
	BUG_ON(!lower_file);
	
	if (lower_file->f_op && lower_file->f_op->flush)
		rc = lower_file->f_op->flush(lower_file, td);
	return rc;
}

static int
ccfs_fsync(struct file *file, struct dentry *dentry, int datasync)
{
	struct file *lower_file = ccfs_get_nested_file(file);
	struct dentry *lower_dentry = ccfs_get_nested_dentry(dentry);
	struct inode *lower_inode = lower_dentry != NULL ? lower_dentry->d_inode : NULL;
	int rc = -EINVAL;
	
	BUG_ON(!lower_dentry);
	BUG_ON(!lower_inode);	
	
	mdbg(INFO3,"Fsync file %p", file);
	
	if (lower_inode->i_fop->fsync) {
		mutex_lock(&lower_inode->i_mutex);
		rc = lower_inode->i_fop->fsync(lower_file, lower_dentry,
					       datasync);
		mutex_unlock(&lower_inode->i_mutex);
	}
	return rc;
}

static int ccfs_fasync(int fd, struct file *file, int flag)
{
	int rc = 0;
	struct file *lower_file = NULL;

	mdbg(INFO3,"Fasync file %p", file);
	
	lower_file = ccfs_get_nested_file(file);
	if (lower_file->f_op && lower_file->f_op->fasync)
		rc = lower_file->f_op->fasync(fd, lower_file, flag);
	return rc;
}

static ssize_t ccfs_splice_read(struct file *file, loff_t * ppos,
				    struct pipe_inode_info *pipe, size_t count,
				    unsigned int flags)
{
	struct file *lower_file = NULL;
	int rc = -EINVAL;

	lower_file = ccfs_get_nested_file(file);
	if (lower_file->f_op && lower_file->f_op->splice_read)
		rc = lower_file->f_op->splice_read(lower_file, ppos, pipe,
						count, flags);

	return rc;
}

static int ccfs_file_mmap(struct file * file, struct vm_area_struct * vma) {
	mdbg(INFO3,"Mmap file %p [%s]", file, file->f_dentry->d_name.name);
  
	return generic_file_mmap(file, vma);
}

static ssize_t ccfs_do_sync_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos) {
  ssize_t res;
  res = do_sync_read(filp, buf, len, ppos);
  mdbg(INFO3,"Read file %p [%s] -> Len: %ld, Res: %ld", filp, filp->f_dentry->d_name.name, len, res);  
  return res;
}

static ssize_t ccfs_generic_file_aio_read(struct kiocb *iocb, const struct iovec *iov, unsigned long nr_segs, loff_t pos) {
  ssize_t res;
  res = generic_file_aio_read(iocb, iov, nr_segs, pos);
  mdbg(INFO3,"Async Read ... Pos: %ld, Res: %ld", pos, res);  
  return res;
}

const struct file_operations ccfs_dir_fops = {
	.readdir = ccfs_readdir,
	.ioctl = ccfs_ioctl,
	.mmap = ccfs_file_mmap,
	.open = ccfs_open,
	.flush = ccfs_flush,
	.release = ccfs_release,
	.fsync = ccfs_fsync,
	.fasync = ccfs_fasync,
	.splice_read = ccfs_splice_read,
};

const struct file_operations ccfs_main_fops = {
	.llseek = generic_file_llseek,
	.read = ccfs_do_sync_read,
	.aio_read = ccfs_generic_file_aio_read,
	.write = do_sync_write,
	.aio_write = generic_file_aio_write,
	.readdir = ccfs_readdir,
	.ioctl = ccfs_ioctl,
	.mmap = ccfs_file_mmap,
	.open = ccfs_open,
	.flush = ccfs_flush,
	.release = ccfs_release,
	.fsync = ccfs_fsync,
	.fasync = ccfs_fasync,
	.splice_read = ccfs_splice_read,
};
