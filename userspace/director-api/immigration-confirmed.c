#include "immigration-confirmed.h"
#include "director-api.h"
#include "msg-common.h"
#include "msgs.h"
#include "internal.h"

#include <errno.h>

static immigration_confirmed_callback_t immigration_confirmed_callback = NULL;

void register_immigration_confirmed_callback(immigration_confirmed_callback_t callback) {
	immigration_confirmed_callback = callback;
}

int handle_immigration_confirmed(struct nl_msg *req_msg) {
	struct nl_msg *msg = NULL;
	struct nlattr *nla;
	int ret = 0;
	int seq;
	struct internal_state* state = get_current_state();

	// In params	
	int uid;
	int slot_index;
	char* name;
	pid_t local_pid;
	pid_t remote_pid;
	
	seq = nlmsg_hdr(req_msg)->nlmsg_seq;

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_UID);
	if (nla == NULL)
		return  -EBADMSG;
	uid = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_INDEX);
	if (nla == NULL)
		return  -EBADMSG;
	slot_index = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_NAME);
	if (nla == NULL)
		return  -EBADMSG;
	//name = nl_data_get(nla_get_data(nla));
	name = nla_data(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_PID);
	if (nla == NULL)
		return  -EBADMSG;
	local_pid = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_REMOTE_PID);
	if (nla == NULL)
		return  -EBADMSG;
	remote_pid = nla_get_u32(nla);

	//printf("NPM CALLED FOR NAME: %s\n", name);
	if ( immigration_confirmed_callback )
        	immigration_confirmed_callback(uid, slot_index, name, local_pid, remote_pid);
	
	if ( (ret=prepare_response_message(state->handle, DIRECTOR_ACK, state->gnl_fid, seq, &msg) ) != 0 ) {
		goto done;
	}
	
	ret = send_request_message(state->handle, msg, 0);
	goto done;	

error_del_resp:
	nlmsg_free(msg);
done:	
	return ret;
}
