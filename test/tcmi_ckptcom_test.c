/**
 * @file tcmi_ckptcom_test.c - Basic test case for the checkpointing component
 *                      - installs a migration mode handler
 *                      - installs a new binary format that supports checkpoiting
 *                      - the handler upon execution creates a checkpoint
 *                      of a process
 *
 *
 * Date: 05/01/2005
 *
 * Author: Jan Capek
 *
 *
 * License....
 */
#ifdef APP_NAME
#undef APP_NAME
#endif
#define APP_NAME CKPTCOM-TEST

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>

#include <tcmi/ckpt/tcmi_ckptcom.h>

#include <clondike/tcmi/tcmi_hooks.h>

#include <dbg.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");


/** This handler is invoked when the sig_unused hook finds a non-NULL entry
 * in the tcmi record in task_struct */
static int ckpt_mig_mode_handler(struct pt_regs *regs)
{
	struct inode *inode;
	struct file *file;

	/* error handling */
	file = filp_open("/tmp/testcheckpoint", 
			 O_CREAT | O_RDWR  | O_NOFOLLOW | O_LARGEFILE | O_TRUNC, 0700);
	if (IS_ERR(file))
		goto exit0;
	inode = file->f_dentry->d_inode;
	if (inode->i_nlink > 1)
		goto exit1;	/* multiple links - don't dump */
	if (d_unhashed(file->f_dentry))
		goto exit1;

	if (!S_ISREG(inode->i_mode))
		goto exit1;
	if (!file->f_op)
		goto exit1;
	if (!file->f_op->write)
		goto exit1;
	/* if (do_truncate(file->f_dentry, 0) != 0)
	   goto exit1; */

	tcmi_ckptcom_checkpoint_ppm(file, regs, 0);
 exit1:
	filp_close(file, NULL);	
 exit0:
	return 0;
}


/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_ckptcom_test_init(void)
{
	tcmi_hooks_register_sig_unused(ckpt_mig_mode_handler);
	return 0;
}

/**
 * Module cleanup, stops the message receiving
 * thread, cleans transactions and cleans all messages
 * in the queue.
 */
static void __exit tcmi_ckptcom_test_exit(void)
{
	tcmi_hooks_unregister_sig_unused();

}


module_init(tcmi_ckptcom_test_init);
module_exit(tcmi_ckptcom_test_exit);



