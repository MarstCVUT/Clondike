#ifndef _9P_GLOBAL_FS_MOUNTER_H
#define _9P_GLOBAL_FS_MOUNTER_H

struct fs_mounter;
struct fs_mount_params;

/**
 * 9p mounter handles mounting of Plan9 filesystem in a public namespace. It does not support any security right not.
 */

/** Creates a new instance of the 9p file system mounter */
struct fs_mounter* new_9p_global_fs_mounter(struct fs_mount_params* params);

#endif
