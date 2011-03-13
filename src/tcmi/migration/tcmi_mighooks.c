/**
 * @file tcmi_mighooks.c - a separate module that install migration hooks into the kernel.
 *                      
 * 
 *
 *
 * Date: 05/06/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_mighooks.c,v 1.11 2009-04-06 21:48:46 stavam2 Exp $
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

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/resource.h>

#include <tcmi/task/tcmi_taskhelper.h>
#include <tcmi/comm/tcmi_vfork_done_procmsg.h>
#include <tcmi/manager/tcmi_ccnman.h>
#include <tcmi/manager/tcmi_penman.h>
#include <clondike/tcmi/tcmi_hooks.h>

#define TCMI_MIGHOOKS_PRIVATE
#include "tcmi_mighooks.h"
#include "tcmi_npm_params.h"
#include <arch/current/regs.h>

#include <dbg.h>
#include <director/director.h>
#include <proxyfs/proxyfs_server.h>
#include <proxyfs/proxyfs_helper.h>

/** 
 * \<\<public\>\> Registers all hooks with the kernel.
 *
 * @return 0 upon success
 */
int tcmi_mighooks_init(void)
{
	minfo(INFO1, "Registering TCMI migration hooks");
	tcmi_hooks_register_exit(tcmi_mighooks_do_exit);
	tcmi_hooks_register_sig_unused(tcmi_mighooks_mig_mode);
	tcmi_hooks_register_execve(tcmi_mighooks_execve);
	tcmi_hooks_register_replace_proc_self_file(tcmi_mighooks_replace_proc_self_file);
	return 0;
}

/** 
 * \<\<public\>\> Unregisters all hooks.
 */
void tcmi_mighooks_exit(void)
{
	minfo(INFO1, "Unregistering TCMI migration hooks");
	tcmi_hooks_unregister_exit();
	tcmi_hooks_unregister_sig_unused();
	tcmi_hooks_unregister_execve();
	tcmi_hooks_unregister_replace_proc_self_file();
}

/** @addtogroup tcmi_mighooks_class
 *
 * @{
 */


// TODO: Temporary 64 bit debug.. make it either arch independed or remove it
/*
static void dump_user_stack(void) {
        struct frame_head {
                struct frame_head * ebp;
                unsigned long ret;
        } __attribute__((packed)) stack_frame[2], *head;

        struct pt_regs *nregs = task_pt_regs(current);

        mdbg(INFO1,"[user] <%lx> EBP <%lx>\n ESP <%lx>", nregs->rip, nregs->rbp, nregs->rsp);

        head = (struct frame_head *)nregs->rbp;

        if (user_mode_vm(nregs)) {
                do {
			if (!access_ok(VERIFY_READ, head, sizeof(stack_frame)))
                 		return;

         		if (__copy_from_user_inatomic((char *)stack_frame, head, sizeof(stack_frame)))
				return;

                        mdbg(INFO1,"[user] <%lx> EBP: <%lx>\n", stack_frame[0].ret, stack_frame[0].ebp);

                        if (head >= stack_frame[0].ebp)
		                return;

                        head = stack_frame[0].ebp;
                        } while (head->ebp);
                mdbg(INFO1,"[user] <%lx>\n", head->ret);
        }
}
*/

/**
 * \<\<private\>\> Exit system call hook.  Delegates work to the task
 * helper that notifies current process by submitting a task_exit
 * method and supplying the exit code.  The task handles the rest on
 * its own.
 *
 * The priority notification is used, so that the exit follows
 * immediately.
 *
 * This hook is called for every process in the system, but does
 * something only for TCMI tasks.
 *
 * @param code - process exit code
 * @return - status of how the notification went.
 */
static long tcmi_mighooks_do_exit(long code)
{
	int prio = 1, is_shadow;
	struct rusage rusage;
	mm_segment_t old_fs;
	long res;
	
	is_shadow = (current->tcmi.task_type == shadow || current->tcmi.task_type == shadow_detached);

	old_fs = get_fs();
	set_fs(get_ds());
	res = getrusage(current, RUSAGE_SELF, &rusage);
	set_fs(old_fs);
	director_task_exit(current->pid, code, &rusage);
	
	res = tcmi_taskhelper_notify_current(tcmi_task_exit, &code, 
					      sizeof(code), prio);

	if ( is_shadow ) { // If the exitting task is a shadow task, we release all its proxy files as they won't be needed
		proxyfs_sync_files_on_exit(); // We have to synchronize all files first so that no new writes occur after server release
		proxyfs_server_release_all();
	}

	return res;
}


/**
 * \<\<private\>\> This hook is called upon SIGUNUSED delivery to a
 * particular process.  All work is delegated to the task helper. The
 * context passed onto the task helper is used for updating the TCMI
 * task. Any potential migration request thus operates on the latest
 * process context.
 *
 * @param *regs - process context right before entering kernel mode.
 */
