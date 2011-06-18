#include "migrated-home.h"
#include "director-api.h"
#include "msg-common.h"
#include "msgs.h"
#include "internal.h"

#include <errno.h>

static migrated_home_callback_t migrated_home_callback = NULL;

void register_migrated_home_callback(migrated_home_callback_t callback) {
	migrated_home_callback = callback;
}

int handle_migrated_home(struct nl_msg *req_msg) {
	struct nl_msg *msg = NULL;
	struct nlattr *nla;
	int ret = 0;
	int seq;
	struct internal_state* state = get_current_state();

	// In params
	pid_t pid;
	
	seq = nlmsg_hdr(req_msg)->nlmsg_seq;

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_PID);
	if (nla == NULL)
		return  -EBADMSG;
	pid = nla_get_u32(nla);

	if ( migrated_home_callback )
        	migrated_home_callback(pid);
	
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
