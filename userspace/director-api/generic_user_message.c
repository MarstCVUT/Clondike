#include "generic_user_message.h"
#include "director-api.h"
#include "msg-common.h"
#include "msgs.h"
#include "internal.h"

#include <errno.h>

static generic_user_message_callback_t generic_user_message_callback = NULL;

void register_generic_user_message_callback(generic_user_message_callback_t callback) {
	generic_user_message_callback = callback;
}

int handle_generic_user_message(struct nl_msg *req_msg) {
	struct nl_msg *msg = NULL;
	struct nlattr *nla;
	int ret = 0;
	int seq;
	struct internal_state* state = get_current_state();

	// In params
	int node_idx;
	int slot_index, slot_type;
	int user_data_size;
	char* user_data;
	
	seq = nlmsg_hdr(req_msg)->nlmsg_seq;

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_INDEX);
	if (nla == NULL)
		return  -EBADMSG;
	node_idx = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_SLOT_INDEX);
	if (nla == NULL)
		return  -EBADMSG;
	slot_index = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_SLOT_TYPE);
	if (nla == NULL)
		return  -EBADMSG;
	slot_type = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_LENGTH);
	if (nla == NULL)
		return  -EBADMSG;
	user_data_size = nla_get_u32(nla);

	if ( user_data_size > 0 ) {
		nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_USER_DATA);
		if (nla == NULL)
			return  -EBADMSG;
		//user_data = nl_data_get(nla_get_data(nla));
		user_data = nla_data(nla);
	} else {
		user_data = NULL;
	}

	if ( generic_user_message_callback )
        	generic_user_message_callback(node_idx, slot_type, slot_index, user_data_size, user_data);
	
	if ( (ret=prepare_response_message(state->handle, DIRECTOR_NODE_CONNECT_RESPONSE, state->gnl_fid, seq, &msg) ) != 0 ) {
		goto done;
	}
	
	if (ret != 0)
		goto error_del_resp;

	ret = send_request_message(state->handle, msg, 0);
	goto done;	

error_del_resp:
	nlmsg_free(msg);
done:	
	return ret;
}
