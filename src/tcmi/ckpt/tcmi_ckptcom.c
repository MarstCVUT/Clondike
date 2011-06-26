/**
 * @file tcmi_ckptcom.c - Process checkpointing component.
 *                      
 * 
 *
 *
 * Date: 05/01/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ckptcom.c,v 1.14 2008/01/17 14:36:44 stavam2 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2005  Jan Capek
 * 
 * TCMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * TCMI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <linux/module.h>
#include <linux/fs_struct.h>
#include <linux/fdtable.h>

#include "tcmi_ckpt.h"
#include "tcmi_ckpt_mm.h"
#include "tcmi_ckpt_regs.h"
#include "tcmi_ckpt_thread.h"
#include "tcmi_ckpt_fsstruct.h"
#include "tcmi_ckpt_resources.h"

#define TCMI_CKPTCOM_PRIVATE
#include "tcmi_ckptcom.h"
#include "tcmi_ckpt_sig.h"
#include "tcmi_ckpt_npm_params.h"

#include <arch/current/restart_fixup.h>
#include <linux/vmalloc.h>

/** 
 * \<\<private\>\> Helper method that handles both PPM and NPM checkpoint creation
 * 
 * This consists of:
 * - creating a new checkpoint instance 
 * - writing a checkpoint header - might fail if the process can't be
 * checkpointed (e.g. has open files or memory areas that are not
 * supported)
 * - writing open files
 * - writing memory descriptor
 * - writing memory areas along with pages
 * - writing process state (registers)
 * - writing signal handlers
 *
 * @param *file - file where the checkpoint is to be stored
 * @param *regs - registers of the checkpointed process
 * @param heavy - if set a full checkpoint of all process pages
 * is made, all memory mapped files will be then restored
 * from the checkpoint image. Setting this options yields
 * a bigger checkpoint image size..
 * @param npm_params - Non-preemptive checkpoint params, or null if we are doing PPM checkpoint
 *
 * @return 0 upon success
 */
static int tcmi_ckptcom_checkpoint(struct file *file, struct pt_regs *regs,
			    int heavy, struct tcmi_npm_params* npm_params)
{
	struct tcmi_ckpt *ckpt;
	int is_npm = npm_params != NULL;
	u64 beg_time, end_time;
	
	beg_time = cpu_clock(smp_processor_id());

	mdbg(INFO3, "Start checkpointing. Is_npm: %d", is_npm);
	if ( !regs ) {
		mdbg(ERR3, "Failed to create a checkpoint file. No regs provided");
		goto exit0;
	}
	
	ckpt = tcmi_ckpt_new(file);
	if ( IS_ERR(ckpt) ) {
		mdbg(ERR3, "Failed to create a checkpoint file. Err: %ld", PTR_ERR(ckpt));
		goto exit0;
	}
	if ( ckpt == NULL ) {
		mdbg(ERR3, "Failed to create a checkpoint file.");
		goto exit0;
	}

	if (tcmi_ckpt_write_hdr(ckpt, is_npm) < 0) {
		mdbg(ERR3, "Error writing checkpoint header!");
		goto exit1;
	}
	if (tcmi_ckpt_write_rlimit(ckpt, current) < 0) {
		mdbg(ERR3, "Error writing checkpoint rlimit!");
		goto exit1;
	}
	if (tcmi_ckpt_write_files(ckpt) < 0) {
		mdbg(ERR3, "Error writing checkpoint files!");
		goto exit1;
	}
	if (tcmi_ckpt_mm_write(ckpt) < 0) {
		mdbg(ERR3, "Error writing memory descriptor!");
		goto exit1;
	}
	
	if ( !is_npm ) {
		if (tcmi_ckpt_write_vmas(ckpt, heavy) < 0) {
			mdbg(ERR3, "Error writing VM areas type: %d", heavy);
			goto exit1;
		}
	}

	if (tcmi_ckpt_regs_write(ckpt, regs) < 0) {
		mdbg(ERR3, "Error writing processor registers descriptor!");
		goto exit1;
	}
	if (tcmi_ckpt_tls_write(ckpt, current) < 0) {
		mdbg(ERR3, "Error writing process tls!");
		goto exit1;
	}
	if (tcmi_ckpt_fsstruct_write(ckpt, current) < 0) {
		mdbg(ERR3, "Error writing process fs struct!");
		goto exit1;
	}
	if (tcmi_ckpt_sig_write(ckpt) < 0) {
		mdbg(ERR3, "Error writing signal data!");
		goto exit1;
	}

	if ( is_npm ) {
		if (tcmi_ckpt_npm_params_write(ckpt, npm_params) < 0) {
			mdbg(ERR3, "Error writing npm params!");
			goto exit1;
		}	
	}

	end_time = cpu_clock(smp_processor_id());
	mdbg(INFO3, "Checkpoint (npm: %d) took '%llu' ms.'", is_npm, (end_time - beg_time) / 1000000);
	printk("Checkpoint (npm: %d) took '%llu' ms.\n'", is_npm, (end_time - beg_time) / 1000000);


	tcmi_ckpt_put(ckpt);
	return 0;

	/* error handling */
 exit1:
	tcmi_ckpt_put(ckpt);
 exit0:
	return -ENOEXEC;
}

