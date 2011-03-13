#include "npm_msg.h"
#include "msgs.h"
#include "genl_ext.h"
#include "comm.h"

#include <dbg.h>

#include <linux/skbuff.h>
#include <linux/resource.h>

struct npm_check_params {
	/* In params */
	pid_t pid;
	uid_t uid;
	int is_guest;
	const char* name;
	int name_length;
	struct rusage *rusage;
	/* Params only in full mode */
	char __user * __user * args;
	char __user * __user * envp;

	/* Out params */
	/** User mode helper decision.. one of npm_msg_response enum */
	int decision;
	/** If the decision has some target value, this will hold it */
	int decision_value; 
};

/** Creates npm check request */
static int npm_check_create_request(struct sk_buff *skb, void* params) {
  	int ret = 0;
  	struct npm_check_params* check_params = params;

  	ret = nla_put_u32(skb, DIRECTOR_A_PID, check_params->pid);
  	if (ret != 0)
      		goto failure;

  	ret = nla_put_u32(skb, DIRECTOR_A_UID, check_params->uid);
  	if (ret != 0)
      		goto failure;

  	ret = nla_put_u32(skb, DIRECTOR_A_TASK_TYPE, check_params->is_guest ? 1 : 0);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_string(skb, DIRECTOR_A_NAME, check_params->name);
  	if (ret != 0)
      		goto failure;

  	ret = nla_put_u32(skb, DIRECTOR_A_LENGTH, check_params->name_length);
	if (ret != 0)
		goto failure;

	if (check_params->rusage) {
		ret = nla_put(skb, DIRECTOR_A_RUSAGE, sizeof *check_params->rusage, check_params->rusage);
  		if (ret != 0)
      			goto failure;
	}

failure:

	return ret;
}

static int npm_check_read_response(struct genl_info* info, void* params) {
	int ret = 0;
	struct nlattr* attr;
	struct npm_check_params* check_params = params;

	attr = nlmsg_find_attr(info->nlhdr, sizeof(struct genlmsghdr), DIRECTOR_A_DECISION);
	if ( attr == NULL ) {
		ret = -EBADMSG;
		goto done;
	}

	check_params->decision = nla_get_u32(attr);

	attr = nlmsg_find_attr(info->nlhdr, sizeof(struct genlmsghdr), DIRECTOR_A_DECISION_VALUE);
	if ( attr != NULL ) {
		check_params->decision_value = nla_get_u32(attr);
	} else {
		check_params->decision_value = 0;
	}
done:
	return ret;
}

static struct msg_transaction_ops npm_check_msg_ops = {
	.create_request = npm_check_create_request,
	.read_response = npm_check_read_response
};

int npm_check(pid_t pid, uid_t uid, int is_guest, const char* name, int* decision, int* decision_value, struct rusage *rusage) {
	struct npm_check_params params;
	int ret;

	params.pid = pid;
	params.uid = uid;
	params.is_guest = is_guest;
	params.name = name;
	params.name_length = strlen(name);
	params.args = NULL;
	params.envp = NULL;
	params.rusage = rusage;

	ret = msg_transaction_do(DIRECTOR_CHECK_NPM, &npm_check_msg_ops, &params, 0);

	if ( ret )
		return ret;

	*decision = params.decision;
	*decision_value = params.decision_value;
	
	return 0;
}

static int put_char_array(struct sk_buff *skb, char** chars, int type, int nested_type) {
	struct nlattr* nest_attr;
	int ret = 0;

	if ( chars ) {
		int i=0;		
		nest_attr = nla_nest_start(skb, type);
                if ( !nest_attr ) {
			ret = -EMSGSIZE;
			goto failure;
		}

		while ( chars[i] ) {
//			printk("Putting arg\n");
			ret = nla_put_string(skb, nested_type, chars[i]);
			if (ret != 0)
				goto failure;
			
			i++;
		}

//		printk("Putting length: %d\n", i);

		ret = nla_put_u32(skb, DIRECTOR_A_LENGTH, i); // Count of elements
		if (ret != 0)
			goto failure;		

		nla_nest_end(skb, nest_attr);
	}

	return ret;

failure:
	minfo(ERR1, "Putting of char array has failed: %d", ret);
	return ret;
}

static int npm_check_full_create_request(struct sk_buff *skb, void* params) {
  	int ret = 0;
  	struct npm_check_params* check_params = params;
	

  	ret = nla_put_u32(skb, DIRECTOR_A_PID, check_params->pid);
  	if (ret != 0)
      		goto failure;

  	ret = nla_put_u32(skb, DIRECTOR_A_UID, check_params->uid);
  	if (ret != 0)
      		goto failure;

  	ret = nla_put_u32(skb, DIRECTOR_A_TASK_TYPE, check_params->is_guest ? 1 : 0);
  	if (ret != 0)
      		goto failure;

	ret = nla_put_string(skb, DIRECTOR_A_NAME, check_params->name);
  	if (ret != 0)
      		goto failure;

//  	ret = nla_put_u32(skb, DIRECTOR_A_LENGTH, check_params->name_length);
//	if (ret != 0)
//		goto failure;

	ret = put_char_array(skb, check_params->args, DIRECTOR_A_ARGS, DIRECTOR_A_ARG);
	if (ret != 0)
		goto failure;

	ret = put_char_array(skb, check_params->envp, DIRECTOR_A_ENVS, DIRECTOR_A_ENV);
	if (ret != 0)
		goto failure;

failure:
	return ret;
}

static struct msg_transaction_ops npm_check_full_msg_ops = {
	.create_request = npm_check_full_create_request,
	.read_response = npm_check_read_response
};


int npm_check_full(pid_t pid, uid_t uid, int is_guest, const char* name, char __user * __user * args, char __user* __user* envp, int* decision, int* decision_value) {
	struct npm_check_params params;
	int ret;

	params.pid = pid;
	params.uid = uid;
	params.is_guest = is_guest;
	params.name = name;
	params.name_length = strlen(name);
	params.args = args;
	params.envp = envp;
	params.rusage = NULL;

	ret = msg_transaction_do(DIRECTOR_CHECK_FULL_NPM, &npm_check_full_msg_ops, &params, 0);

	if ( ret )
		return ret;

	*decision = params.decision;
	*decision_value = params.decision_value;
	
	return 0;
}
