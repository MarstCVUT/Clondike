#ifndef _9P_FS_HELPER_H
#define _9P_FS_HELPER_H

#include <linux/types.h>
#include <linux/completion.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/sched.h>
#include <linux/jiffies.h>

#define MAX_ADDITIONAL_CONNECT_STR_LEN 200

static int mount_9p_fs(const char* mount_point, struct fs_mounter* mounter, int16_t fsuid, int16_t fsgid) {
	int err;
	char* device = mounter->params.mount_device;
	char* connect_param = mounter->params.mount_options;
	char* mount_point_page = __getname();
	int connect_str_len = strlen(connect_param);

	if ( !mount_point_page )
		return -ENOMEM;

	memcpy(mount_point_page, mount_point, strlen(mount_point) + 1);
	
	connect_param = kmalloc(connect_str_len + MAX_ADDITIONAL_CONNECT_STR_LEN, GFP_KERNEL);
	if ( !connect_param ) {
		err = -ENOMEM;
		goto exit0;
	}

	if ( connect_str_len == 0 ) {
		snprintf(connect_param, connect_str_len + MAX_ADDITIONAL_CONNECT_STR_LEN, "cuid=%d,cgid=%d", fsuid, fsgid);
	} else {
		snprintf(connect_param, connect_str_len + MAX_ADDITIONAL_CONNECT_STR_LEN, "%s,cuid=%d,cgid=%d", mounter->params.mount_options, fsuid, fsgid);
	}

	mdbg(INFO2, "9p mount device: %s mount options: %s mount point: %s", device, connect_param, mount_point);

	err = do_mount(device, mount_point_page, "9p", 0, connect_param);
	mdbg(INFO2, "After 9P mount: %d", err);
	if ( err ) {
		minfo(ERR1, "Failed to mount filesystem: %d", err);
		goto exit1;
	}
	return 0;

exit1:
	kfree(connect_param);
exit0:
	__putname(mount_point_page);
	return err;	
};

/**
 * Chroots process to a new root
 *
 * @todo: Move to some generic fs helper, if we ever have another fs mounter.
 */
static int chroot_to(const char* path_to_chroot) {
	int err;
	struct path old_root;
	struct nameidata nd;

	err = path_lookup(path_to_chroot, LOOKUP_FOLLOW | LOOKUP_DIRECTORY ,&nd);
	if ( err ) {
		minfo(ERR1, "Failed to lookup path %s: %d", path_to_chroot, err);
		goto exit0;
	}	

	/* Chroots.. duplicated code from chroot as it cannot be used directly */
        write_lock(&current->fs->lock);
        old_root = current->fs->root;
	path_get(&nd.path);
        current->fs->root = nd.path;
        write_unlock(&current->fs->lock);
        if (old_root.dentry) {
                 path_put(&old_root);                 
        }

	return 0;

exit0:
	return err;
};

#endif
