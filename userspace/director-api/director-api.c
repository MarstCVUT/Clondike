#include "director-api.h"

#include "msgs.h"

#include <netlink/netlink.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/handlers.h>
#include <linux/genetlink.h>

#include <errno.h>
#include <poll.h>
#include <time.h>

#include "internal.h"
#include "msg-common.h"
#include "npm-check.h"
#include "node-connected.h"
#include "node-disconnected.h"
#include "immigration-request.h"
#include "immigration-confirmed.h"
#include "task-exitted.h"
#include "task-forked.h"
#include "migrated-home.h"
#include "emigration-failed.h"
#include "generic_user_message.h"


static struct internal_state state = {
	.gnl_fid = 0,
	.handle = NULL,
};

/* Array of registered handlers (max 1 handler per command) */
static cmd_handler handlers[DIRECTOR_MSG_MAX];
/* Mapping of response message types on request messages */
static uint8_t ans_mapping[DIRECTOR_MSG_MAX] = {
	[DIRECTOR_CHECK_NPM] = DIRECTOR_NPM_RESPONSE,
	[DIRECTOR_CHECK_FULL_NPM] = DIRECTOR_NPM_RESPONSE,
	[DIRECTOR_NODE_CONNECTED] = DIRECTOR_NODE_CONNECT_RESPONSE,
	[DIRECTOR_NODE_DISCONNECTED] = DIRECTOR_ACK,
	[DIRECTOR_IMMIGRATION_REQUEST] = DIRECTOR_IMMIGRATION_REQUEST_RESPONSE,
	[DIRECTOR_IMMIGRATION_CONFIRMED] = DIRECTOR_ACK,
	[DIRECTOR_TASK_EXIT] = DIRECTOR_ACK,
	[DIRECTOR_TASK_FORK] = DIRECTOR_ACK,
	[DIRECTOR_MIGRATED_HOME] = DIRECTOR_ACK,
	[DIRECTOR_EMIGRATION_FAILED] = DIRECTOR_ACK,
	[DIRECTOR_GENERIC_USER_MESSAGE] = DIRECTOR_ACK,
};

struct internal_state* get_current_state(void) {
	return &state;
}

static void process_handle_error(int err, struct nl_msg* msg) {
	struct nlmsghdr *nl_hdr;
	struct genlmsghdr* genl_hdr;

	uint8_t cmd;
	uint32_t seq;
	
	nl_hdr = nlmsg_hdr(msg);
	genl_hdr = nl_msg_genlhdr(msg);
	cmd = genl_hdr->cmd;
	seq = nl_hdr->nlmsg_seq;

	send_error_message(err, seq, ans_mapping[cmd]);
}

/** Handles recieve of generic event that needs dispatching */
static int genl_cmd_dispatch(struct nl_msg *msg) {		
	struct genlmsghdr* genl_hdr;
	int cmd;

	//printf("Dispatch called\n");
	genl_hdr = nl_msg_genlhdr(msg);
	cmd = genl_hdr->cmd;

	if ( handlers[cmd] != NULL ) {
		//printf("Handling cmd: %d\n", cmd);
		fflush(stdout);
		return handlers[cmd](msg);
	}

	printf("Unable to handle unknown command: %d\n", cmd);

	return -EINVAL;
}

int get_netlink_fd(void) {
	return nl_socket_get_fd(state.handle);
}

static int read_would_block(struct nl_handle* handle) {
  	int ret_val;
  	struct pollfd pollfds[1];
  
  	pollfds[0].fd = nl_socket_get_fd(handle);
  	pollfds[0].events = POLLIN;

  	ret_val = poll(pollfds, 1, 0);
	return ret_val == 0; // 0 == timeouted
}

#define NO_RESPONSE_CODE -324888

static int get_message_response_code(struct nl_msg *msg) {
  struct nlmsghdr *nl_hdr;

  nl_hdr = nlmsg_hdr(msg);
  return nl_hdr->nlmsg_type == NLMSG_ERROR ? ((struct nlmsgerr*)nlmsg_data(nlmsg_hdr(msg)))->error : NO_RESPONSE_CODE;
}  

static int is_ack_message(struct nl_msg *msg) {
  return get_message_response_code(msg) == 0;
}

