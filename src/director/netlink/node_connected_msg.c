#include "node_connected_msg.h"
#include "msgs.h"
#include "genl_ext.h"
#include "comm.h"

#include <dbg.h>

#include <linux/skbuff.h>

struct node_connected_params {
	/* In params */
	const char* address;
	int slot_index;
	int auth_data_size;
	const char* auth_data;

	/* Out params */
	/** User mode helper decision*/
	int accepted;

};

static int node_connected_create_request(struct sk_buff *skb, void* params) {
  	int ret = 0;
  	struct node_connected_params* node_connected_params = params;

	ret = nla_put_string(skb, DIRECTOR_A_ADDRESS, node_connected_params->address);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_u32(skb, DIRECTOR_A_INDEX, node_connected_params->slot_index);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_u32(skb, DIRECTOR_A_LENGTH, node_connected_params->auth_data_size);
  	if (ret != 0)
      		goto failure;

	if ( node_connected_params->auth_data_size > 0 ) {
		ret = nla_put(skb, DIRECTOR_A_AUTH_DATA, node_connected_params->auth_data_size, node_connected_params->auth_data);
		if (ret != 0)
			goto failure;	
	}
failure:
	return ret;
}

static int node_connected_read_response(struct genl_info* info, void* params) {
	int ret = 0;
	struct nlattr* attr;
	struct node_connected_params* node_connected_params = params;

	attr = nlmsg_find_attr(info->nlhdr, sizeof(struct genlmsghdr), DIRECTOR_A_DECISION);
	if ( attr == NULL ) {
		ret = -EBADMSG;
		goto done;
	}

	node_connected_params->accepted = nla_get_u32(attr);
done:
	return ret;
}

static struct msg_transaction_ops node_connected_msg_ops = {
	.create_request = node_connected_create_request,
	.read_response = node_connected_read_response
};

int node_connected(const char* address, int slot_index, int auth_data_size, char* auth_data, int* accept) {
	struct node_connected_params params;
	int ret;

	params.address = address;
	params.slot_index = slot_index;
	params.auth_data_size = auth_data_size;
	params.auth_data = auth_data;

	ret = msg_transaction_do(DIRECTOR_NODE_CONNECTED, &node_connected_msg_ops, &params, 0);

	if ( ret )
		return ret;

	*accept = params.accepted;
	
	return 0;
}
