#include "npm_msg.h"
#include "msgs.h"
#include "genl_ext.h"
#include "comm.h"

#include "emigration_failed_msg.h"

#include <dbg.h>

#include <linux/skbuff.h>

struct emigration_failed_params {
	/* In params */
	u32 pid;

	/* Out params -> NONE */

};

static int emigration_failed_create_request(struct sk_buff *skb, void* params) {
  	int ret = 0;
  	struct emigration_failed_params* emigration_failed_params = params;

	ret = nla_put_u32(skb, DIRECTOR_A_PID, emigration_failed_params->pid);
  	if (ret != 0)
      		goto failure;
failure:
	return ret;
}


static int emigration_failed_read_response(struct genl_info* info, void* params) {
	int ret = 0;

	return ret;
}

static struct msg_transaction_ops emigration_failed_msg_ops = {
	.create_request = emigration_failed_create_request,
	.read_response = emigration_failed_read_response
};


int emigration_failed(pid_t pid) {
	struct emigration_failed_params params;
	int ret;

	params.pid = pid;

	ret = msg_transaction_do(DIRECTOR_EMIGRATION_FAILED, &emigration_failed_msg_ops, &params, 1);

	minfo(INFO3, "Emigration failed. Pid:  %u -> Res: %d", pid, ret);

	return ret;
};
