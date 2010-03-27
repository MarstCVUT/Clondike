/**
 * @file tcmi_ppm_p_shadow_test.c - Basic test case for shadow process and preemptive process
 *                      migration using physical checkpoint image.
 *                      - opens a listening socket on 'tcp:192.168.0.3:54321'
 *                      - instantiates a TCMI comm component for this socket. Message
 *                      delivery will be handled by a custom method that:
 *                        - ignores all non-process messages
 *                        - delivers process messages to the destination process
 *                      - user initiates a process migration by writing local PID into
 *                      a 'emigrate' ctlfs file. Upon successful migration, there
 *                      will be an entry in 'shadows' directory.
 *                      
 *
 *
 * Date: 04/26/2005
 *
 * Author: Jan Capek
 *
 *
 * License....
 */
#ifdef APP_NAME
#undef APP_NAME
#endif
#define APP_NAME SHADOW-TEST

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/spinlock.h>

#include <tcmi/comm/tcmi_comm.h>
#include <tcmi/comm/tcmi_procmsg.h>
#include <tcmi/task/tcmi_shadowtask.h>
#include <tcmi/task/tcmi_taskhelper.h>
#include <tcmi/migration/tcmi_mighooks.h>

#include <kkc/kkc.h>

#include <clondike/tcmi/tcmi_hooks.h>

#include <dbg.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

static struct tcmi_comm *comm = NULL;
static struct tcmi_ctlfs_entry *shadow_dir = NULL;
static struct tcmi_ctlfs_entry *mig_file = NULL;
static struct tcmi_ctlfs_entry *reap_file = NULL;
static struct tcmi_ctlfs_entry *root = NULL;

/* used as communication link between the shadow and guest process. */
static struct kkc_sock *new_sock;

/** This handler is invoked when the sig_unused hook finds a non-NULL entry
 * in the tcmi record in task_struct */
void shadow_mig_mode_handler(void)
{
	struct tcmi_task *tmp, *task;
	int res;
	long exit_code;

	minfo(INFO1, "Migration mode handler for task '%s' executing", 
	      current->comm);
	/* Check for a valid task in the current kernel process */
	if (!(task = tcmi_taskhelper_sanity_check())) {
		minfo(ERR3, "Stub - invalid entry to migration mode - kernel task %p, PID=%d",
		      current, current->pid);
		goto exit0;
	}

	/* keep moving the task between migration managers */
	while ((res = tcmi_task_process_methods(task)) ==
	       TCMI_TASK_MOVE_ME);
	/* check the result of the last method processed */
	switch (res) {
	case TCMI_TASK_KEEP_PUMPING:
		minfo(ERR1, "KEEP PUMPING - but no methods left %d", res);
		break;
	case TCMI_TASK_LET_ME_GO:
		minfo(ERR1, "LET ME GO - request %d", res);
		break;
	case TCMI_TASK_KILL_ME:
		minfo(ERR1, "KILL ME - request %d", res);
		tmp = tcmi_taskhelper_detach();
		/* get the exit code prior terminating. */
		exit_code = tcmi_task_exit_code(tmp);
		tcmi_task_put(tmp);
		complete_and_exit(NULL, exit_code);
		break;
		/* unlike the guest case, we don't catch the exit
		 * system call, so we have to reap the TCMI task like
		 * this */
	case TCMI_TASK_EXECVE_FAILED_KILL_ME:
		minfo(INFO1, "EXECVE_FAILED KILL ME - request %d", res);
	case TCMI_TASK_REMOVE_AND_LET_ME_GO:
		minfo(ERR1, "REMOVE AND LET ME GO - request %d", res);
		tcmi_task_put(tcmi_taskhelper_detach());
		break;
	default:
		minfo(ERR1, "Unexpected result %d", res);
		break;
	}
	/* error handling */
 exit0:
	return;
}

static int task_exit(long code)
{
	/*minfo(INFO1, "Process terminating %p '%s'", current, current->comm);*/
	return 0xff;
}

