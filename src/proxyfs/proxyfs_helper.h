#ifndef PROXYFS_HELPER_H
#define PROXYFS_HELPER_H

#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/fdtable.h>

/**
 * Contains helper methods for working with proxyfs.
 */

static const char PROXYFS_MNT_POINT[] = "/mnt/proxy";

/**
 * Checks, whether the file is proxyfs file.
 *
 * @param path Path to the file
 * @return 1 if the file is proxyfs file, 0 otherwise
 */
static inline int is_proxyfs_file(const char* path) {
	int proxyfs_mnt_point_len = strlen(PROXYFS_MNT_POINT);
	return strncmp(path, PROXYFS_MNT_POINT, proxyfs_mnt_point_len) == 0;
}

/**
 * Returns true, if the path points to a dev null file
 */
static inline int is_dev_null(const char* path) {
	return strncmp(path, "/dev/null", 9) == 0;
}

/**
 * Creates a name of a proxy file
 *
 * @param owning_task Task, that is owning a reference to the physical file
 * @param buffer Output buffer to be filled with the path
 * @param proxy_ident Identifier of the file in the proxy file system 
 */
static inline void create_proxyfs_name(struct task_struct* owning_task, char* buffer, unsigned long proxy_ident) {
	sprintf(buffer, "%s/%d-%lu", PROXYFS_MNT_POINT, task_tgid_vnr(owning_task), proxy_ident);
}


/**
 * Modifies proxyfs file name to reflect a new owner of the file
 *
 * @param old_name Old name of proxy file
 * @param new_owner_pid CCN pid of the task, that is owning a reference to the physical file (new task)
 * @param buffer Output buffer to be filled with the path
 * @return 0 on successful change, 1 if no change was required, negative error code otherwise
 */
static inline int alter_proxyfs_name_task_change(const char* old_name, pid_t new_owner_pid, char* buffer) {
	if ( !is_proxyfs_file(old_name) ) {
		mdbg(ERR2, "Old name is not a proxyfs file name! Name: %s", old_name);
		return -EINVAL;
	} else {
		int proxyfs_mnt_point_len = strlen(PROXYFS_MNT_POINT);	
		pid_t old_pid;
		unsigned long proxy_ident;
	
		sscanf(old_name + proxyfs_mnt_point_len + 1, "%d-%lu",&old_pid, &proxy_ident);
		if ( old_pid == new_owner_pid )
			return 1; // No change required

		sprintf(buffer, "%s/%d-%lu",PROXYFS_MNT_POINT, new_owner_pid, proxy_ident);
	}

	return 0;
};

/**
 * Helper method, that is used for altering file names on distant fork.. this method resolves duplicate fds
 * that should not be changed vie close/open, but just sys_dup-ed
 */
static inline int resolve_duplicates(int modified_fd, struct file* modified_file) {
	int fd;
	struct file* file;

	for (fd = modified_fd+1, file = current->files->fdt->fd[fd]; 					\
	     fd < current->files->fdt->max_fds; fd++, file = current->files->fdt->fd[fd])		\
		if (FD_ISSET(fd, current->files->fdt->open_fds)) {

		// If this is a duplicate fd to file we had to modify => modify this fd to point to correct file
		if ( file == modified_file )	{ 
			mdbg(INFO3, "Resolving duplicate fd %d (duplicate of %d)", fd, modified_fd);
			sys_close(fd);

			if (sys_dup2(modified_fd, fd) != fd) {
				mdbg(ERR3, "Error duplicating file descriptor %d into %d!", modified_fd, fd);
				return -EFAULT;
			}
		}
	}

	return 0;
}

/**
 * This method is used after remote-fork on a client side. It will iterate all open files of the client
 * and if the file is a proxy file, it will be closed and a new file will be opened instead.
 *
 * On the serverside, there are created appropriated proxy files after the fork of shadow process, so they
 * are already prepared to be used.
 *
 * By using this "file clonning" mechanism, we ensure that we have independend file connections from parent and child
 * and so we can migrate parent on a different node that the child.
 *
 *
 * @param new_owner_pid Pid of the process, for which we are making the clone (pid on CCN!)
 * @return 0 on successs
 */
