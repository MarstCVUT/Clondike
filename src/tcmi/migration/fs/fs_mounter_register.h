#ifndef _FS_MOUNTER_REGISTER_H
#define _FS_MOUNTER_REGISTER_H

struct fs_mounter;
struct fs_mount_params;

/**
 * Returns new mounter instance with specified params.
 */
struct fs_mounter* get_new_mounter(struct fs_mount_params* params);

/** Releases mounter instance */
void free_mounter(struct fs_mounter* mounter);

#endif
