/**
 * @file tcmi_ctlfs_test1.c - Basic test case for the TCMI control file system
 *                      - during initialization, it creates 3 directories in the control filesystem
 *                      - during exit - deletes all 3 entries
 * 
 *
 *
 * Date: 03/28/2005
 *
 * Author: Jan Capek
 *
 *
 * License....
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h>

#include <tcmi/ctlfs/tcmi_ctlfs.h>
/*#include <tcmifs/tcmi_ctlfs_entry.h>*/
#include <tcmi/ctlfs/tcmi_ctlfs_dir.h>

#include <dbg.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

static struct tcmi_ctlfs_entry *dir1;
static struct tcmi_ctlfs_entry *subdir1;
static struct tcmi_ctlfs_entry *subdir2;
static struct tcmi_ctlfs_entry *dir2;

/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_ctlfs_test1_init(void)
{
	struct tcmi_ctlfs_entry *root = tcmi_ctlfs_get_root();
	minfo(INFO1, "Initializing filesystem");
	dir1 = tcmi_ctlfs_dir_new(root, 0750, "directory-%d", 1);
	dir2 = tcmi_ctlfs_dir_new(root, 0750, "directory-2");
	subdir1 = tcmi_ctlfs_dir_new(dir1, 0750, "subdir1");
	subdir2 = tcmi_ctlfs_dir_new(dir1, 0752, "subdir2"); 
	
	return 0;
}

/**
 * module cleanup
 */
static void __exit tcmi_ctlfs_test1_exit(void)
{
	minfo(INFO1, "Destroying all created entries");
	tcmi_ctlfs_entry_put(dir1);
	tcmi_ctlfs_entry_put(dir2);
	tcmi_ctlfs_entry_put(subdir1);
	tcmi_ctlfs_entry_put(subdir2);
	tcmi_ctlfs_put_root();
}




module_init(tcmi_ctlfs_test1_init);
module_exit(tcmi_ctlfs_test1_exit);



