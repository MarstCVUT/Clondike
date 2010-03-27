#ifndef _TCMI_CKPT_FSSTRUCT_H
#define _TCMI_CKPT_FSSTRUCT_H

#include <linux/err.h>
#include <linux/mount.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/namei.h>
#include "tcmi_ckpt.h"

/** Currently only current "pwd" of the process is migrated. If more data are required (like current mount point) extend it
 * (though for current mount point we have to be careful as on PENs process may run chrooted into distributed FS)
 *
 * @TODO: If process is in not "rooted" in root of root fs, we do not handle the migration properly.
 */
static inline int tcmi_ckpt_fsstruct_write(struct tcmi_ckpt *ckpt, struct task_struct* task)
{
	char* pwd = NULL;
	int32_t pwdlen;
	unsigned long page;

	if (!(page = __get_free_page(GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate page for pwd pathname!");
		goto exit0;
	}

	pwd = d_path(&task->fs->pwd, (char*)page, PAGE_SIZE);
	if ( IS_ERR(pwd) ) {
		mdbg(ERR3, "Failed to lookup pwd path!");
		goto exit1;
	}

	pwdlen = strlen(pwd)+1;
		
	if (tcmi_ckpt_write(ckpt, &pwdlen, sizeof(pwdlen)) < 0) {
		mdbg(ERR3, "Error writing to checkpoint");
		goto exit1;
	}

	if (tcmi_ckpt_write(ckpt, pwd, pwdlen) < 0) {
		mdbg(ERR3, "Error writing to checkpoint");
		goto exit1;
	}

	mdbg(INFO4, "Written pwd path: %s", pwd);
	free_page(page);
	return 0;
		
	/* error handling */
 exit1:
	kfree(pwd);
 exit0:
	free_page(page);
	return -EINVAL;
}


/**
 * Loads and resotres task's pwd. It is assumed the task is already chrooted in its new root if required.
 *
 * There are 3 basic cases:
 *   1) (PEN) Migrated processes are running chrooted into the distributed file system
 *   2) (PEN) Migrated processes are running in standard PEN fs and for shared files a distributed filesystem is used. Only file in this filesystem may be used by the process
 *   3) (CCN) Process is migrated back to its UHN and it has to open real files in the fs (or files in distributed filesystem if option 2 is used)
 *   
 * Option 1) does not need any pwdmnt resolution as it is equal to new process current dir. Options 2&3 need resolution of pwdmnt by pwd path. We do the resolution in all 3 options
 * for simplicity.
 */
static inline int tcmi_ckpt_fsstruct_read(struct tcmi_ckpt *ckpt, struct task_struct* task)
{
	int32_t pwd_len, error = -EINVAL;
	char* pwd = NULL;
	struct nameidata nd;
	
	if (tcmi_ckpt_read(ckpt, &pwd_len, sizeof(pwd_len)) < 0) {
		mdbg(ERR3, "Error reading from ckpt (fsstruct pwdlen)");
		goto exit0;
	}
	
	pwd = kmalloc(pwd_len, GFP_KERNEL);
	if ( !pwd )
		return -ENOMEM;

	if (tcmi_ckpt_read(ckpt, pwd, pwd_len) < 0) {
		mdbg(ERR3, "Error reading from ckpt (fsstruct pwd)");
		goto exit0;
	}

	/* Lookup path data */
	error = path_lookup(pwd, LOOKUP_FOLLOW, &nd);
	if ( error ) {
		mdbg(ERR3, "Cannot lookup path: %s", pwd);
		goto exit0;
	}

	/* Set current process pwd */
	//path_get(&nd.path);
	task->fs->pwd = nd.path;	
	//path_put(&nd.path);

	mdbg(INFO4, "Pwd set to path: %s", pwd);

	kfree(pwd);
	return 0;
		
	/* error handling */
 exit0:
	kfree(pwd);
	return error;
}

#endif
