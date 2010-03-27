/**
 * @file tcmi_ppm_p_guest_test.c - Basic test case for a guest process and preemptive process
 *                      migration using physical checkpoint image.
 *                      - opens a connection socket on 'tcp:192.168.0.3:54321'
 *                      - instantiates a TCMI comm component for this socket. Message
 *                      delivery will be handled by a custom method that:
 *                        - delivers migration messages to the message processing thread                        
 *                        - delivers process messages to the destination process
 *                      - message processing thread(module init function):
 *                          handles only 1 migration message - P_EMIGRATE_MSG. This
 *                        message causes:
 *                        - instantiation of a new guest task
 *                        - adding this message to its message queue
 *                        - scheduling process_msg into the guest method queue
 *                        - creation of a new thread on whose behalf the migrated process
 *                        will be executing.
 *                      
 *
 *
 * Date: 04/27/2005
 *
 * Author: Jan Capek
 *
 *
 * License....
 */
#ifdef APP_NAME
#undef APP_NAME
#endif
#define APP_NAME STUB-TEST

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
#include <linux/completion.h>

#include <tcmi/comm/tcmi_comm.h>
#include <tcmi/comm/tcmi_procmsg.h>
#include <tcmi/task/tcmi_guesttask.h>
#include <tcmi/task/tcmi_taskhelper.h>

#include <kkc/kkc.h>

#include <clondike/tcmi/tcmi_hooks.h>

#include <dbg.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

static struct tcmi_comm *comm = NULL;
static struct tcmi_ctlfs_entry *guest_dir = NULL;
static struct tcmi_ctlfs_entry *migback_file = NULL;
static struct tcmi_ctlfs_entry *root = NULL;

/* used as communication link between the shadow and guest process. */
static struct kkc_sock *sock1;

/* transactions and message queue are shared among the main and receiving thread */
static struct tcmi_slotvec *transactions;
static struct tcmi_queue queue;


/** This handler is invoked when the sig_unused hook finds a non-NULL entry
 * in the clondike record in the task_struct */
void guest_mig_mode_handler(void)
{
	struct tcmi_task *tmp, *task;
	int res;
	long exit_code;

	minfo(INFO1, "Migration mode handler for task '%s' executing", 
	      current->comm);


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
		minfo(INFO1, "KEEP PUMPING - but no methods left %d", res);
		break;
	case TCMI_TASK_LET_ME_GO:
		minfo(INFO1, "LET ME GO - request %d", res);
		break;
		/* no need for special handling a fall through causes
		 * do_exit which sends a message to the shadow */
	case TCMI_TASK_EXECVE_FAILED_KILL_ME:
		minfo(INFO1, "EXECVE_FAILED KILL ME - request %d", res);
		break;
	case TCMI_TASK_KILL_ME:
		minfo(INFO1, "KILL ME - request %d", res);
		tmp = tcmi_taskhelper_detach();
		/* get the exit code prior terminating. */
		exit_code = tcmi_task_exit_code(tmp);
		tcmi_task_put(tmp);
		complete_and_exit(NULL, exit_code);
		break;
	case TCMI_TASK_REMOVE_AND_LET_ME_GO:
		minfo(INFO1, "REMOVE AND LET ME GO - request %d", res);
		tcmi_task_put(tcmi_taskhelper_detach());
		break;
	default:
		minfo(ERR1, "Unexpected result %d.", res);
		break;
	}
	/* error handling */
 exit0:
	return;
}

static int task_exit(long code)
{
	struct tcmi_task *task = current->tcmi.tcmi_task;
	if (!((task && 
	      (current->tcmi.mig_mode_handler == guest_mig_mode_handler)))) 
		goto exit0;
	/* schedule task exit */
	mdbg(INFO3, "Submitting tcmi_task_exit");
	tcmi_task_flush_and_submit_method(task, tcmi_task_exit, &code, sizeof(code));
	mdbg(INFO3, "Deleting TCMI guest task PID %d pointer=%p", current->pid, task);
	/* tcmi_task_put(task); */
	mdbg(INFO3, "Deleted TCMI guest task PID %d", current->pid);
	/* invoke the migration mode handler */
 	guest_mig_mode_handler();

exit0:
	return 0xff;
}



/** 
 * Message delivering method that is called back from the receiving
 * thread. It looks up a task based on PID and delivers the message to
 * it.
 * 
 *
 * @param *obj - not used here, carries instance pointer that has been
 * passed to the TCMI comm component upon creation
 * @param *m - points to the message that is to be delivered.
 * @return 0 - when successfully delivered, otherwise the message
 * will be discarded by the caller.
 */
