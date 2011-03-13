#include "npm_msg.h"
#include "msgs.h"
#include "genl_ext.h"
#include "comm.h"

#include "task_exitted_msg.h"

#include <dbg.h>

#include <linux/skbuff.h>

struct task_exitted_params {
	/* In params */
	u32 pid;
	u32 exit_code;
	struct rusage *rusage;

	/* Out params -> NONE */

};

static int task_exitted_create_request(struct sk_buff *skb, void* params) {
  	int ret = 0;
  	struct task_exitted_params* task_exitted_params = params;

	ret = nla_put_u32(skb, DIRECTOR_A_PID, task_exitted_params->pid);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_u32(skb, DIRECTOR_A_EXIT_CODE, task_exitted_params->exit_code);
  	if (ret != 0)
      		goto failure;

	ret = nla_put(skb, DIRECTOR_A_RUSAGE, sizeof *task_exitted_params->rusage, task_exitted_params->rusage);
  	if (ret != 0)
      		goto failure;

failure:
	return ret;
}


static int task_exitted_read_response(struct genl_info* info, void* params) {
	int ret = 0;
//	struct task_exitted_params* task_exitted_params = params;

	return ret;
}

static struct msg_transaction_ops task_exitted_msg_ops = {
	.create_request = task_exitted_create_request,
	.read_response = task_exitted_read_response
};


int task_exitted(pid_t pid, int exit_code, struct rusage *rusage) {
	struct task_exitted_params params;
	int ret;

	params.pid = pid;
	params.exit_code = exit_code;
	params.rusage = rusage;

	ret = msg_transaction_do(DIRECTOR_TASK_EXIT, &task_exitted_msg_ops, &params, 1);

	minfo(INFO3, "Task exitted. Pid:  %u  Exit code: %d -> Res: %d", pid, exit_code, ret);

	return ret;
}