static int handle_incoming_message(struct nl_msg *msg) {
	struct nlmsghdr *nl_hdr;
	int is_genl_msg;
	int res = 0;
  
	is_genl_msg = 1;		

	nl_hdr = nlmsg_hdr(msg);
	if ( nl_hdr->nlmsg_type == NLMSG_ERROR ) {
		struct nlmsgerr* nl_err = (struct nlmsgerr*)nlmsg_data(nlmsg_hdr(msg));
		res = nl_err->error;
		if ( res != 0 ) /* Else we got just ACK message, no special handling of those */
			printf("Error message response code: %d!!\n", nl_err->error);
		
		is_genl_msg = 0;
	} else if ( nl_hdr->nlmsg_type != state.gnl_fid ) {
		printf("Ignore unknown message type: %d!!\n", nl_hdr->nlmsg_type);
		is_genl_msg = 0;
	}

	if ( is_genl_msg ) {
		//printf("Handling generic netlink msg: Tx id: %d\n", nlmsg_hdr(msg)->nlmsg_seq); 
		if ( res = genl_cmd_dispatch(msg) ) {
			printf("Handling generic netlink msg error %d\n", res);
			process_handle_error(res, msg);
		}
	}

	nlmsg_free(msg);    
	
	return res;
}

/** Runs loop that waits for a new messages and dispatches them */
int run_processing_callback(int allow_block) {
	struct nl_msg *msg = NULL;	
	int res;
	
	//printf("Starting processing loop\n");
	while (1) {
		//struct timeval start, end;
		//gettimeofday(&start);
//		printf("New netlink message arrived\n");
		if ( !allow_block && read_would_block(state.handle) ) 
			return;

		if ( (res = read_message(state.handle, &msg) ) != 0 ) {
			printf("Error in message reading: %d\n", res);
			if ( res == -EINTR ) // Interrupted => finish the thread
				break;
			continue;
		}
		
		handle_incoming_message(msg);		
	}
}

/** 
 * Setups generic netlink connection with the kernel module and retrieves assocaited family id
 *
 * @return 0 on success
 */
static int initialize_netlink_family(void) {
	struct nl_handle* hndl;
	struct nl_msg* msg = NULL;
	struct nl_msg *ans_msg = NULL;
	struct nlmsghdr *nl_hdr;
	struct genlmsghdr* genl_hdr;
	struct nlattr *nla;
	int ret_val = 0;

	hndl = nl_handle_alloc();
	nl_set_buffer_size(hndl, 15000000, 15000000);
	
	//nl_handle_set_peer_pid(hndl, 0);
	//nl_set_passcred(hndl, 1);
	nl_disable_sequence_check(hndl);

	if ( (ret_val=nl_connect(hndl, NETLINK_GENERIC)) )
		goto init_return;
	
	nl_set_buffer_size(hndl, 15000000, 15000000);
  
	if ( (ret_val=prepare_request_message(hndl, CTRL_CMD_GETFAMILY, GENL_ID_CTRL, &msg) ) != 0 ) {
		goto init_return;
  	}
  
	ret_val = nla_put_string(msg,
		   CTRL_ATTR_FAMILY_NAME,
		   "DIRECTORCHNL");

  	if (ret_val != 0)
		goto init_return;

	if ( (ret_val = send_request_message(hndl, msg, 0) ) != 0 )
		goto init_return;
	if ( (ret_val = read_message(hndl, &ans_msg) ) != 0 )
		goto init_return;

	genl_hdr = nl_msg_genlhdr(ans_msg);
 	if (genl_hdr == NULL || genl_hdr->cmd != CTRL_CMD_NEWFAMILY) {
    		ret_val = -EBADMSG;
    		goto init_return;
  	}

	nla = nlmsg_find_attr(nlmsg_hdr(ans_msg), sizeof(struct genlmsghdr), CTRL_ATTR_FAMILY_ID);
  	if (nla == NULL) {
    		ret_val = -EBADMSG;
    		goto init_return;
  	}

  	state.gnl_fid = nla_get_u16(nla);  
  	if (state.gnl_fid == 0) {
    		ret_val = -EBADMSG;
    		goto init_return;
  	}
  	  	

  	state.handle = hndl;

	return 0;

init_return:
	nlmsg_free(ans_msg);
	
	if ( state.handle == NULL ) {
		nl_close(hndl);
		nl_handle_destroy(hndl);
  	}

	return -EINVAL;
}

