#include "msgs.h"
#include "comm.h"
#include "genl_ext.h"

#include <linux/skbuff.h>
#include <linux/resource.h>
#include <dbg.h>

#include "generic_user_message_send_handler.h"

const char* DIRECTOR_CHANNEL_NAME = "DIRECTORCHNL";

/** Pid of associated user space process (if any) */
static u32 user_director_pid = 0;
/** Sequence generating unique transaction numbers */
static atomic_t director_seq = ATOMIC_INIT(1);

/** Read timeout in seconds */
static int read_timeout = 5;

static struct genl_family director_gnl_family = {
	.id = GENL_ID_GENERATE,
	.hdrsize = 0,
	.name = "DIRECTORCHNL", /*DIRECTOR_CHANNEL_NAME... MUST BE SHORTER THAN 16 CHARS!!!!!!!!!!!!!!!!! */
	.version = 1,
	.maxattr = DIRECTOR_ATTR_MAX
};

/* Attribute policies */
static struct nla_policy director_genl_policy[DIRECTOR_ATTR_MAX + 1] = {
	[DIRECTOR_A_PID] = { .type = NLA_U32 },
	[DIRECTOR_A_LENGTH] = { .type = NLA_U32 },
	[DIRECTOR_A_EXIT_CODE] = { .type = NLA_U32 },
	[DIRECTOR_A_ERRNO] = { .type = NLA_U32 },
	[DIRECTOR_A_RUSAGE] = { .type = NLA_BINARY, .len = sizeof(struct rusage) },
};

/**
 * Checks, if the transaction returned an error.
 *
 * @return 0, if there was no error, error code otherwise
 */
static int check_for_error(struct genl_info* info) {
	struct nlattr* attr;
	int err;

	attr = nlmsg_find_attr(info->nlhdr, sizeof(struct genlmsghdr), DIRECTOR_A_ERRNO);
	if ( attr == NULL ) {
		return 0;
	}

	err = nla_get_u32(attr); 
	minfo(ERR3, "Error code from user deamon: %d", err);
	return err;
}

/** Returns pid of director process if there is user-space director connected, 0 otherwise */
static int is_director_connected(void) {
	return user_director_pid;
}

int is_director_pid(pid_t ppid) {
	return (u32)ppid == user_director_pid;
}

/** Returns unique sequence number for transaction */
static int get_unique_seq(void) {
	return atomic_inc_return(&director_seq);
}

/* Callback for initial daemon registration */
static int register_pid_handler(struct sk_buff *skb, struct genl_info *info) {
	struct nlattr* attr;

	attr = nlmsg_find_attr(info->nlhdr, sizeof(struct genlmsghdr), DIRECTOR_A_PID);
	if ( attr == NULL ) {
		return -1;
	}

	user_director_pid = nla_get_u32(attr);
	minfo(INFO1, "Registered director pid: %u", user_director_pid);


	return 0;
}

static struct genl_ops register_pid_ops = {
        .cmd = DIRECTOR_REGISTER_PID,
        .flags = 0,
        .policy = director_genl_policy,
	.doit = register_pid_handler,
	.dumpit = NULL,
};

static struct genl_ops send_user_message_ops = {
        .cmd = DIRECTOR_SEND_GENERIC_USER_MESSAGE,
        .flags = 0,
        .policy = director_genl_policy,
	.doit = handle_send_generic_user_message,
	.dumpit = NULL,
};

static struct genl_ops* check_npm_ops_ref,
		      * node_connected_ops_ref,
		      * ack_ops_ref,
	    	      * immigration_request_ops_ref;

