#include <linux/pagemap.h>
#include <linux/writeback.h>
#include <linux/page-flags.h>
#include <linux/mount.h>
#include <linux/file.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include "ccfs.h"

#include <dbg.h>

// TODO: Unused
struct kmem_cache *ccfs_lower_page_cache;

struct page *ccfs_get_locked_page(struct file *file, loff_t index)
{
	struct dentry *dentry;
	struct inode *inode;
	struct address_space *mapping;
	struct page *page;

	dentry = file->f_path.dentry;
	inode = dentry->d_inode;
	mapping = inode->i_mapping;
	page = read_mapping_page(mapping, index, (void *)file);
	if (!IS_ERR(page))
		lock_page(page);
	return page;
}

static int ccfs_writepage(struct page *page, struct writeback_control *wbc)
{
	int rc = 0;

	mdbg(INFO3,"Write page index: %lu (%p)", (unsigned long)page->index, page);

	// TODO: Write to lower FS.. but we do not support writeback for this fs yet!
	SetPageUptodate(page);
	unlock_page(page);

	return rc;
}


static int ccfs_readpage(struct file *file, struct page *page)
{
	int rc = 0;

	mdbg(INFO3,
			"Reading page index: %lu (%p)", (unsigned long)page->index, page);
	rc = ccfs_read_lower_page_segment(page, page->index, 0,
						PAGE_CACHE_SIZE,
						page->mapping->host);

	if (rc) {
		ClearPageUptodate(page);
	} else {
		mdbg(INFO3, "Setting page up to date");
		//flush_dcache_page(page);
		SetPageUptodate(page);
	}
	mdbg(INFO3, "Unlocking page with index = [0x%.16x]",
			(u16)page->index);
	unlock_page(page);
	return rc;
}


static int ccfs_write_begin(
			struct file *file,
			struct address_space *mapping,
			loff_t pos, unsigned len, unsigned flags,
			struct page **pagep, void **fsdata)
{
	int rc = 0;
	loff_t prev_page_end_size;
	pgoff_t index = pos >> PAGE_CACHE_SHIFT;
	struct page *page;

	page = grab_cache_page_write_begin(mapping, index, flags);
	if (!page)
		return -ENOMEM;
	*pagep = page;

	mdbg(INFO3, "Prepare write for file: %p page index: %lu", file, (unsigned long)page->index);

	if (!PageUptodate(page)) {
		rc = ccfs_read_lower_page_segment(page, index, 0,
						      PAGE_CACHE_SIZE,
						      mapping->host);
		if (rc) {
			printk(KERN_ERR "%s: Error attemping to read lower "
			       "page segment; rc = [%d]\n", __FUNCTION__, rc);
			ClearPageUptodate(page);
			goto out;
		} else
			SetPageUptodate(page);
	}

	prev_page_end_size = ((loff_t)page->index << PAGE_CACHE_SHIFT);

out:
	return rc;
}

static int ccfs_write_end(struct file *file,
			struct address_space *mapping,
			loff_t pos, unsigned len, unsigned copied,
			struct page *page, void *fsdata)
{
	struct inode *ccfsinode = mapping->host;
	//pgoff_t index = pos >> PAGE_CACHE_SHIFT;
	unsigned from = pos & (PAGE_CACHE_SIZE - 1);
	unsigned to = from + copied;
	int rc = 0;

	mdbg(INFO3,"Commit write for file: %p page index: %lu (From: %lu To: %lu)", file, (unsigned long)page->index, (unsigned long)from, (unsigned long)to);

	rc = ccfs_write_lower_page_segment(ccfsinode, page, from, to);

	//pos = (((loff_t)page->index) << PAGE_CACHE_SHIFT) + to;
	if (pos + copied > i_size_read(ccfsinode)) {
		// TODO: DO we need to update lower file metadata?
		i_size_write(ccfsinode, pos + copied);
		//printk(KERN_DEBUG "Expanded file size to "
		//		"[0x%.16x], required (%lld)\n", (u16)i_size_read(ccfsinode), pos + copied);
	}

	unlock_page(page);
	page_cache_release(page);

	return copied;
}

static sector_t ccfs_bmap(struct address_space *mapping, sector_t block)
{
	int rc = 0;
	struct inode *inode;
	struct inode *lower_inode;

	inode = (struct inode *)mapping->host;
	lower_inode = ccfs_get_nested_inode(inode);
	if (lower_inode->i_mapping->a_ops->bmap)
		rc = lower_inode->i_mapping->a_ops->bmap(lower_inode->i_mapping,
							 block);
	return rc;
}


// TODO: Well, now write seems to work at least sometimes, though not sure it is really implemented correctly
struct address_space_operations ccfs_aops = {
	//.writepage = ccfs_writepage,
	.readpage = ccfs_readpage,
	.write_begin = ccfs_write_begin,
	.write_end = ccfs_write_end,
	.bmap = ccfs_bmap,
};
