/**
 * @file tcmi_hooks_test.c - Simple test that registers an exec function that prints out
 *                      hello message upon any exec.
 *
 *
 * Date: 04/14/2005
 *
 * Author: Jan Capek
 *
 *
 * License....
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <asm/ptrace.h>

#include <clondike/tcmi/tcmi_hooks.h>

#include <dbg.h>



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");


static int hook_execve(char *filename,
		       char __user *__user *argv,
		       char __user *__user *envp,
		       struct pt_regs *regs)

{
	minfo(INFO1, "Testing hook execve filename: '%s'", filename);
	return 0xff;
}

static int hook_exit(long code)

{
	minfo(INFO1, "Process terminating %p '%s'", current, current->comm);
	return 0xff;
}

static int migr_mode(struct pt_regs *regs)

{
	minfo(INFO1, "Migration mode active for %p '%s'", current, current->comm);
	minfo(INFO1, "Sigpending: %lux %lux, current state = %ld, stop_count = %d", 
	      current->pending.signal.sig[1], 
	      current->pending.signal.sig[0],
	      current->state, current->signal->group_stop_count);
	return 0xfe;
}

static int did_stop(int signr)

{
	minfo(INFO1, "Stopped on signal %d for  %p '%s'", signr, current, current->comm);
	minfo(INFO1, "Sigpending: %lux %lux, current state = %ld, stop_count = %d", 
	      current->pending.signal.sig[1], 
	      current->pending.signal.sig[0],
	      current->state, current->signal->group_stop_count);
	return 0xfe;
}

static int sig_deliver(int signr)

{
	minfo(INFO1, "Delivering default signal %d for  %p '%s'", signr, current, current->comm);
	minfo(INFO1, "Sigpending: %lux %lux, current state = %ld, stop_count = %d", 
	      current->pending.signal.sig[1], 
	      current->pending.signal.sig[0],
	      current->state, current->signal->group_stop_count);
	return 0xfe;
}
static int sig_delivered(int signr)

{
	minfo(INFO1, "Signal to deliver %d for  %p '%s'", signr, current, current->comm);
	minfo(INFO1, "Sigpending: %lux %lux, current state = %ld, stop_count = %d", 
	      current->pending.signal.sig[1], 
	      current->pending.signal.sig[0],
	      current->state,  current->signal->group_stop_count);
	return 0xfe;
}
static int group_stop(int stop_count)

{
	minfo(INFO1, "Group stop count is: %d for  %p '%s'", stop_count, current, current->comm);
	minfo(INFO1, "Sigpending: %lux %lux, current state = %ld, stop_count = %d", 
	      current->pending.signal.sig[1], 
	      current->pending.signal.sig[0],
	      current->state, current->signal->group_stop_count);
	return 0xfe;
}

static int deq_sig(int signr)

{
	minfo(INFO1, "Dequeued signal: %d for  %p '%s'", signr, current, current->comm);
	minfo(INFO1, "Sigpending: %lux %lux, current state = %ld, stop_count = %d", 
	      current->pending.signal.sig[1], 
	      current->pending.signal.sig[0],
	      current->state, current->signal->group_stop_count);
	return 0xfe;
}

static int send_sg(int signr)

{
	minfo(INFO1, "Sending signal: %d ", signr);
	return 0xfe;
}
/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_hooks_test_init(void)
{

	minfo(INFO1, "Registering hook (%p)", hook_execve);
	tcmi_hooks_register_execve(hook_execve);
	tcmi_hooks_register_exit(hook_exit);
	tcmi_hooks_register_sig_unused(migr_mode);
	tcmi_hooks_register_sig_deliver(sig_deliver);
	tcmi_hooks_register_sig_delivered(sig_delivered);
	tcmi_hooks_register_did_stop(did_stop);
	tcmi_hooks_register_group_stop(group_stop);
	tcmi_hooks_register_deq_sig(deq_sig);
	tcmi_hooks_register_send_sig(send_sg);

	minfo(INFO1, "Checking, execve hook method got installed: %p %p", 
	      tcmi_hooks_execve, &tcmi_hooks_execve);
	minfo(INFO1, "Checking, exit hook method got installed: %p %p", 
	      tcmi_hooks_exit, &tcmi_hooks_exit);
	minfo(INFO1, "Checking, sig_unused hook method got installed: %p %p", 
	      tcmi_hooks_sig_unused, &tcmi_hooks_sig_unused);

	return 0;
}

/**
 * module cleanup
 */
static void __exit tcmi_hooks_test_exit(void)
{
	minfo(INFO1, "Unregistering hook");	
	tcmi_hooks_unregister_execve();
	tcmi_hooks_unregister_exit();
	tcmi_hooks_unregister_sig_unused();
	tcmi_hooks_unregister_sig_deliver();
	tcmi_hooks_unregister_sig_delivered();
	tcmi_hooks_unregister_did_stop();
	tcmi_hooks_unregister_group_stop();
	tcmi_hooks_unregister_deq_sig();
	tcmi_hooks_unregister_send_sig();
}





module_init(tcmi_hooks_test_init);
module_exit(tcmi_hooks_test_exit);


int *test_pointer = NULL;

