#include "msg-common.h"

#include "internal.h"
#include "msgs.h"

#include <netlink/netlink.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/handlers.h>

#include <errno.h>
#include <poll.h>



/** Prepares netlink message for sending and fills the resut to the result_message parameter. Additional netlink message arguments can be added to that message */
int prepare_request_message(struct nl_handle* hndl, uint8_t cmd, uint16_t type, struct nl_msg** result_message) {
  struct nl_msg* msg = NULL;
  struct nlmsghdr *nl_hdr;
  struct genlmsghdr genl_hdr;

  int ret_val = 0;
  unsigned char* data;

  msg = nlmsg_alloc();
  if ( msg == NULL ) {
	ret_val = -ENOMEM;
	goto prepare_error;
  }

  nl_hdr = nlmsg_hdr(msg);
  nl_hdr->nlmsg_type = type;

   genl_hdr.cmd = cmd;
   genl_hdr.version = 0x1;
   genl_hdr.reserved = 0;
   if ( (ret_val=nlmsg_append(msg, &genl_hdr, sizeof(genl_hdr), 1) ) != 0)
      goto prepare_error;

  *result_message = msg;
  return 0;

prepare_error:
  *result_message = NULL;
  nlmsg_free(msg);
  return ret_val;
}

/** Prepares netlink message for sending and fills the resut to the result_message parameter. Additional netlink message arguments can be added to that message */
int prepare_response_message(struct nl_handle* hndl, uint8_t cmd, uint16_t type, uint32_t seq, struct nl_msg** result_message) {
	struct nlmsghdr *nl_hdr;
	int res;	

	res = prepare_request_message(hndl, cmd, type, result_message);
	if ( res )
		return res;

	nl_hdr = nlmsg_hdr(*result_message);	
	nl_hdr->nlmsg_seq = seq;

	return 0;	
}


/** Sends netlink message and destroys it. It is guaranteed that msg will be freed after this call!  */
int send_request_message(struct nl_handle* hndl, struct nl_msg* msg, int requires_ack) {
  struct nlmsghdr *nl_hdr;
  int ret_val;

  nl_hdr = nlmsg_hdr(msg);

  //printf("Sending msg\n");

  if ( msg == NULL ) 
	return -EINVAL;

  if ( requires_ack )
  	nl_hdr->nlmsg_flags |= NLM_F_ACK;
  else 
  	nl_hdr->nlmsg_flags |= NLM_F_REQUEST;

  ret_val = nl_send_auto_complete(hndl, msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;

    goto send_error;
  }

 ret_val = 0;

 send_error:
  nlmsg_free(msg);
  return ret_val;
}


/** Sends netlink message and destroys it. It is guaranteed that msg will be freed after this call!  */
int send_response_message(struct nl_handle* hndl, struct nl_msg* msg) {
	return send_request_message(hndl, msg, 0);
}

/** Reads a netlink message */
int read_message(struct nl_handle* hndl, struct nl_msg** result_message) {
  struct sockaddr_nl peer;
  struct nlmsghdr *nl_hdr;
  struct nl_msg *ans_msg = NULL;
  unsigned char* data = NULL;
  int ret_val;
  struct pollfd pollfds[1];
  
  pollfds[0].fd = nl_socket_get_fd(hndl);
  pollfds[0].events = POLLIN;

  ret_val = poll(pollfds, 1, -1);
  if ( ret_val == -1 ) {
	ret_val = -errno;
	goto read_error;
  }
  

  //printf("Receiving\n");
  /* read the response */
  ret_val = nl_recv(hndl, &peer, &data, NULL);
  //printf("After rcv %d\n", ret_val);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto read_error;
  }  

  nl_hdr = (struct nlmsghdr *)data;

 // This makes a new buffer! Old buffer (data) has to be freed!
  ans_msg = nlmsg_convert((struct nlmsghdr *)data);
  
  /* process the response */
  if (!nlmsg_ok(nl_hdr, ret_val)) {
    ret_val = -EBADMSG;
    goto read_error;
  }

  if (nl_hdr->nlmsg_type == NLMSG_NOOP ||
      nl_hdr->nlmsg_type == NLMSG_OVERRUN) {
    ret_val = -EBADMSG;
    goto read_error;
  }

  if ( nl_hdr->nlmsg_type == NLMSG_ERROR ) {
    struct nlmsgerr* nl_err = (struct nlmsgerr*)nlmsg_data(nlmsg_hdr(ans_msg));
    ret_val = nl_err->error;
    if ( ret_val != 0 ) {
    	printf("Error message response code in read: %d!!\n", nl_err->error);    
        goto read_error;
    }
    // Ret_val == ZERO means that we got just ack message
  }

  *result_message = ans_msg;
  
  free(data);
  return 0;

read_error:
  free(data);
  nlmsg_free(ans_msg);
  *result_message = NULL;
  return ret_val;
}

/** Sends error response to the kernel module */
void send_error_message(int err, int seq, uint8_t cmd) {
	struct nl_msg *msg;
	int ret;
	struct internal_state* state = get_current_state();

	//printf("Preparing error message: Err %d  Seq %d Cmd %d\n", err, seq, cmd);	
	if ( (ret=prepare_response_message(state->handle, cmd, state->gnl_fid, seq, &msg) ) != 0 ) {
		return;
	}
	
	ret = nla_put_u32(msg,
			DIRECTOR_A_ERRNO,
			err);
	
	if (ret != 0) {
		nlmsg_free(msg);
		return;
	}

	ret = send_response_message(state->handle, msg);
}

/** Sends ack message to kernel */
int send_ack_message(struct nl_msg* req_msg) {
	struct nlmsghdr *nl_hdr;
	struct nl_msg *msg;
	int ret;
	int seq;
	struct internal_state* state = get_current_state();

	nl_hdr = nlmsg_hdr(req_msg);
	seq = nl_hdr->nlmsg_seq;
	
	if ( (ret=prepare_response_message(state->handle, DIRECTOR_ACK, state->gnl_fid, seq, &msg) ) != 0 ) {
		return ret;
	}
	
	return send_response_message(state->handle, msg);
}
