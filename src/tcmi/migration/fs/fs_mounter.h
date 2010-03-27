#ifndef _FS_MOUNTER_H
#define _FS_MOUNTER_H

#include "fs_mount_params.h"
#include <linux/types.h>

/** 
 * Class handling the filesystem mounting, that is performed before the process restart 
 * If there is no fs_mounter registered, no mounting is performed and the filesystem is expected to be already mounted (or not required)
 */
struct fs_mounter {
	struct fs_mount_params params;
	/** Called, in context of a kernel process, it can make mounts to a global namespace*/
	int (*global_mount)(struct fs_mounter* self, int16_t fsuid, int16_t fsgid);
	/** Called, in context of a new process, in its private namespace */
	int (*private_mount)(struct fs_mounter* self, int16_t fsuid, int16_t fsgid);
	/** Custom free method */
	void (*free)(struct fs_mounter* self);
};

#endif