static inline int proxyfs_clone_file_names(pid_t new_owner_pid) {
	int fd, res = 0;
	struct file* file;
	unsigned long page;
	char* pathname;
	mm_segment_t old_fs;


	/* resolve the path name. */
	if (!(page = __get_free_page(GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate page for file pathname!");
		return -ENOMEM;
	}

	old_fs = get_fs();
	set_fs(get_ds());

	for (fd = 0, file = current->files->fdt->fd[fd]; 					\
	     fd < current->files->fdt->max_fds; fd++, file = current->files->fdt->fd[fd])		\
		if (FD_ISSET(fd, current->files->fdt->open_fds)) {
			// Same "hack" with -1000 as in tcmi_ckpt_openfile.. see there for cmnt
			if (IS_ERR(pathname = d_path(&file->f_path, (char*)page, PAGE_SIZE - 1000))) {
				mdbg(ERR3, "Can't resolve pathname for '%s'", file->f_dentry->d_name.name);
				res = -EINVAL;
				goto exit1;
			}

			if ( is_proxyfs_file(pathname) ) {
				u_int32_t flags;
				u_int32_t mode;
				u_int64_t pos;
				int new_fd, alter_res;
				
				mdbg(INFO3, "Resolving proxy fs file %s [%d]", pathname, fd);

				flags = file->f_flags;
				mode = file->f_mode;
				pos = file->f_pos;

				// Change filename to new proper name				
				alter_res = alter_proxyfs_name_task_change(pathname, new_owner_pid, (char*)page);
				if ( alter_res == 1 ) {
					continue; // File does not need to be changed.. it was a duplicate and is already changed	
				} else if ( alter_res < 0 ) {
					res = alter_res;
					goto exit1;
				}
				pathname = (char*)page;

				// Close old file
				sys_close(fd);
				// Open new file		
				if ((new_fd = do_sys_open(AT_FDCWD, pathname, flags, mode)) < 0) {
					mdbg(ERR3, "Error opening file '%s', err=%d", pathname, new_fd);
					res = new_fd;
					goto exit1;
				}
				// Ensure the new file has proper id
				if (new_fd != fd) {
					mdbg(INFO3, "Duplicating file descriptor %d into %d!", new_fd, fd);
					if (sys_dup2(new_fd, fd) != fd) {
						res = -EINVAL;
						mdbg(ERR3, "Error duplicating file descriptor %d into %d!", new_fd, fd);
						goto exit1;
					}
					// Close newly opened file that was not open with a proper fd
					sys_close(new_fd);
				}
				/* setup the file position */
				if (pos != vfs_llseek(current->files->fdt->fd[fd], pos, 0)) {
					mdbg(ERR3, "Failed to set position %Lx in file '%s'", pos, pathname);
					res = -EINVAL;
					goto exit1;
				}			

				// Resolve all other fd's that were pointing to this file.. they should not open it, they should instead duplicate the fd
				if ( resolve_duplicates(fd, file) ) {
					mdbg(ERR3, "Failed to resolve duplicates");
					res = -EINVAL;
					goto exit1;
				}
			}
	}

exit1:
	set_fs(old_fs);
	free_page(page);
	return res;
}

/** 
 * Synchronizes all files open by "current" process 
 *
 * Iterates over all open files of the process and synchronizes it.. it is required, because the shadow process
 * may end before the guest process and in this case some proxyfs files won't get synced on their close
 */
static inline void proxyfs_sync_files_on_exit(void) {
	int fd;
	struct file *file;

	for (fd = 0, file = current->files->fdt->fd[fd]; 					
	     fd < current->files->fdt->max_fds; fd++, file = current->files->fdt->fd[fd])	
		if (FD_ISSET(fd, current->files->fdt->open_fds)) {
			mdbg(INFO3, "Synchronizing fd: %d", fd);
			vfs_fsync(file, file->f_path.dentry, 0); // TODO: Here we may do fsync unnecessarily multiple times as files can be alias by multiple fds
	}
}

#endif