/** Registeres pid as a user-space director into the kernel */
static int register_pid(pid_t pid) {
	struct nl_msg* msg = NULL;
	struct nl_msg *ans_msg = NULL;
	struct nlmsghdr *nl_hdr;
	struct genlmsghdr* genl_hdr;
	struct nlattr *nla;

	int ret_val = 0;

	printf("Registering pid\n");	

	if ( (ret_val=prepare_request_message(state.handle, DIRECTOR_REGISTER_PID, state.gnl_fid, &msg) ) != 0 ) {
		goto done;
  	}
    	
  	ret_val = nla_put_u32(msg,
			   DIRECTOR_A_PID,
			   pid);
  	
	if (ret_val != 0)
    		goto done;

  	if ( (ret_val = send_request_message(state.handle, msg, 1) ) != 0 )
    		goto done;

	  if ( (ret_val = read_message(state.handle, &ans_msg) ) != 0 )
    		goto done;

done:
	nlmsg_free(ans_msg);
	return ret_val;
}

/** Sends a user message to remote node via associated kernel connection. */
int send_user_message(int target_slot_type, int target_slot_index, int data_length, char* data) {
	struct nl_msg* msg = NULL;
	struct nl_msg *ans_msg = NULL;
	struct nlmsghdr *nl_hdr;
	struct genlmsghdr* genl_hdr;
	struct nlattr *nla;

	int ret_val = 0;

	if ( (ret_val=prepare_request_message(state.handle, DIRECTOR_SEND_GENERIC_USER_MESSAGE, state.gnl_fid, &msg) ) != 0 ) {
		goto done;
  	}

  	ret_val = nla_put_u32(msg,
			   DIRECTOR_A_SLOT_TYPE,
			   target_slot_type);
	if (ret_val != 0)
    		goto done;
    	
  	ret_val = nla_put_u32(msg,
			   DIRECTOR_A_SLOT_INDEX,
			   target_slot_index);
	if (ret_val != 0)
    		goto done;

  	ret_val = nla_put_u32(msg,
			   DIRECTOR_A_LENGTH,
			   data_length);
	if (ret_val != 0)
    		goto done;

	ret_val = nla_put(msg, DIRECTOR_A_USER_DATA, data_length, data);
	if (ret_val != 0)
    		goto done;

  	if ( (ret_val = send_request_message(state.handle, msg, 1) ) != 0 )
    		goto done;

	if ( (ret_val = read_message(state.handle, &ans_msg) ) != 0 ) {
	      goto done;
	}

	// TODO: We are not checking, the ack is for this request. Check it!
	while ( !is_ack_message(ans_msg) ) {
	    // We can get different than ack messega here.. in this case we have to process it
	    handle_incoming_message(ans_msg);
	    
	    if ( (ret_val = read_message(state.handle, &ans_msg) ) != 0 ) {
		  goto done;
	    }	  
	}

done:
	nlmsg_free(ans_msg);
	return ret_val;
}

int initialize_director_api(void) {
	printf("Initializing director\n");

	if ( initialize_netlink_family() )
		return -EINVAL;

	 // Registers self into the kernel
	if ( register_pid(getpid()) ) {
		return -EBUSY;
	}

	printf("Generic netlink channel for director number is: %d\n", state.gnl_fid);
  	handlers[DIRECTOR_CHECK_NPM] = handle_npm_check;
	handlers[DIRECTOR_CHECK_FULL_NPM] = handle_npm_check_full;
	handlers[DIRECTOR_NODE_CONNECTED] = handle_node_connected;
	handlers[DIRECTOR_NODE_DISCONNECTED] = handle_node_disconnected;
	handlers[DIRECTOR_IMMIGRATION_REQUEST] = handle_immigration_request;
	handlers[DIRECTOR_IMMIGRATION_CONFIRMED] = handle_immigration_confirmed;
	handlers[DIRECTOR_TASK_EXIT] = handle_task_exitted;
	handlers[DIRECTOR_TASK_FORK] = handle_task_forked;
	handlers[DIRECTOR_MIGRATED_HOME] = handle_migrated_home;
	handlers[DIRECTOR_EMIGRATION_FAILED] = handle_emigration_failed;
	handlers[DIRECTOR_GENERIC_USER_MESSAGE] = handle_generic_user_message;

	printf("Comm channel initialized\n");
    
  	return 0;
}

void finalize_director_api(void) {
	// TODO: What to do here?
}
