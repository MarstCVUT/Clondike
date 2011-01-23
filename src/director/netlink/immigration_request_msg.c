#include "immigration_request_msg.h"
#include "msgs.h"
#include "genl_ext.h"
#include "comm.h"

#include <dbg.h>

#include <linux/skbuff.h>

struct immigration_request_params {
	/* In params */
	const char* name;
	int slot_index;
	uid_t uid;

	/* Out params */
	/** User mode helper decision*/
	int accepted;

};

static int immigration_request_create_request(struct sk_buff *skb, void* params) {
  	int ret = 0;
  	struct immigration_request_params* immigration_request_params = params;

	ret = nla_put_u32(skb, DIRECTOR_A_UID, immigration_request_params->uid);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_u32(skb, DIRECTOR_A_INDEX, immigration_request_params->slot_index);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_string(skb, DIRECTOR_A_NAME, immigration_request_params->name);
  	if (ret != 0)
      		goto failure;

failure:
	return ret;
}

static int immigration_request_read_response(struct genl_info* info, void* params) {
	int ret = 0;
	struct nlattr* attr;
	struct immigration_request_params* immigration_request_params = params;

	attr = nlmsg_find_attr(info->nlhdr, sizeof(struct genlmsghdr), DIRECTOR_A_DECISION);
	if ( attr == NULL ) {
		ret = -EBADMSG;
		goto done;
	}

	immigration_request_params->accepted = nla_get_u32(attr);
done:
	return ret;
}

static struct msg_transaction_ops immigration_request_msg_ops = {
	.create_request = immigration_request_create_request,
	.read_response = immigration_request_read_response
};

int immigration_request(int slot_index, uid_t uid, const char* name, int* accept) {
	struct immigration_request_params params;
	int ret;
	
	params.slot_index = slot_index;
	params.uid = uid;
	params.name = name;

	ret = msg_transaction_do(DIRECTOR_IMMIGRATION_REQUEST, &immigration_request_msg_ops, &params, 0);

	if ( ret )
		return ret;

	*accept = params.accepted;
	
	return 0;
}
