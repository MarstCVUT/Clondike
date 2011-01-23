#include "node_disconnected_msg.h"
#include "msgs.h"
#include "genl_ext.h"
#include "comm.h"

#include <dbg.h>

#include <linux/skbuff.h>

struct node_disconnected_params {
	/* In params */
	int slot_index;
	int detached;
	int reason;

	/* Out params - NONE */
};

static int node_disconnected_create_request(struct sk_buff *skb, void* params) {
  	int ret = 0;
  	struct node_disconnected_params* node_disconnected_params = params;

	ret = nla_put_u32(skb, DIRECTOR_A_INDEX, node_disconnected_params->slot_index);
  	if (ret != 0)
      		goto failure;

	// If remote node is detached, then local slot is of type core node (because detached nodes are connected into core slots)
	ret = nla_put_u32(skb, DIRECTOR_A_SLOT_TYPE, node_disconnected_params->detached != 0);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_u32(skb, DIRECTOR_A_REASON, node_disconnected_params->reason);
  	if (ret != 0)
      		goto failure;
failure:
	return ret;
}

static struct msg_transaction_ops node_disconnected_msg_ops = {
	.create_request = node_disconnected_create_request,
	.read_response = NULL
};

int node_disconnected(int slot_index, int detached, int reason) {
	struct node_disconnected_params params;
	int ret;

	params.slot_index = slot_index;
	params.detached = detached;
	params.reason = reason;

	ret = msg_transaction_do(DIRECTOR_NODE_DISCONNECTED, &node_disconnected_msg_ops, &params, 1);

	if ( ret )
		return ret;
	
	return 0;
}

