#include "task-exitted.h"
#include "director-api.h"
#include "msg-common.h"
#include "msgs.h"
#include "internal.h"

#include <errno.h>

static task_exitted_callback_t task_exitted_callback = NULL;

void register_task_exitted_callback(task_exitted_callback_t callback) {
	task_exitted_callback = callback;
}

int handle_task_exitted(struct nl_msg *req_msg) {
	struct nl_msg *msg = NULL;
	struct nlattr *nla;
	int ret = 0;
	int seq;
	struct internal_state* state = get_current_state();
	struct rusage *rusage;

	// In params
	pid_t pid;
	int exit_code;
	
	seq = nlmsg_hdr(req_msg)->nlmsg_seq;

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_PID);
	if (nla == NULL)
		return  -EBADMSG;
	pid = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_EXIT_CODE);
	if (nla == NULL)
		return  -EBADMSG;
	exit_code = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_RUSAGE);
	if (nla == NULL)
		return  -EBADMSG;
	
	//rusage = nl_data_get(nla_get_data(nla));
	rusage = nla_data(nla);

	if ( task_exitted_callback )
        	task_exitted_callback(pid, exit_code, rusage);
	
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
