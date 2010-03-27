/**
 * @file tcmi_ctlfs_test2.c - Test case for the TCMI control file system
 *                      - during initialization, it creates 1 directory and 2 files in the control 
 *                        filesystem
 *                      - the methods associated with file1 will accept 1 integer(4bytes)
 *                      - the methods associated with file2 will accept a vector of 4 integer(16bytes)
 *                     
 *                      - during exit - deletes all 3 entries
 * 
 *
 *
 * Date: 03/29/2005
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
#include <linux/mount.h>

#include <tcmi/ctlfs/tcmi_ctlfs.h>
#include <tcmi/ctlfs/tcmi_ctlfs_entry.h>
#include <tcmi/ctlfs/tcmi_ctlfs_dir.h>
#include <tcmi/ctlfs/tcmi_ctlfs_file.h>
#include <tcmi/ctlfs/tcmi_ctlfs_symlink.h>

#include <dbg.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

static struct tcmi_ctlfs_entry *root = NULL;
static struct tcmi_ctlfs_entry *dir1 = NULL;
static struct tcmi_ctlfs_entry *dir2 = NULL;
static struct tcmi_ctlfs_entry *dir3 = NULL;
static struct tcmi_ctlfs_entry *file1 = NULL;
static struct tcmi_ctlfs_entry *file2 = NULL;
static struct tcmi_ctlfs_entry *file3 = NULL;
static struct tcmi_ctlfs_entry *symlink1 = NULL;

static int testdata[4] = {1234, 567899999, 988887, 555555555};
static char teststring[] = {"My first test string"};

static int get1word(void *object, void *data)
{
	int *word = (int *)data;
	minfo(INFO1, "got word[0] value: %d", testdata[0]);
	*word = testdata[0];
	return 0;
}

static int set1word(void *object, void *data)
{
	int *word = (int *)data;
	minfo(INFO1, "set word[0] value: %d", *word);
	testdata[0] = *word;
	return 0;
}

static int get4word(void *object, void *data)
{
	int *word = (int *)data;
	int i;
	for(i = 0; i < sizeof(testdata)/sizeof(int); i++) {
		minfo(INFO1, "got word[%d] value: %d", i, testdata[i]);
		word[i] = testdata[i];
	}
	return 0;
}

static int set4word(void *object, void *data)
{
	int *word = (int *)data;
	int i;
	for(i = 0; i < sizeof(testdata)/sizeof(int); i++) {
		minfo(INFO1, "set word[%d] value: %d", i, word[i]);
		testdata[i] = word[i];
	}
	return 0;
}

static int getstring(void *object, void *data)
{
	char *str = (char *)data;
	minfo(INFO1, "Get string:%s", teststring);
	memcpy(str, teststring, sizeof(teststring));
	return 0;
}
static int setstring(void *object, void *data)
{
	char *str = (char *)data;
	minfo(INFO1, "Set string:%s", str);
	memcpy(teststring, str, sizeof(teststring));
	return 0;
}

/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_ctlfs_test2_init(void)
{
	root = tcmi_ctlfs_get_root();
	minfo(INFO1, "Root entry reference count = %d", atomic_read(&TCMI_CTLFS_ENTRY(root)->dentry->d_count));
	dir1 = tcmi_ctlfs_dir_new(root, 0750, "directory1");
	dir2 = tcmi_ctlfs_dir_new(root, 0750, "directory2");
	dir3 = tcmi_ctlfs_dir_new(root, 0750, "directory3");
	minfo(INFO1, "created dir1 %p, dir2 %p, dir3 %p", dir1, dir2, dir3);
	file1 = tcmi_ctlfs_intfile_new(root, 0750,
				       NULL, get1word, set1word,
				       sizeof(int), "file1");  

	file2 = tcmi_ctlfs_intfile_new(dir1, 0750,
				       NULL, get4word, set4word,
				       sizeof(int) * 4, "file2");

	file3 = tcmi_ctlfs_strfile_new(dir1, 0750,
				       NULL, getstring, setstring,
				       sizeof(teststring), "file3");
	symlink1 = tcmi_ctlfs_symlink_new(dir1, file1, "sym1");

	minfo(INFO1, "Root entry reference count = %d after", atomic_read(&TCMI_CTLFS_ENTRY(root)->dentry->d_count));
	minfo(INFO1, "Dir2 reference count = %d after", atomic_read(&TCMI_CTLFS_ENTRY(dir2)->dentry->d_count));
	return 0;
}

/**
 * module cleanup
 * The order in which we destroy the entries is not important as the reference
 * counter of all the VFS dentries takes care of it automatically
 */
static void __exit tcmi_ctlfs_test2_exit(void)
{

	minfo(INFO1, "Destroying all created entries");
	tcmi_ctlfs_entry_put(dir1);
	tcmi_ctlfs_entry_put(dir2);
	tcmi_ctlfs_entry_put(dir3);

	tcmi_ctlfs_file_unregister(file1);
	tcmi_ctlfs_entry_put(file1); 

	tcmi_ctlfs_file_unregister(file2);
	tcmi_ctlfs_entry_put(file2); 

	tcmi_ctlfs_file_unregister(file3);
	tcmi_ctlfs_entry_put(file3); 

	tcmi_ctlfs_entry_put(symlink1);

	minfo(INFO1, "Root entry reference count = %d after deleting all entries", 
	      atomic_read(&TCMI_CTLFS_ENTRY(root)->dentry->d_count));
	tcmi_ctlfs_put_root();

}




module_init(tcmi_ctlfs_test2_init);
module_exit(tcmi_ctlfs_test2_exit);



