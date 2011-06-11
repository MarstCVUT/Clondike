#include <linux/fs.h>
#include <linux/pagemap.h>
#include "ccfs.h"

#include <dbg.h>

int ccfs_write_lower(struct inode *ccfsinode, char *data,
			 loff_t offset, size_t size)
{
	struct ccfs_inode *inode_info;
	ssize_t octets_written;
	mm_segment_t fs_save;
	int rc = 0;

	inode_info = ccfs_inode_to_private(ccfsinode);
	mutex_lock(&inode_info->lower_file_mutex);
	BUG_ON(!inode_info->lower_file);
	inode_info->lower_file->f_pos = offset;
	mdbg(INFO3, "Inode %p has lower file: %p (%ld)", ccfsinode, inode_info->lower_file, atomic_long_read(&inode_info->lower_file->f_count));
	
	fs_save = get_fs();
	set_fs(get_ds());
	octets_written = vfs_write(inode_info->lower_file, data, size,
				   &inode_info->lower_file->f_pos);
	set_fs(fs_save);
	if (octets_written < 0) {
	  	mdbg(INFO3, "Error writing. Written %d, size %d", octets_written, size);
		rc = -EINVAL;
	}	
	mutex_unlock(&inode_info->lower_file_mutex);
	mark_inode_dirty_sync(ccfsinode);
	return rc;
}

int ccfs_write_lower_page_segment(struct inode *ccfsinode,
				      struct page *page_for_lower,
				      size_t offset_in_page, size_t size)
{
	struct ccfs_inode *inode_info;
	char *virt;
	loff_t offset;
	int rc;
	inode_info = ccfs_inode_to_private(ccfsinode);

	mdbg(INFO3, "Inode %p has lower file: %p (%ld)", ccfsinode, inode_info->lower_file, atomic_long_read(&inode_info->lower_file->f_count));
	
	offset = ((((loff_t)page_for_lower->index) << PAGE_CACHE_SHIFT)
		  + offset_in_page);
	virt = kmap(page_for_lower);
	rc = ccfs_write_lower(ccfsinode, virt, offset, size);
	kunmap(page_for_lower);
	return rc;
}

int ccfs_read_lower(char *data, loff_t offset, size_t size,
			struct inode *ccfsinode)
{
	struct ccfs_inode *inode_info =
		ccfs_inode_to_private(ccfsinode);
	ssize_t octets_read;
	mm_segment_t fs_save;
	int rc = 0;

	BUG_ON(!inode_info);
			
	mutex_lock(&inode_info->lower_file_mutex);
	mdbg(INFO3, "Inode %p has lower file: %p (%ld)", ccfsinode, inode_info->lower_file, atomic_long_read(&inode_info->lower_file->f_count));
	
	BUG_ON(!inode_info->lower_file);
	inode_info->lower_file->f_pos = offset;
	fs_save = get_fs();
	set_fs(get_ds());
	octets_read = vfs_read(inode_info->lower_file, data, size,
			       &inode_info->lower_file->f_pos);
	set_fs(fs_save);
	if (octets_read < 0) {
		printk(KERN_ERR "%s: octets_read = [%td]; "
		       "expected [%td]\n", __FUNCTION__, octets_read, size);
		rc = -EINVAL;
	}
	mutex_unlock(&inode_info->lower_file_mutex);
	return rc;
}

int ccfs_read_lower_page_segment(struct page *page_for_ccfs,
				     pgoff_t page_index,
				     size_t offset_in_page, size_t size,
				     struct inode *ccfsinode)
{
	char *virt;
	loff_t offset;
	int rc;
	struct ccfs_inode *inode_info =
		ccfs_inode_to_private(ccfsinode);
		
	mdbg(INFO3, "Inode %p has lower file: %p (%ld)", ccfsinode, inode_info->lower_file, atomic_long_read(&inode_info->lower_file->f_count));	

	offset = ((((loff_t)page_index) << PAGE_CACHE_SHIFT) + offset_in_page);
	virt = kmap(page_for_ccfs);
	rc = ccfs_read_lower(virt, offset, size, ccfsinode);
	kunmap(page_for_ccfs);
	flush_dcache_page(page_for_ccfs);
	return rc;
}
