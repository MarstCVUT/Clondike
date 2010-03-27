/**
 * @file tcmi_ckpt_resources.h - a helper class that provides functionality to checkpoint process rlimits
 */
#ifndef _TCMI_CKPT_RESOURCES_H
#define _TCMI_CKPT_RESOURCES_H

#include <asm/uaccess.h>
#include <linux/resource.h>

#include "tcmi_ckpt.h"

struct tcmi_ckpt_rlimit {
          u_int64_t   rlim_cur;
          u_int64_t   rlim_max;
}  __attribute__((__packed__));

struct tcmi_ckpt_rlimits {
	struct tcmi_ckpt_rlimit rlim[RLIM_NLIMITS];
}  __attribute__((__packed__));

/** 
 * Writes a current process rlimits into the file
 *
 * @param *ckpt - checkpoint file where the area is to be stored
 * @param *task - current task
 * @return 0 upon success.
 */
static inline int tcmi_ckpt_write_rlimit(struct tcmi_ckpt *ckpt, struct task_struct* task)
{	
	struct tcmi_ckpt_rlimits rlims;
	int i;	

	for ( i=0; i < RLIM_NLIMITS; i++ ) {
		rlims.rlim[i].rlim_cur = task->signal->rlim[i].rlim_cur;
		rlims.rlim[i].rlim_max = task->signal->rlim[i].rlim_max;	
	} 

	if (tcmi_ckpt_write(ckpt, &rlims, sizeof(rlims)) < 0) {
		mdbg(ERR3, "Error writing resource limits");
		goto exit0;
	}

	mdbg(INFO4, "Written resouce limits.");

	return 0;
		
	/* error handling */
 exit0:
	return -EINVAL;

}

/** 
 * Reads a process rlimits
 *
 * @param *ckpt - checkpoint file where the area is stored
 * @param *task - current task
 * @return 0 upon success.
 */
static inline int tcmi_ckpt_read_rlimit(struct tcmi_ckpt *ckpt, struct task_struct* task)
{
	struct tcmi_ckpt_rlimits rlims;
	int i;	

	if (tcmi_ckpt_read(ckpt, &rlims, sizeof(rlims)) < 0) {
		mdbg(ERR3, "Error reading process limits");
		goto exit0;
	}

	for ( i=0; i < RLIM_NLIMITS; i++ ) {
		task->signal->rlim[i].rlim_cur = CHECKED_UINT64_TO_ULONG(rlims.rlim[i].rlim_cur);
		task->signal->rlim[i].rlim_max = CHECKED_UINT64_TO_ULONG(rlims.rlim[i].rlim_max);
	} 

	mdbg(INFO4, "Read process limits.");

	return 0;
		
	/* error handling */
 exit0:
	return -EINVAL;
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CKPT_RESOURCES_PRIVATE


#endif /* TCMI_CKPT_RESOURCES_PRIVATE */

/**
 * @}
 */

#endif /* _TCMI_CKPT_RESOURCES_H */