static long tcmi_mighooks_mig_mode(struct pt_regs *regs)
{
	tcmi_taskhelper_do_mig_mode(regs);
	return 0;
}

static void tcmi_try_npm_on_exec(const char *filename, char __user * __user * argv, 
		char __user * __user * envp, struct pt_regs *regs, 
		struct tcmi_man* man, int is_guest) 
{
	struct rusage rusage;
	mm_segment_t old_fs;
	int migman_to_use = -1;
	int migrate_home = 0;

	old_fs = get_fs();
	set_fs(get_ds());
	getrusage(current, RUSAGE_SELF, &rusage);
	set_fs(old_fs);
	if (director_npm_check(current->pid, current_euid(), is_guest, filename, argv, envp, &migman_to_use, &migrate_home, &rusage) != 1 )
		return;

	migrate_home = is_guest ? migrate_home : 0;

	// Check if we should emigrate and if so where
	if ( migman_to_use != -1 || migrate_home ) {
		struct tcmi_npm_params* npm_params = vmalloc(sizeof(struct tcmi_npm_params));
		int err = extract_tcmi_npm_params(npm_params, filename, argv, envp);	
		mdbg(INFO4, "AFTER EXTRACT Filename: %s, Argc: %d, Evnpc: %d.", npm_params->file_name, npm_params->argsc, npm_params->envpc);
		if ( err ) {
			mdbg(ERR3, "Error extracting npm params: %d. Skipping npm emigration attempt.", err);
			vfree(npm_params);
		} else {
			if ( migrate_home )
				tcmi_penman_migrateback_npm(man, regs, npm_params);
			else
				tcmi_man_emig_npm(man, current->pid, migman_to_use, regs, npm_params);
			// If we got here, the migration was not performed => free npm params
			mdbg(INFO3, "Freeing npm_params after unsuccessful npm migration attempt.");
			vfree(npm_params);
		}
	}
}

static long tcmi_mighooks_execve(char *filename, char __user * __user * argv, char __user * __user * envp, struct pt_regs *regs) {	
	if ( !current->tcmi.tcmi_task ) {
		// The task is not controlled by clondike => we can non-preemptively migrate it ONLY if there is CCN registered on current node
		struct tcmi_ccnman* ccn_man = tcmi_ccnman_get_instance();
		if ( ccn_man != NULL )
			tcmi_try_npm_on_exec(filename, argv, envp, regs, TCMI_MAN(ccn_man), 0);
	} else if ( tcmi_task_get_type(current->tcmi.tcmi_task) == guest ) {
		// We allow npm only after we get initial execve executed.. not during the initial image load!
		if ( tcmi_task_get_execve_count(current->tcmi.tcmi_task) > 1 ) {
			// Task is guest, so DN has to take care of its possible NPM
			struct tcmi_penman* pen_man = tcmi_penman_get_instance();
	
			mdbg(INFO3, "Guest exec hook called. File %s", filename);
			tcmi_try_npm_on_exec(filename, argv, envp, regs, TCMI_MAN(pen_man), 1);
		} else {
			// On first exceve, we notify about possible vfork done
			// TODO: We may optimize this, and notify only in case it is required.. this would require some flag in tcmi_task, that would be caried here during emigration
			struct tcmi_msg *m;
			// We do not aquire reference to t_task as it cannot be release at this moment
			struct tcmi_task* t_task = current->tcmi.tcmi_task;
		
			if ( !t_task) {
				mdbg(ERR3, "No tcmi task associated!");
				goto exit0;
			}

			if (!(m = tcmi_vfork_done_procmsg_new_tx(t_task->remote_pid))) {
				mdbg(ERR3, "Can't create vfork done message");
				// TODO: What to do in this case? ;) For now we just ignore it.
				goto exit0;
			}

		
			tcmi_task_send_anonymous_msg(t_task, m);
			tcmi_msg_put(m);
			
			exit0:
			;
		}
		tcmi_task_inc_execve_count(current->tcmi.tcmi_task);
	} else {
		// We are shadow task => ignore this hook, shadow execves only when restarting process that was migrated back
		tcmi_task_inc_execve_count(current->tcmi.tcmi_task);
	}

	return 0;
};


static long tcmi_mighooks_replace_proc_self_file(const char* filename, const char** result) {	
	if ( strncmp("/proc/self",filename,10) == 0 ) {
		int len;
		char * tmp_result = __getname();
	
		if ( !(tmp_result) )
			return -ENOMEM;

		//len = sprintf((tmp_result), "/proc/%d%s", tcmi_task_remote_pid(current->tcmi.tcmi_task), filename+10);
		len = sprintf((tmp_result), "/mnt/local/proc/self%s", filename+10);

		mdbg(ERR3, "Replaced open filename from %s to %s (len: %d)", filename, (tmp_result), len);
		*result = tmp_result;
	}	

	return 0;
}

/**
 * @}
 */
