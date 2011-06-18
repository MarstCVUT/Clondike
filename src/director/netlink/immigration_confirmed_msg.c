#include "immigration_confirmed_msg.h"
#include "msgs.h"
#include "genl_ext.h"
#include "comm.h"

#include <dbg.h>

#include <linux/skbuff.h>

struct immigration_confirmed_params {
	/* In params */
	const char* name;
	int slot_index;
	uid_t uid;
	pid_t local_pid;
	pid_t remote_pid;

	/* Out params */
};

static int immigration_confirmed_create_request(struct sk_buff *skb, void* params) {
  	int ret = 0;
  	struct immigration_confirmed_params* immigration_confirmed_params = params;

	ret = nla_put_u32(skb, DIRECTOR_A_UID, immigration_confirmed_params->uid);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_u32(skb, DIRECTOR_A_INDEX, immigration_confirmed_params->slot_index);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_string(skb, DIRECTOR_A_NAME, immigration_confirmed_params->name);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_u32(skb, DIRECTOR_A_PID, immigration_confirmed_params->local_pid);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_u32(skb, DIRECTOR_A_REMOTE_PID, immigration_confirmed_params->remote_pid);
  	if (ret != 0)
      		goto failure;

failure:
	return ret;
}

static int immigration_confirmed_read_response(struct genl_info* info, void* params) {
	int ret = 0;
	return ret;
}

static struct msg_transaction_ops immigration_confirmed_msg_ops = {
	.create_request = immigration_confirmed_create_request,
	.read_response = immigration_confirmed_read_response
};

int immigration_confirmed(int slot_index, uid_t uid, const char* name, pid_t local_pid, pid_t remote_pid) {
	struct immigration_confirmed_params params;
	int ret;
	
	params.slot_index = slot_index;
	params.uid = uid;
	params.name = name;
	params.local_pid = local_pid;
	params.remote_pid = remote_pid;

	ret = msg_transaction_do(DIRECTOR_IMMIGRATION_CONFIRMED, &immigration_confirmed_msg_ops, &params, 0);

	if ( ret )
		return ret;

	return 0;
}
