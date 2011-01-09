#include "task-forked.h"
#include "director-api.h"
#include "msg-common.h"
#include "msgs.h"
#include "internal.h"

#include <errno.h>

static task_forked_callback_t task_forked_callback = NULL;

void register_task_forked_callback(task_forked_callback_t callback) {
	task_forked_callback = callback;
}

int handle_task_forked(struct nl_msg *req_msg) {
	struct nl_msg *msg = NULL;
	struct nlattr *nla;
	int ret = 0;
	int seq;
	struct internal_state* state = get_current_state();

	// In params
	pid_t pid, ppid;	
	
	seq = nlmsg_hdr(req_msg)->nlmsg_seq;

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_PID);
	if (nla == NULL)
		return  -EBADMSG;
	pid = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_PPID);
	if (nla == NULL)
		return  -EBADMSG;
	ppid = nla_get_u32(nla);

	//printf("NPM CALLED FOR NAME: %s\n", name);
	if ( task_forked_callback )
        	task_forked_callback(pid, ppid);
	
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
