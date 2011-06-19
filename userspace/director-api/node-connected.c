#include "node-connected.h"
#include "director-api.h"
#include "msg-common.h"
#include "msgs.h"
#include "internal.h"

#include <errno.h>

static node_connected_callback_t node_connected_callback = NULL;

void register_node_connected_callback(node_connected_callback_t callback) {
	node_connected_callback = callback;
}

int handle_node_connected(struct nl_msg *req_msg) {
	struct nl_msg *msg = NULL;
	struct nlattr *nla;
	int ret = 0;
	int seq;
	struct internal_state* state = get_current_state();

	// In params
	char* address;
	int slot_index;
	int auth_data_size;
	char* auth_data;
	// Out params
	int accept = 1;
	
	seq = nlmsg_hdr(req_msg)->nlmsg_seq;

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_INDEX);
	if (nla == NULL)
		return  -EBADMSG;
	slot_index = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_ADDRESS);
	if (nla == NULL)
		return  -EBADMSG;
	//address = nl_data_get(nla_get_data(nla));
	address = nla_data(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_LENGTH);
	if (nla == NULL)
		return  -EBADMSG;
	auth_data_size = nla_get_u32(nla);

	if ( auth_data_size > 0 ) {
		nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_AUTH_DATA);
		if (nla == NULL)
			return  -EBADMSG;
		auth_data = nl_data_get(nla_get_data(nla));
	} else {
		auth_data = NULL;
	}

	//printf("NPM CALLED FOR NAME: %s\n", name);
	if ( node_connected_callback )
        	node_connected_callback(address, slot_index, auth_data_size, auth_data, &accept);
	
	if ( (ret=prepare_response_message(state->handle, DIRECTOR_NODE_CONNECT_RESPONSE, state->gnl_fid, seq, &msg) ) != 0 ) {
		goto done;
	}
	
	ret = nla_put_u32(msg,
			DIRECTOR_A_DECISION,
			accept);

	if (ret != 0)
		goto error_del_resp;

	ret = send_request_message(state->handle, msg, 0);
	goto done;	

error_del_resp:
	nlmsg_free(msg);
done:	
	return ret;
}
