#include "npm_msg.h"
#include "msgs.h"
#include "genl_ext.h"
#include "comm.h"

#include "migrated_home_msg.h"

#include <dbg.h>

#include <linux/skbuff.h>

struct migrated_home_params {
	/* In params */
	u32 pid;

	/* Out params -> NONE */

};

static int migrated_home_create_request(struct sk_buff *skb, void* params) {
  	int ret = 0;
  	struct migrated_home_params* migrated_home_params = params;

	ret = nla_put_u32(skb, DIRECTOR_A_PID, migrated_home_params->pid);
  	if (ret != 0)
      		goto failure;
failure:
	return ret;
}


static int migrated_home_read_response(struct genl_info* info, void* params) {
	int ret = 0;

	return ret;
}

static struct msg_transaction_ops migrated_home_msg_ops = {
	.create_request = migrated_home_create_request,
	.read_response = migrated_home_read_response
};


int migrated_home(pid_t pid) {
	struct migrated_home_params params;
	int ret;

	params.pid = pid;

	ret = msg_transaction_do(DIRECTOR_MIGRATED_HOME, &migrated_home_msg_ops, &params, 1);

	minfo(INFO3, "Migrated home. Pid:  %u -> Res: %d", pid, ret);

	return ret;
};
