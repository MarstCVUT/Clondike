#include "9p_fs_mounter.h"
#include "fs_mounter.h"
#include <dbg.h>
#include "9p_fs_helper.h"
#include <tcmi/lib/util.h>

static int do_private_9p_mount(struct fs_mounter* mounter, int16_t fsuid, int16_t fsgid) {	
	int err = 0;
	/* 
 	   TODO: improvement would be to find some existing dir, currently /mnt/test is assumed to exist, perhaps just /mnt would be better..
	*/
	if ( (err = mount_9p_fs("/mnt/test", mounter, fsuid, fsgid)) )
		return err;

	if ( (err = chroot_to("/mnt/test")) )
		// TODO: Unmount?
		return err;
		
	return 0;
}

struct fs_mounter* new_9p_fs_mounter(struct fs_mount_params* params) {
	struct fs_mounter* mounter = kmalloc(sizeof(struct fs_mounter), GFP_ATOMIC);
	
	if ( !mounter )
		return NULL;

	mounter->global_mount = NULL;
	mounter->private_mount = do_private_9p_mount;
	mounter->free = NULL;
	mounter->params = *params;
	return mounter;
}
