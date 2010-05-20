#ifndef _LIB_UTIL_H
#define _LIB_UTIL_H

#include <linux/pid_namespace.h>
#include <linux/namei.h>

static inline struct task_struct* task_find_by_pid(pid_t pid) {
	return pid_task(find_pid_ns(pid, current->nsproxy->pid_ns), PIDTYPE_PID);
}

/**
 * Creates a new directory. Assumes, its parent directory already exists!!
 */
static inline int mk_dir(const char* path, int mode) {
	struct nameidata nd;
	struct dentry *dentry;
	int err;

	err = path_lookup(path, LOOKUP_PARENT ,&nd);
	if ( err ) {
		return err;
	}

	dentry = lookup_create(&nd, 1);
        err = PTR_ERR(dentry);
        if (IS_ERR(dentry))
                return err;

	err = vfs_mkdir(nd.path.dentry->d_inode, dentry, mode);
	return err;
}

#endif
