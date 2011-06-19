#include "msg-common.h"

#include "internal.h"
#include "msgs.h"

#include <netlink/netlink.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/handlers.h>
#include <netlink/utils.h>

#include <linux/genetlink.h>

#include <errno.h>
#include <poll.h>

#define NL_MSG_PEEK		(1<<3)
#define NL_SOCK_PASSCRED	(1<<1)

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


/** Duplicated from NL internal structs.. required for fixed function */
struct nl_handle
{
	struct sockaddr_nl	h_local;
	struct sockaddr_nl	h_peer;
	int			h_fd;
	int			h_proto;
	unsigned int		h_seq_next;
	unsigned int		h_seq_expect;
	int			h_flags;
	void *		h_cb;
};


/** Fixed netlink receive function that is capable of properly receiving messages larger than pagesize */
int nl_recv_fixed(struct nl_handle *handle, struct sockaddr_nl *nla,
	    unsigned char **buf, struct ucred **creds)
{
	int n;
	int flags = 0;
	static int page_size = 0;
	struct iovec iov;
	struct msghdr msg = {
		.msg_name = (void *) nla,
		.msg_namelen = sizeof(struct sockaddr_nl),
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = NULL,
		.msg_controllen = 0,
		.msg_flags = 0,
	};
	struct cmsghdr *cmsg;
	
	if (handle->h_flags & NL_MSG_PEEK)
		flags |= MSG_PEEK;	

	if (page_size == 0)
		page_size = getpagesize() * 10;

	iov.iov_len = page_size;
	iov.iov_base = *buf = calloc(1, iov.iov_len);

retry:

	n = recvmsg(handle->h_fd, &msg, flags);
	if (!n)
		goto abort;
	else if (n < 0) {
		if (errno == EINTR) {
			NL_DBG(3, "recvmsg() returned EINTR, retrying\n");
			goto retry;
		} else if (errno == EAGAIN) {
			NL_DBG(3, "recvmsg() returned EAGAIN, aborting\n");
			goto abort;
		} else {
			free(msg.msg_control);
			free(*buf);
			*buf = 0;
			printf("Recvmsg failed with error: %d\n", errno);
			return -errno;
		}
	}

	if (iov.iov_len < n ||
	    msg.msg_flags & MSG_TRUNC) {
		/* Provided buffer is not long enough, enlarge it
		 * and try again. */
		iov.iov_len *= 2;
		iov.iov_base = *buf = realloc(*buf, iov.iov_len);
		goto retry;
	} else if (msg.msg_flags & MSG_CTRUNC) {
		msg.msg_controllen *= 2;
		msg.msg_control = realloc(msg.msg_control, msg.msg_controllen);
		goto retry;
	} else if (flags != 0) {
		/* Buffer is big enough, do the actual reading */
		flags = 0;
		goto retry;
	}

	if (msg.msg_namelen != sizeof(struct sockaddr_nl)) {
		free(msg.msg_control);
		free(*buf);
		printf("Addr no avail in packet %d vs %d\n", msg.msg_namelen, sizeof(struct sockaddr_nl));
		return -EADDRNOTAVAIL;
	}


	free(msg.msg_control);
	return n;

abort:
	free(msg.msg_control);
	free(*buf);
	return 0;
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
  

  /* read the response */
  ret_val = nl_recv_fixed(hndl, &peer, &data, NULL);
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
	if ( nl_err->error == -ENOENT ) {
	  printf("No node in target slot, cannot deliver the message\n");	  
	  goto read_error_no_print;
	} else {
	  printf("Error message response code in read: %d!!\n", nl_err->error);
	  goto read_error;
	}	    	        
    }
    // Ret_val == ZERO means that we got just ack message
  }

  *result_message = ans_msg;
  
  free(data);
  return 0;

read_error:
  printf("Read message error: %d\n", ret_val);
read_error_no_print:  
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
