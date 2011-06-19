#include "npm-check.h"
#include "director-api.h"
#include "msg-common.h"
#include "msgs.h"
#include "internal.h"

#include <errno.h>

static npm_check_callback_t npm_callback = NULL;

static npm_check_full_callback_t npm_full_callback = NULL;

void register_npm_check_callback(npm_check_callback_t callback) {
	npm_callback = callback;
}

void register_npm_check_full_callback(npm_check_full_callback_t callback) {
	npm_full_callback = callback;
}

int handle_npm_check(struct nl_msg *req_msg) {
	struct nl_msg *msg = NULL;
	struct nlattr *nla;
	int ret = 0;
	int seq;
	struct internal_state* state = get_current_state();

	// In params
	char* name;
	int is_guest;
	pid_t pid;
	uid_t uid;
	struct rusage *rusage;
	// Out params
	int decision = DO_NOT_MIGRATE;
	int decision_value = -1;
	
	seq = nlmsg_hdr(req_msg)->nlmsg_seq;

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_PID);
	if (nla == NULL)
		return  -EBADMSG;

	pid = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_UID);
	if (nla == NULL)
		return  -EBADMSG;

	uid = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_TASK_TYPE);
	if (nla == NULL)
		return  -EBADMSG;

	is_guest = nla_get_u32(nla) == 1 ? 1 : 0;

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_NAME);
	if (nla == NULL)
		return  -EBADMSG;

	//name = nl_data_get(nla_get_data(nla));
	name = nla_data(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_RUSAGE);
	if (nla == NULL) {
		rusage = NULL;
	} else {
		//rusage = nl_data_get(nla_get_data(nla));
		rusage = nla_data(nla);
	}

	//printf("NPM CALLED FOR NAME: %s\n", name);
	if ( npm_callback )
        	npm_callback(pid, uid, is_guest, name, rusage, &decision, &decision_value);

	
	if ( (ret=prepare_response_message(state->handle, DIRECTOR_NPM_RESPONSE, state->gnl_fid, seq, &msg) ) != 0 ) {
		goto done;
	}
	
	ret = nla_put_u32(msg,
			DIRECTOR_A_DECISION,
			decision);

	if (ret != 0)
		goto error_del_resp;

	ret = nla_put_u32(msg,
			DIRECTOR_A_DECISION_VALUE,
			decision_value);

	if (ret != 0)
		goto error_del_resp;

	ret = send_request_message(state->handle, msg, 0);
	goto done;	

error_del_resp:
	nlmsg_free(msg);
done:	
	return ret;
}

static char** parse_chars(struct nl_msg *req_msg, int type, int nested_type) {
	struct nlattr *nla, *iter, *head;
	int length, rem, i = 0;
	char** res = NULL;
	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), type);
	if (nla == NULL)
		return NULL;

	head = nla_find(nla_data(nla), nla_len(nla), DIRECTOR_A_LENGTH);
	if ( head == NULL ) {
		printf("LEN NOT FOUND\n");
		return NULL;
	}

	length = nla_get_u32(head);

	res = malloc(sizeof(char*)*(length+1));
	if ( !res )
		return NULL;

	res[length] = NULL;

	head = (struct nlattr*)nla_data(nla);
	nla_for_each_attr(iter, head, nla_len(nla), rem) {		
		int attr_type = iter->nla_type;
		if ( attr_type != nested_type )
			continue;

		//res[i] = nl_data_get(nla_get_data(iter));
		res[i] = nla_data(iter);
		//printf("STR: %s\n", res[i]);
		i++;
	}

	return res;
failed:
	free(res);
	return NULL;
};

int handle_npm_check_full(struct nl_msg *req_msg) {
	struct nl_msg *msg = NULL;
	struct nlattr *nla;
	int ret = 0;
	int seq;
	char** args = NULL, **envp = NULL;
	struct internal_state* state = get_current_state();

	// In params
	char* name;
	int is_guest;
	pid_t pid;
	uid_t uid;
	// Out params
	int decision = DO_NOT_MIGRATE;
	int decision_value = -1;
	
	seq = nlmsg_hdr(req_msg)->nlmsg_seq;

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_PID);
	if (nla == NULL)
		return  -EBADMSG;

	pid = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_UID);
	if (nla == NULL)
		return  -EBADMSG;

	uid = nla_get_u32(nla);

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_TASK_TYPE);
	if (nla == NULL)
		return  -EBADMSG;

	is_guest = nla_get_u32(nla) == 1 ? 1 : 0;

	nla = nlmsg_find_attr(nlmsg_hdr(req_msg), sizeof(struct genlmsghdr), DIRECTOR_A_NAME);
	if (nla == NULL)
		return  -EBADMSG;

	//name = nl_data_get(nla_get_data(nla));
	name = nla_data(nla);

	args = parse_chars(req_msg, DIRECTOR_A_ARGS, DIRECTOR_A_ARG);
	if ( args == NULL ) {
		ret = -EBADMSG;
		goto error_del_resp;
	}

	envp = parse_chars(req_msg, DIRECTOR_A_ENVS, DIRECTOR_A_ENV);
	if ( envp == NULL ) {
		ret = -EBADMSG;
		goto error_del_resp;
	}

	//printf("NPM FULL CALLED FOR NAME: %s\n", name);
	if ( npm_full_callback )
        	npm_full_callback(pid, uid, is_guest, name, args, envp, &decision, &decision_value);

	
	if ( (ret=prepare_response_message(state->handle, DIRECTOR_NPM_RESPONSE, state->gnl_fid, seq, &msg) ) != 0 ) {
		goto done;
	}
	
	ret = nla_put_u32(msg,
			DIRECTOR_A_DECISION,
			decision);

	if (ret != 0)
		goto error_del_resp;

	ret = nla_put_u32(msg,
			DIRECTOR_A_DECISION_VALUE,
			decision_value);

	if (ret != 0)
		goto error_del_resp;

	ret = send_request_message(state->handle, msg, 0);
	goto done;	

error_del_resp:
	nlmsg_free(msg);
done:	
	free(args);
	free(envp);
	return ret;
}
