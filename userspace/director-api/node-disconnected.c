#include "node-disconnected.h"
#include "director-api.h"
#include "msg-common.h"
#include "msgs.h"
#include "internal.h"

#include <errno.h>

static node_disconnected_callback_t node_disconnected_callback = NULL;

void register_node_disconnected_callback(node_disconnected_callback_t callback) {
	node_disconnected_callback = callback;
}

int handle_node_disconnected(struct nl_msg *req_msg) {
	struct nl_msg *msg = NULL;
	struct nlattr *nla;
	int ret = 0;
	int seq;
	struct internal_state* state = get_current_state();

	// In params
	int slot_index;
	int slot_type;
	int reason;
	// Out params - NONE
	
	seq = nlmsg_hdr(req_msg)->nlmsg_seq;

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_INDEX);
	if (nla == NULL)
		return  -EBADMSG;
	slot_index = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_SLOT_TYPE);
	if (nla == NULL)
		return  -EBADMSG;
	slot_type = nla_get_u32(nla);


	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_REASON);
	if (nla == NULL)
		return  -EBADMSG;
	reason = nla_get_u32(nla);


	//printf("NPM CALLED FOR NAME: %s\n", name);
	if ( node_disconnected_callback )
        	node_disconnected_callback(slot_index, slot_type, reason);
	
	if ( (ret=prepare_response_message(state->handle, DIRECTOR_ACK, state->gnl_fid, seq, &msg) ) != 0 ) {
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