/** \<\<public\>\> Creates a preemptive process checkpoint. */
int tcmi_ckptcom_checkpoint_ppm(struct file *file, struct pt_regs *regs, int heavy) {
	return tcmi_ckptcom_checkpoint(file, regs, heavy, NULL);
}

/** \<\<public\>\> Creates a non-preemptive process checkpoint. */
int tcmi_ckptcom_checkpoint_npm(struct file *file, struct pt_regs *regs, struct tcmi_npm_params* params) {
	return tcmi_ckptcom_checkpoint(file, regs, 0, params);
}

/** 
 * \<\<public\>\> Restarts a process from a checkpoint (new binfmt
 * handler).  This consists of:
 *
 * - creating a new checkpoint instance 
 * - reading a checkpoint header - might fail, if the magic number
 * doesn't match.
 * - reading open files
 * - reading memory descriptor
 * - reading memory areas along with pages
 * - reading process state (registers)
 * - reading signal handlers
 *
 * @param *bprm - binary object that is passed to this method by
 * execve and contains a pointer to the executable file
 * @param *regs - references the registers of the process, so that
 * they can be overlayed from the checkpoint.
 * @return 0 upon success
 */
int tcmi_ckptcom_restart(struct linux_binprm *bprm, struct pt_regs *regs)
{
	struct tcmi_ckpt *ckpt;
	struct pt_regs* original_regs;	
//	int i;	
	u64 beg_time, end_time;
	
	beg_time = cpu_clock(smp_processor_id());
	

	memory_sanity_check("Start");
	
	original_regs = kmalloc(sizeof(struct pt_regs), GFP_KERNEL);
	if ( !original_regs )
		return -ENOMEM;

	if (!tcmi_ckpt_check_magic(bprm->buf)) {
		goto exit0;
	}
	
	mdbg(INFO3, "Restarting '%s'", bprm->filename);
	if (!(ckpt = tcmi_ckpt_new(bprm->file))) {
		mdbg(ERR3, "Failed to instantiate a checkpoint");
		goto exit0;
	}
	if (tcmi_ckpt_read_hdr(ckpt) < 0) {
		mdbg(ERR3, "Error reading checkpoint header!");
		goto exit1;
	}
memory_sanity_check("Post header");
	/* Flush all traces of the currently running executable */
	if (flush_old_exec(bprm)) {
		mdbg(ERR3, "Error flushing the old execution context!");
		goto exit0;
	}
memory_sanity_check("Pre-rlimit");
	if (tcmi_ckpt_read_rlimit(ckpt, current) < 0) {
		mdbg(ERR3, "Error reading checkpoint rlimit!");
		goto exit1;
	}
memory_sanity_check("Post-rlimit");
	if ( FD_ISSET(0, current->files->fdt->open_fds) ) {
		mdbg(INFO3, "Closing open fs.. this should not happend though..");
		sys_close(0);
	}
memory_sanity_check("Pre files");
	if (tcmi_ckpt_read_files(ckpt) < 0) {
		mdbg(ERR3, "Error reading checkpoint files!");
		goto exit1;
	}
memory_sanity_check("Post - files");
	if (tcmi_ckpt_mm_read(ckpt) < 0) {
		mdbg(ERR3, "Error reading memory descriptor!");
		goto exit1;
	}
memory_sanity_check("Post mm");
	if ( !ckpt->hdr.is_npm ) {
		if (tcmi_ckpt_read_vmas(ckpt) < 0) {
			mdbg(ERR3, "Error reading VM areas");
			goto exit1;
		}
	}

	*original_regs = *regs;
	if (tcmi_ckpt_regs_read(ckpt, regs) < 0) {
		mdbg(ERR3, "Error reading processor registers descriptor!");
		goto exit1;
	}
	if (tcmi_ckpt_tls_read(ckpt, current, regs) < 0) {
		mdbg(ERR3, "Error reading process tls!");
		goto exit1;
	}
	if (tcmi_ckpt_fsstruct_read(ckpt, current) < 0) {
		mdbg(ERR3, "Error reading process fsstruct!");
		goto exit1;
	}
	if (tcmi_ckpt_sig_read(ckpt) < 0) {
		mdbg(ERR3, "Error reading signals informations!");
		goto exit1;
	}
	if ( ckpt->hdr.is_npm ) {
		struct tcmi_npm_params* params = vmalloc(sizeof(struct tcmi_npm_params));
		int exec_result = -EFAULT;
		mm_segment_t old_fs;
		*regs = *original_regs;

		if ( !params ) {
			mdbg(ERR3, "Cannot allocate memory for npm params!");
			goto exit1;
		}
		

		if (tcmi_ckpt_npm_params_read(ckpt, params) < 0) {
			mdbg(ERR3, "Error reading npm params!");
			goto exit1;
		}				
		tcmi_ckpt_put(ckpt);

		// TEMPORARY DEBUG!
		if ( current->mm )
			mdbg(INFO2, "MM %p nr_ptes: %lu", current->mm, current->mm->nr_ptes);
		if ( current->active_mm )
			mdbg(INFO2, "ACTIVE %p MM nr_ptes: %lu", current->active_mm, current->active_mm->nr_ptes);

		// TODO: This is something REALLY NASTY! Some better solution is appreciated
		if ( current->mm ) {
			current->mm->nr_ptes = 0;
		}

		old_fs = get_fs();
		set_fs(KERNEL_DS);		

		// TEMPORARY DEBUG!
		if ( current->mm )
			mdbg(INFO2, "MM %p nr_ptes: %lu", current->mm, current->mm->nr_ptes);
		if ( current->active_mm )
			mdbg(INFO2, "ACTIVE %p MM nr_ptes: %lu", current->active_mm, current->active_mm->nr_ptes);

		// We have to unlock temprarily guar to prevent recursive lock (we are calling recursive exceve). TODO: Some better solution?
		mutex_unlock(&current->cred_guard_mutex);
		current->fs->in_exec = 0; // Required to pass through 'check_unsafe_exec'

		mdbg(INFO3, "Going to execute '%s', Args: %d, Envps %d", params->file_name, params->argsc, params->envpc);
		//mdbg(INFO3, "Arg[0] '%s', Envp[0] '%s'", params->args[0], params->envp[0]);
		
		exec_result = do_execve(params->file_name, (char __user *__user *)params->args, (char __user *__user *)params->envp, regs);
		mdbg(INFO3, "NPM internal execution result %d", exec_result);

		// And now we relock again as the relock of outer execve will be attempted.
		if (mutex_lock_interruptible(&current->cred_guard_mutex)) {
			minfo(ERR3, "Failed to relock cred guard!");		
		}				

		// TODO: How do we release reference to the binmt of the module that was used for this execve.. ?
		// Do we have to call "set_binfmt(&tcmi_ckptcom_format);" here or can we release it here?
		set_fs(old_fs);

		vfree(params);
		mdbg(INFO3, "NPM after param free");		

		if ( exec_result ) {// In case of error of internal execution, we have to return ENOEXEC
			return -EFAULT;
		}

		end_time = cpu_clock(smp_processor_id());
		mdbg(INFO3, "Checkpoint NPM took '%llu' ms.'", (end_time - beg_time) / 1000000);
		printk("Checkpoint NPM took '%llu' ms.\n'", (end_time - beg_time) / 1000000);

		return 0;
	} else {
		// Restart fixup is performed only in PPM
		tcmi_resolve_restart_block(current, regs, ckpt->hdr.checkpoint_arch, ckpt->hdr.is_32bit_application);
	}
	kfree(original_regs);
	
	/* flush_signals(current);*/
	tcmi_ckpt_put(ckpt);
	/* successul execution of the image - need to set the format */
	set_binfmt(&tcmi_ckptcom_format);

	mdbg(INFO3, "Checkpoint PPM took '%llu' ms.'", (end_time - beg_time) / 1000000);
	printk("Checkpoint PPM took '%llu' ms.\n'", (end_time - beg_time) / 1000000);

	/* Something went wrong, return the inode and free the argument pages*/
/* 
What is the point of the following code? It does not really seem to free memory, but in any case, it does not compile on latest kernels ;)
	for (i = 0 ; i < MAX_ARG_PAGES ; i++) {
		bprm->page[i] = NULL;
	}
*/

	return 0;

	/* error handling */
 exit1:
	tcmi_ckpt_put(ckpt);
 exit0:
	return -ENOEXEC;
}