static int migr_mode(struct pt_regs *regs)

{
	tcmi_taskhelper_do_mig_mode(regs);
	return 0;
}

/** 
 * Emigrates a specified task to a new node.
 * This requires:
 * - instantiating a new shadow task
 * - submitting emigration method
 * - attaching the shadow task the target process that is
 * to be migrated
 * - forcing the target process into migration mode
 * - checking if the process has properly picked up the shadow task
 */
static int emigrate_task(void *obj, void *data)
{
	int err = 0;
	int local_pid = *((int*) data);
	struct tcmi_task *shadow;


	minfo(INFO1, "User request to migrate PID %d", local_pid);

	/* create a new PPM shadow task for physical ckpt image  */
	if (!(shadow = 
	      tcmi_shadowtask_new(local_pid, 0, new_sock, 
				  shadow_dir, root))) {
		minfo(ERR3, "Error creating a shadow task");
		goto exit0;
	}
	/* submit the emigrate method */
	mdbg(INFO3, "Submitting emigrate_ppm_p");
	tcmi_task_submit_method(shadow, tcmi_task_emigrate_ppm_p, NULL, 0);

	/* attaches the shadow to its thread. */
	if (tcmi_taskhelper_attach(shadow, shadow_mig_mode_handler) < 0) {
		minfo(ERR3, "Failed attaching shadow PID=%d to its thread", 
		      tcmi_task_local_pid(shadow));
		goto exit1;
	}


	/* switch the task to migration mode */
	tcmi_taskhelper_enter_mig_mode(shadow);

	/* wait for pickup */
	if (tcmi_taskhelper_wait_for_pick_up_timeout(shadow, 2*HZ) < 0)
		minfo(INFO1, "Shadow not picked up: %p", shadow);
	else
		minfo(INFO1, "Shadow successfully picked up: %p", shadow);

	/* release the shadow instance */
	tcmi_task_put(shadow);

	return 0;

	/* error handling */

 exit1:
	tcmi_task_put(shadow);
 exit0:
	return err;
}

/** 
 * Reaps a task specified by PID.
 * This consists of:
 * - detaching the TCMI task from its process (identified by PID)
 * - destroy the task instance.
 */
static int reap_task(void *obj, void *data)
{
	pid_t pid = *((int*) data);
	/* storage for the task that we want to reap. */
	struct tcmi_task *task;

	if (!(task = tcmi_taskhelper_detach_by_pid(pid))) {
		minfo(ERR2, "Failed detaching TCMI task from  process PID %d", pid);
		goto exit0;
	}
	/* destroy the task */
	minfo(INFO2, "Reaping TCMI task (local PID=%d, remote PID%d), process PID=%d", 
	      tcmi_task_local_pid(task), tcmi_task_remote_pid(task), pid);
	tcmi_task_put(task);
	
	return 0;

	/* error handling */
 exit0:
	return -EINVAL;
}

/** 
 * Message delivering method that is called back from the receiving
 * thread. 
 *
 * @param *obj - not used here, carries instance pointer that has been
 * passed to the TCMI comm component upon creation
 * @param *m - points to the message that is to be delivered.
 * @return 0 - when successfully delivered, otherwise the message
 * will be discarded by the caller.
 */
int ccn_deliver_method(void *obj, struct tcmi_msg *m)
{
	int err = -EINVAL;
	struct tcmi_task *task;
	/* try to deliver if the delivery fails, it is communicated
	 * back to the receiving thread and the message will be
	 * disposed. */
	minfo(INFO1, "Arrived message %x at CCN", tcmi_msg_id(m));
	if (TCMI_MSG_GROUP(tcmi_msg_id(m)) == TCMI_MSG_GROUP_PROC) {
		/* extrac the PID from the message*/
		pid_t pid = tcmi_procmsg_dst_pid(TCMI_PROCMSG(m));
		minfo(INFO1, "CCN proc message arrived as expected for PID=%d", pid);

		/* find task by PID */
		task = tcmi_taskhelper_find_by_pid(pid);
		err = tcmi_task_deliver_msg(task, m);
		tcmi_task_put(task);
	}
	else
		minfo(INFO1, "Unexpected message type");

	return 0;
}


