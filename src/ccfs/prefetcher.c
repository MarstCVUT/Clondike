#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <asm/page.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/pagemap.h>
#include <linux/pagevec.h>

#include "prefetcher.h"
#include <dbg.h>

struct prefetcher_data {
	/** Reference to current prefetcher thread, if there is any */
	struct task_struct* prefetcher_thread;
	/** For waiting on the prefetch requests */
	wait_queue_head_t request_wait;
	/** Lock guarding prefetch enqueing */
	spinlock_t enqueue_lock;
	/** Requests for prefetching */
	struct list_head requests;
};

static struct prefetcher_data* prefetcher = NULL;

/** Queued info about files prefetching */
struct prefetch_info {
	/** File to be prefetched */
	struct file* filp;
	/** Used to enqueue into the requests list */
	struct list_head head;
};

static inline int is_prefetch_requested(void) {
	int empty;

	spin_lock(&prefetcher->enqueue_lock);
	empty = list_empty(&prefetcher->requests);
	spin_unlock(&prefetcher->enqueue_lock);

	return empty == 0;
}

static struct prefetch_info* get_next_prefetch_info(void) {
	struct prefetch_info* info = NULL;
	wait_event_interruptible( prefetcher->request_wait, is_prefetch_requested() || kthread_should_stop());

	spin_lock(&prefetcher->enqueue_lock);
	if ( !list_empty(&prefetcher->requests) ) {
		struct list_head* first = prefetcher->requests.next;
		list_del(first);
		info = list_entry(first, struct prefetch_info, head);
	}
	spin_unlock(&prefetcher->enqueue_lock);
	
	return info;
}

#define list_to_page(head) (list_entry((head)->prev, struct page, lru))
/** Copied from kernel */
static int read_pages(struct address_space *mapping, struct file *filp,
                struct list_head *pages, unsigned nr_pages)
  {
	unsigned page_idx;
	int ret;

	if (mapping->a_ops->readpages) {
		ret = mapping->a_ops->readpages(filp, mapping, pages, nr_pages);
		/* Clean up the remaining pages */
		put_pages_list(pages);
		goto out;
	}

	for (page_idx = 0; page_idx < nr_pages; page_idx++) {
		struct page *page = list_to_page(pages);
		list_del(&page->lru);
		if (!add_to_page_cache_lru(page, mapping,
					page->index, GFP_KERNEL)) {
			mapping->a_ops->readpage(filp, page);
		}
		page_cache_release(page);
	}
	ret = 0;
out:
	return ret;
}

/** TODO: This is now +- copy of kernel readahead code. We may either use kernel code here, or better rewrite this to do some sort
 of paralelized prefetched! */
static void do_file_prefetch(struct file* filp) {
	struct address_space* mapping = filp->f_mapping;
	struct inode *inode = mapping->host;
	struct page* page;
	loff_t isize = i_size_read(inode);
	unsigned long end_index = ((isize - 1) >> PAGE_CACHE_SHIFT);
	unsigned long index = 0;
	int pool_size = 0;
	LIST_HEAD(page_pool);

	printk(KERN_DEBUG "File %p prefetching %lu pages\n", filp, end_index);

	spin_lock_irq(&mapping->tree_lock);
	for ( ; index < end_index; index++ ) {
		pgoff_t page_offset = index;
		page = radix_tree_lookup(&mapping->page_tree, page_offset);
 	        if (page) // Already exists?
 	            	continue;

 		spin_unlock_irq(&mapping->tree_lock);
 	        page = page_cache_alloc_cold(mapping);
 	        spin_lock_irq(&mapping->tree_lock);
 	        if (!page)
 	        	break;
 	        
		pool_size++;
		page->index = page_offset;
 	        list_add(&page->lru, &page_pool);
	}
	spin_unlock_irq(&mapping->tree_lock);

	read_pages(mapping, filp, &page_pool, pool_size);
}

static void perform_prefetch(struct prefetch_info* info) {
	do_file_prefetch(info->filp);
	mdbg(INFO3,  "Puting lower file: %p (%ld)", info->filp, atomic_long_read(&info->filp->f_count));
	fput(info->filp);
	kfree(info);
}

static int prefetcher_run(void* data){
	printk(KERN_DEBUG "Prefetcher thread started\n");
	while(!kthread_should_stop()) {
		// Read from prefetch queue
		struct prefetch_info* info = get_next_prefetch_info();
		if ( !info )
			break;

		// Do prefetch of a single element
		perform_prefetch(info);
		printk(KERN_DEBUG "Prefetch done\n");
	}

	printk(KERN_DEBUG "Prefetcher thread done\n");

	return 0;
}

int initialize_prefetcher(void) {	
	struct task_struct* new_thread = NULL;

	if ( prefetcher )
		return -EINVAL;

	prefetcher = kmalloc(sizeof(struct prefetcher_data), GFP_KERNEL);
	if ( !prefetcher ) {
		return -EINVAL;
	}

	prefetcher->prefetcher_thread = new_thread;
	INIT_LIST_HEAD( & prefetcher->requests );
	spin_lock_init(&prefetcher->enqueue_lock);
	init_waitqueue_head( & prefetcher->request_wait );

	new_thread = kthread_run(prefetcher_run, NULL, "ccfs prefetching thread");
	if( IS_ERR(new_thread) ) {		
		kfree(prefetcher);
		prefetcher = NULL;
		return PTR_ERR(new_thread);
	}

	prefetcher->prefetcher_thread = new_thread;

	return 0;
}

void finalize_prefetcher(void) {
	int rc;

	if ( !prefetcher )
		return;

	rc = kthread_stop(prefetcher->prefetcher_thread);
	if ( rc ) {
		printk(KERN_ERR "Prefetcher stop failed with err %d\n", rc);
	}

	prefetcher->prefetcher_thread = NULL;
	kfree(prefetcher);
	prefetcher = NULL;
}

void submit_for_prefetch(struct file* filp) {
	struct prefetch_info* info = kmalloc(sizeof(struct prefetch_info), GFP_KERNEL);
	if ( !info ) {
		fput(filp);
		// We can ignore no-mem on prefetch, we simply do not prefetch..
		return;
	}

	info->filp = filp;
	spin_lock(&prefetcher->enqueue_lock);
	list_add(&info->head, &prefetcher->requests);
	spin_unlock(&prefetcher->enqueue_lock);

	wake_up(&prefetcher->request_wait);
}