int pen_deliver_method(void *obj, struct tcmi_msg *m)
{
	int err = -EINVAL;
	struct tcmi_task *task;

	/* try to deliver if the delivery fails, it is communicated
	 * back to the receiving thread and the message will be
	 * disposed. */
	minfo(INFO1, "Arrived message %x at PEN", tcmi_msg_id(m));
	if (TCMI_MSG_GROUP(tcmi_msg_id(m)) == TCMI_MSG_GROUP_PROC) {
		/* try finding the task based on PID  in the message*/
		pid_t pid = tcmi_procmsg_dst_pid(TCMI_PROCMSG(m));
		minfo(INFO1, "PEN proc message arrived as expected for PID=%d", pid);
		/* find task by PID */
		task = tcmi_taskhelper_find_by_pid(pid);
		err = tcmi_task_deliver_msg(task, m);
		tcmi_task_put(task);
	}
	else {
		minfo(INFO1, "Regular migration message");
		err = tcmi_msg_deliver(m, &queue, transactions);
	}
	return err;
}


/** 
 * This method represents a kernel thread that will eventually
 * become the migrated task. It does following:
 * - waits until signalled, that the guest is ready
 * - picks up the guest task
 * - runs its migration mode handler.
 */
int migrated_task(void *data)
{
	int err = -EINVAL;
	struct completion *guest_ready = (struct completion*) data;

	minfo(INFO1, "Starting new thread for migrating task..");
	if ((err = wait_for_completion_interruptible(guest_ready))) {
		minfo(INFO1, "Received signal guest not ready");
		goto exit0;
	}
	minfo(INFO1, "Stub is ready by now..");
	/* This part simulates when the process didn't pickup
	   the shadow task on time 
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(5*HZ); 
	minfo(INFO1, "Task has woken up %p", current);
	*/
	if (!current->tcmi.mig_mode_handler) {
		minfo(ERR3, "Missing migration mode handler for guest task!!");
		goto exit0;
	}
	current->tcmi.mig_mode_handler();

	return 0;
	/* error handling */
 exit0:

	return err;
}

/** 
 * This method immigrates a new task based on the P emigrate
 * message as follows:
 * - creates a new kernel thread that will wait until the
 * new guest with its PID is created (waits with timeout on guest_ready completion)
 * - instantiates a new guest, delivers the migration message to it and 
 * makes a necessary setup, so that the guest processes the message as soon as
 * it becomes a regular migrated process
 * - attaches the guest to its new process(thread)
 * - announces that the guest is ready to the thread via completion
 * - waits for the thread to pickup the guest and become a migrated
 * process
 */
int immigrate_task(struct tcmi_msg *m)
{
	int err = -EINVAL;
	int pid;
	/* used to signal the thread the guest is ready */
	struct completion guest_ready;
	struct tcmi_task *guest;
	init_completion(&guest_ready);

	if ((pid = kernel_thread(migrated_task, &guest_ready, 0/*SIGCHLD*/)) < 0) {
		minfo(ERR3, "Cannot start kernel thread for guest process %d", pid);
		goto exit0;
	}
	if (!(guest = tcmi_guesttask_new(pid, 0,
				       sock1, guest_dir, root))) {
		minfo(ERR3, "Error creating a guest task");
		goto exit0;
	}
	/* have the guest task deliver the message to itself, extra message reference is for the guest.*/
	if (tcmi_task_deliver_msg(guest, tcmi_msg_get(m)) < 0) {
		minfo(ERR3, "Error delivering the message to the guest process");
		tcmi_msg_put(m);
	}
	/* The message will be processed first. */
	mdbg(INFO3, "Submitting process_msg");
	tcmi_task_submit_method(guest, tcmi_task_process_msg, NULL, 0);

	/* attaches the guest to its thread. */
	if (tcmi_taskhelper_attach(guest, guest_mig_mode_handler) < 0) {
		minfo(ERR3, "Failed attaching guest PID=%d to its thread", 
		      tcmi_task_local_pid(guest));
		goto exit1;
	}

	/* signal the new thread that the guest is ready. */
	complete(&guest_ready);

	/* wait for pickup */
	if (tcmi_taskhelper_wait_for_pick_up_timeout(guest, 2*HZ) < 0)
		minfo(ERR1, "Stub not picked up: %p!!!!", guest);
	else
		minfo(INFO1, "Shadow successfully picked up: %p", guest);

	tcmi_task_put(guest);
	return 0;
	/* error handling */
 exit1:
	tcmi_task_put(guest);
 exit0:
	return err;
}

/** 
 * This method is responsible for handing TCMI migration messages
 * 
 */