int init_director_comm(void) {
	int ret;

	ret = genl_register_family(&director_gnl_family);
	if (ret != 0)
		return ret;

	// TODO: Release temporarily allocated resources ;)

	/* Register callback for daemin PID registration */
	genl_register_ops(&director_gnl_family, &register_pid_ops);

	genl_register_ops(&director_gnl_family, &send_user_message_ops);	

	/* Register generic dispatching callback for all other calls */
	check_npm_ops_ref = genlmsg_register_tx_ops(&director_gnl_family, director_genl_policy, DIRECTOR_NPM_RESPONSE);
	if ( !check_npm_ops_ref )
		return -1;	

	node_connected_ops_ref = genlmsg_register_tx_ops(&director_gnl_family, director_genl_policy, DIRECTOR_NODE_CONNECT_RESPONSE);
	if ( !check_npm_ops_ref )
		return -1;	

	immigration_request_ops_ref = genlmsg_register_tx_ops(&director_gnl_family, director_genl_policy, DIRECTOR_IMMIGRATION_REQUEST_RESPONSE);
	if ( !immigration_request_ops_ref )
		return -1;	

	ack_ops_ref = genlmsg_register_tx_ops(&director_gnl_family, director_genl_policy, DIRECTOR_ACK);
	if ( !ack_ops_ref )
		return -1;	


	minfo(INFO3, "Director comm component initialized");

	return 0;
}

void destroy_director_comm(void) {
	//int res;

//	if ( (res = shutdown_user_daemon_request()) ) {
//		printk(KERN_ERR "Failed to stop the helper daemon: %d\n", res);
//	}

	genl_unregister_ops(&director_gnl_family, &register_pid_ops);
	genl_unregister_ops(&director_gnl_family, &send_user_message_ops);
	genl_unregister_ops(&director_gnl_family, check_npm_ops_ref);
	genl_unregister_ops(&director_gnl_family, node_connected_ops_ref);
	genl_unregister_ops(&director_gnl_family, ack_ops_ref);
	genl_unregister_ops(&director_gnl_family, immigration_request_ops_ref);
	genl_unregister_family(&director_gnl_family);

	kfree(check_npm_ops_ref);
}


/****************** Msg support ************************/

/** Helper method to create&send request */
static int msg_transaction_request(int msg_code, struct genl_tx* tx, struct msg_transaction_ops* ops, void* params, int interruptible) {
  int ret;
  void *msg_head;
  struct sk_buff *skb;
  int seq, director_pid;

  director_pid = is_director_connected();
  if ( !director_pid )
	return -EFAULT;

  skb = nlmsg_new(63000, GFP_KERNEL);
  if (skb == NULL)
      return -1;

  seq = get_unique_seq();

  msg_head = genlmsg_put(skb, director_pid, seq, &director_gnl_family, 0, msg_code);
  if (msg_head == NULL) {
      ret = -ENOMEM;
      goto failure;
  }
  
  ret = ops->create_request(skb, params);
  if (ret < 0)
      goto failure;

  ret = genlmsg_end(skb, msg_head);
  if (ret < 0)
      goto failure;

  ret = genlmsg_unicast_tx(skb, director_pid, tx, interruptible);
  if (ret != 0)
      goto failure;

  return ret;

failure:
  genlmsg_cancel(skb, msg_head);

  return ret;
}

static int msg_transaction_response(struct genl_tx* tx, struct msg_transaction_ops* ops, void* params) {
	struct sk_buff *skb;
	struct genl_info info;
	unsigned int ret = 0;

	if ( (ret= genlmsg_read_response(tx, &skb, &info, read_timeout)) )
		return ret;

	if ( (ret=check_for_error(&info)) )
		goto done;


	if ( ops->read_response ) {
	  ret = ops->read_response(&info, params);
	  if (ret < 0)
		  goto done;
	}

done:
	kfree_skb(skb);
	return ret;
}

int msg_transaction_do(int msg_code, struct msg_transaction_ops* ops, void* params, int interruptible) {
	struct genl_tx tx;

	int res = msg_transaction_request(msg_code, &tx, ops, params, interruptible);

	if ( res )
		return res;

	return msg_transaction_response(&tx, ops, params);
}
