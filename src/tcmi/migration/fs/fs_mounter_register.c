#include "fs_mounter_register.h"
#include <dbg.h>
#include <asm/string.h>
#include <linux/slab.h>

#include "fs_mounter.h"
#include "fs_mount_params.h"
#include "9p_fs_mounter.h"
#include "9p_fs_global_mounter.h"

/**
 * Returns new mounter instance with specified params.
 */
struct fs_mounter* get_new_mounter(struct fs_mount_params* params) {
	if ( strcmp("9p",params->mount_type) == 0 )
		return new_9p_fs_mounter(params);

	if ( strcmp("9p-global",params->mount_type) == 0 )
		return new_9p_global_fs_mounter(params);

	return NULL;
}

/** Releases mounter instance */
void free_mounter(struct fs_mounter* mounter) {
	if ( !mounter )
		return;
	
	if ( mounter->free )
		mounter->free(mounter);

	kfree(mounter);
}