void guest_test_handle_msg(struct tcmi_msg *m, struct kkc_sock *sock)
{
	if (!m)
		return;
	
	switch(tcmi_msg_id(m)) {
		/* create a new PPM guest task.  */
	case TCMI_P_EMIGRATE_MSG_ID:
		minfo(INFO1, "P emigrate message has arrived..");
		if (immigrate_task(m)) {
			minfo(ERR3, "Error imigrating process");
		}

		/* let the guest finish the migration. */
		break;
	default:
		minfo(INFO1, "Unexpected message ID %x, no handler available.", 
		      tcmi_msg_id(m));
		break;
	}
	/* release the message */
	minfo(INFO1, "Done handling message %x on PEN", tcmi_msg_id(m));
	tcmi_msg_put(m);
}

static int send_sg(int signr)

{
	minfo(INFO1, "Sending signal: %d ", signr);
	return 0xfe;
}
/** 
 * Migrates a specified task back to its CCN.
 * The notification is delegated to the task helper, we specify onlhy
 * the ppm p migrate back method. The priority is set to 1, as we
 * want this method to execute immediately.
 */
static int migrback_task(void *obj, void *data)
{
	int err = 0;
	pid_t pid = *((int*) data);
	/* set priority for the method - causes all methods to be flushed */
	int prio = 1;

	if (tcmi_taskhelper_notify_by_pid(pid, tcmi_task_migrateback_ppm_p, 
					  NULL, 0, prio) < 0) {
		mdbg(ERR3, "Failed migrating back process PID %d", pid);
		err = - EINVAL;
	}
	
	return err;
}


/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_ppm_p_guest_test_init(void)
{
	int err = -EINVAL;
	/* tcmi_hooks_register_exit(task_exit); */
	/* tcmi_hooks_register_send_sig(send_sg); */

	if (kkc_connect(&sock1, "tcp:192.168.0.5:54321")) {
		minfo(ERR1, "Failed connecting on TCP, shouldn't fail.. ");
		goto exit0;
	}
	if (!(comm = tcmi_comm_new(sock1, pen_deliver_method, NULL, 
				   TCMI_MSG_GROUP_ANY))) {
		minfo(ERR2, "Failed to create TCMI comm component!");
		goto exit1;
	}
	
	if (!(root = tcmi_ctlfs_get_root())) {
		minfo(ERR2, "Failed to get root directory!");
		goto exit2;
	}
	if (!(guest_dir = tcmi_ctlfs_dir_new(root, 0750, "guests"))) {
		minfo(ERR2, "Failed to create guests directory!");
		goto exit3;
	}
	if (!(migback_file = 
	      tcmi_ctlfs_intfile_new(guest_dir, TCMI_PERMS_FILE_W,
				     NULL, NULL, migrback_task, 
				     sizeof(int), "migrate-back"))) {
		minfo(ERR2, "Can't create 'migrate-back' file");
		goto exit4;
	}

	if (!(transactions = tcmi_slotvec_hnew(10))) {
		minfo(ERR1, "Failed to create hashtable");
		goto exit5;
	}
	tcmi_queue_init(&queue);

	/* Keep on processing messages. */
	while (!signal_pending(current)) {
		int ret;
		struct tcmi_msg *m;
		/* sleep on the message queue, until a message arrives */
		if ((ret = tcmi_queue_wait_on_empty_interruptible(&queue)) < 0) {
			minfo(INFO1, "Signal arrived %d", ret);
		}
		tcmi_queue_remove_entry(&queue, m, node);
		guest_test_handle_msg(m, sock1);
	}

	return 0;

	/* error handling */
 exit5:
	tcmi_ctlfs_file_unregister(migback_file);
	tcmi_ctlfs_entry_put(migback_file);
 exit4:
	tcmi_ctlfs_entry_put(guest_dir);
 exit3:
	tcmi_ctlfs_put_root();
 exit2:
	tcmi_comm_put(comm);    
 exit1:
	kkc_sock_put(sock1);
 exit0:
/*	tcmi_hooks_unregister_exit();*/
	return err;
}

/**
 * Module cleanup, stops the message receiving
 * thread, cleans transactions and cleans all messages
 * in the queue.
 */
static void __exit tcmi_ppm_p_guest_test_exit(void)
{
	minfo(INFO1, "TCMI shadow task test terminating");
	/* tcmi_hooks_unregister_exit(); */
	/* tcmi_hooks_unregister_send_sig(); */

	tcmi_ctlfs_entry_put(guest_dir);
	tcmi_ctlfs_file_unregister(migback_file);
	tcmi_ctlfs_entry_put(migback_file);

	tcmi_comm_put(comm);
	kkc_sock_put(sock1);
	tcmi_slotvec_put(transactions); 
	tcmi_ctlfs_put_root();
}


module_init(tcmi_ppm_p_guest_test_init);
module_exit(tcmi_ppm_p_guest_test_exit);



