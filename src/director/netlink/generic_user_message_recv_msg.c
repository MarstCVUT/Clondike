#include "generic_user_message_recv_msg.h"
#include "msgs.h"
#include "genl_ext.h"
#include "comm.h"

#include <dbg.h>

#include <linux/skbuff.h>

struct generic_user_message_params {
	/* In params */
	int node_id;
	int is_core_node;
	int slot_index;
	int user_data_size;
	const char* user_data;

	/* Out params */

};

static int generic_user_message_create_request(struct sk_buff *skb, void* params) {
  	int ret = 0;
  	struct generic_user_message_params* generic_user_message_params = params;

	ret = nla_put_u32(skb, DIRECTOR_A_INDEX, generic_user_message_params->node_id);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_u32(skb, DIRECTOR_A_SLOT_INDEX, generic_user_message_params->slot_index);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_u32(skb, DIRECTOR_A_SLOT_TYPE, generic_user_message_params->is_core_node);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_u32(skb, DIRECTOR_A_LENGTH, generic_user_message_params->user_data_size);
  	if (ret != 0)
      		goto failure;

	if ( generic_user_message_params->user_data_size > 0 ) {
		ret = nla_put(skb, DIRECTOR_A_USER_DATA, generic_user_message_params->user_data_size, generic_user_message_params->user_data);
		if (ret != 0)
			goto failure;	
	}
failure:
	return ret;
}

static int generic_user_message_read_response(struct genl_info* info, void* params) {
	int ret = 0;
	return ret;
}

static struct msg_transaction_ops generic_user_message_msg_ops = {
	.create_request = generic_user_message_create_request,
	.read_response = generic_user_message_read_response
};

int generic_user_message_recv(int node_id, int is_core_node, int slot_index, int user_data_size, char* user_data) {
	struct generic_user_message_params params;
	int ret;

	params.node_id = node_id;
	params.is_core_node = is_core_node;
	params.slot_index = slot_index;
	params.user_data_size = user_data_size;
	params.user_data = user_data;

	ret = msg_transaction_do(DIRECTOR_GENERIC_USER_MESSAGE, &generic_user_message_msg_ops, &params, 0);

	if ( ret )
		return ret;
	
	return 0;
}
