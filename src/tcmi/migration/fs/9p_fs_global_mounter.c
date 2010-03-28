#include "9p_fs_global_mounter.h"
#include "fs_mounter.h"
#include <dbg.h>
#include "9p_fs_helper.h"
#include <tcmi/lib/util.h>

#define MAX_MOUNT_ROOT_PATH 300

static void get_mount_root(const char* device, int fsuid, int fsgid, char* mount_root) {
	// TODO: Now we have fixed "root" of mount to /mnt/clondike.. maybe we can parametrize that? ;)
	sprintf(mount_root, "/mnt/clondike/%s-%d-%d", device, fsuid, fsgid);
}

static int global_9p_mount(struct fs_mounter* mounter, int16_t fsuid, int16_t fsgid) {	
	int err = 0;
	char mount_buffer[MAX_MOUNT_ROOT_PATH], root_buffer[MAX_MOUNT_ROOT_PATH+6];
	struct nameidata nd;
	get_mount_root(mounter->params.mount_device, fsuid, fsgid, mount_buffer);
	err = path_lookup(mount_buffer, LOOKUP_FOLLOW | LOOKUP_DIRECTORY ,&nd);
	if ( err ) {
		minfo(ERR1, "Failed to lookup path %s: %d. Trying to create", mount_buffer, err);
		err = mk_dir(mount_buffer, 777);
	}
	if ( err ) {
		// TODO: For now, the dirs for mount have to be precreated.. they can be for example
		// auto created via a user-space controller. In kernel, we do not have access to mkdir, right?
		minfo(ERR1, "Failed to lookup path %s: %d. Creation failed.", mount_buffer, err);
		return err;
	}
	sprintf(root_buffer, "%s/root", mount_buffer);
	// TODO: In critical section so that we do not do multiple mounts
	err = path_lookup(root_buffer, LOOKUP_FOLLOW | LOOKUP_DIRECTORY ,&nd);	
	if ( err ) {
		// We've checked that "root"/root does not exist (We assume existence of a root dir in root of fs
		// => 9p is not yet globally mount => mount
		if ( (err = mount_9p_fs(mount_buffer, mounter, fsuid, fsgid)) ) {
			minfo(ERR1, "Mount failed with err %d", err);
			return err;
		}
	}		

	return 0;
}

static int chroot_to_mount(struct fs_mounter* mounter, int16_t fsuid, int16_t fsgid) {	
	int err = 0;
	char mount_buffer[MAX_MOUNT_ROOT_PATH];

	get_mount_root(mounter->params.mount_device, fsuid, fsgid, mount_buffer);

	if ( (err = chroot_to(mount_buffer)) )
		return err;
		
	return 0;
}


struct fs_mounter* new_9p_global_fs_mounter(struct fs_mount_params* params) {
	struct fs_mounter* mounter = kmalloc(sizeof(struct fs_mounter), GFP_KERNEL);
	
	if ( !mounter )
		return NULL;

	mounter->global_mount = global_9p_mount;
	mounter->private_mount = chroot_to_mount;
	mounter->free = NULL;
	mounter->params = *params;
	return mounter;
}