/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_ppm_p_shadow_test_init(void)
{
	int err = -EINVAL;
	struct kkc_sock *sock1 = NULL;
	tcmi_mighooks_init();
	/* tcmi_hooks_register_exit(task_exit);
	   tcmi_hooks_register_sig_unused(migr_mode); */

	if (kkc_listen(&sock1, "tcp:192.168.0.5:54321")) {
		minfo(ERR1, "Failed listening on TCP, shouldn't fail.. ");
		goto exit0;
	}
	/* Accept the first incoming connection */
	if ((err = kkc_sock_accept(sock1, &new_sock, KKC_SOCK_BLOCK)) < 0) {
		minfo(ERR1, "Accept terminated with error code: %d",err); 
		goto exit1;
	}
	minfo(INFO1, "Accepted incoming connection, terminating listening on: '%s'",
	      kkc_sock_getsockname2(sock1));

	if (!(comm = tcmi_comm_new(new_sock, ccn_deliver_method, NULL, 
				   TCMI_MSG_GROUP_ANY))) {
		minfo(ERR2, "Failed to create TCMI comm component!");
		goto exit2;
	}
	
	if (!(root = tcmi_ctlfs_get_root())) {
		minfo(ERR2, "Failed to get root directory!");
		goto exit3;
	}
	if (!(shadow_dir = tcmi_ctlfs_dir_new(root, 0750, "shadows"))) {
		minfo(ERR2, "Failed to create shadow directory!");
		goto exit4;
	}
	if (!(mig_file = 
	      tcmi_ctlfs_intfile_new(shadow_dir, TCMI_PERMS_FILE_W,
				     NULL, NULL, emigrate_task, 
				     sizeof(int), "emigrate"))) {
		minfo(ERR2, "Can't create 'emigrate' file");
		goto exit5;
	}
	if (!(reap_file = 
	      tcmi_ctlfs_intfile_new(shadow_dir, TCMI_PERMS_FILE_W,
				     NULL, NULL, reap_task, 
				     sizeof(int), "reap"))) {
		minfo(ERR2, "Can't create 'reap' file");
		goto exit6;
	}


	/* destroy the listening socket*/
	kkc_sock_put(sock1);	
	return 0;

	/* error handling */
 exit6:
	tcmi_ctlfs_file_unregister(mig_file);
	tcmi_ctlfs_entry_put(mig_file);
 exit5:
	tcmi_ctlfs_entry_put(shadow_dir);
 exit4:
	tcmi_ctlfs_put_root();
 exit3:
	tcmi_comm_put(comm);    
 exit2:
	kkc_sock_put(new_sock);
 exit1:
	kkc_sock_put(sock1);
 exit0:
	/* tcmi_hooks_unregister_exit();
	   tcmi_hooks_unregister_sig_unused(); */
	return err;
}

/**
 * Module cleanup, stops the message receiving
 * thread, cleans transactions and cleans all messages
 * in the queue.
 */
static void __exit tcmi_ppm_p_shadow_test_exit(void)
{
	minfo(INFO1, "TCMI shadow task test terminating");
	tcmi_mighooks_exit();
/*	tcmi_hooks_unregister_exit();
	tcmi_hooks_unregister_sig_unused(); */

	tcmi_ctlfs_file_unregister(mig_file);
	tcmi_ctlfs_entry_put(mig_file);

	tcmi_ctlfs_file_unregister(reap_file);
	tcmi_ctlfs_entry_put(reap_file);

	tcmi_ctlfs_entry_put(shadow_dir);

	tcmi_comm_put(comm);
	kkc_sock_put(new_sock);

	tcmi_ctlfs_put_root();
}


module_init(tcmi_ppm_p_shadow_test_init);
module_exit(tcmi_ppm_p_shadow_test_exit);



