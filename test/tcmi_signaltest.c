/**
 * @file tcmi_signaltest.c - 
 *
 *
 * Date: 04/29/2005
 *
 * Author: Jan Capek
 *
 *
 * License....
 */
#ifdef APP_NAME
#undef APP_NAME
#endif
#define APP_NAME SIGNAL-TEST

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>

#include <clondike/tcmi/tcmi_hooks.h>

#include <dbg.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");

static int send_sg(int signr)
{
	if ((signr != SIGALRM) && (signr != SIGIO))
		minfo(INFO1, "Sending signal: %d ", signr);
	return 0xfe;
}

static int doing_sigfatal(int signr, int dump)
{
	minfo(INFO1, "Group completing  a fatal signal %d, coredump=%d", 
	      signr, dump);
	return 0xfe;
}

/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_signaltest_init(void)
{
	tcmi_hooks_register_send_sig(send_sg);
	tcmi_hooks_register_doing_sigfatal(doing_sigfatal);

	siginfo_t info;
	unsigned long signr = 0;
	sigset_t *mask = &current->blocked;

	flush_signals(current);
	recalc_sigpending();

	for(;;) {
		/* check pending signal */
		if (signal_pending(current)) {
			/* dequeue signal */
			spin_lock_irq(&current->sighand->siglock);
			signr = dequeue_signal(current, mask, &info);
			recalc_sigpending();
			spin_unlock_irq(&current->sighand->siglock);
			/* no more signals in queue */
			minfo(INFO2, "Dequeued %lx..", signr);
			if (signr == SIGUSR1) {
				minfo(INFO2, "Terminating.. %lx.", signr);
				break;
			}
		}
		else {
			minfo(INFO1, "No mor signals, going to sleep");
			schedule();
			set_current_state(TASK_INTERRUPTIBLE);
		}
	}

	set_current_state(TASK_RUNNING);
	flush_signals(current);
	return 0;
}

/**
 * Module cleanup, stops the message receiving
 * thread, cleans transactions and cleans all messages
 * in the queue.
 */
static void __exit tcmi_signaltest_exit(void)
{
	minfo(INFO1, "TCMI shadow task test terminating");
	tcmi_hooks_unregister_send_sig();
	tcmi_hooks_unregister_doing_sigfatal();

}


module_init(tcmi_signaltest_init);
module_exit(tcmi_signaltest_exit);