/**
 * Core dumping function.
 * Currently just logs some process data.
 *
 * @TODO: We should somehow keep track of the original binary format and locate corresponding dump function here and perform original binmt dump
 */
static int tcmi_core_dump(struct coredump_params *cprm) {
	debug_registers(cprm->regs);
	return 0;
}


/** 
 * Initializes the migration component.  This requires registering a
 * new binary format with the kernel.
 *
 * @return 0 upon success
 */
static int __init tcmi_ckptcom_init(void)
{
	return register_binfmt(&tcmi_ckptcom_format);
}

/** 
 * Shutdown for the migration component.  This requires unregistering
 * a new binary format with the kernel.
 */
static void __exit tcmi_ckptcom_exit(void)
{
	unregister_binfmt(&tcmi_ckptcom_format);
}


/** @addtogroup tcmi_ckptcom_class
 *
 * @{
 */
/** Initialize the new checkpoint binary format  */
static struct linux_binfmt tcmi_ckptcom_format = { 
	.module = THIS_MODULE, 
	.load_binary = tcmi_ckptcom_restart,	
	.core_dump = tcmi_core_dump,
	.min_coredump = 0
};

/**
 * @}
 */


module_init(tcmi_ckptcom_init);
module_exit(tcmi_ckptcom_exit);

EXPORT_SYMBOL_GPL(tcmi_ckptcom_checkpoint_ppm);
EXPORT_SYMBOL_GPL(tcmi_ckptcom_checkpoint_npm);
EXPORT_SYMBOL_GPL(tcmi_ckptcom_restart);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");
