#ifndef DIRECTOR_COMM_H
#define DIRECTOR_COMM_H

#include <linux/types.h>

/** Initializes director generic netlink family */
int init_director_comm(void);
/** Finalizes director generic netlink family */
void destroy_director_comm(void);


/******** Message support **********/


struct sk_buff;
struct genl_info;

/** Specific operations that are performed on the message */
struct msg_transaction_ops {
	/** Fills in request with params. Return 0 on success */
	int (*create_request)(struct sk_buff *skb, void* params);
	/** Reads response data. Return 0 on success */
	int (*read_response)(struct genl_info* info, void* params);
};

/**
 * Performs a netlink transaction.
 *
 * 1) Creates a message (with specified code)
 * 2) Uses "create_resquest" method to fill params
 * 3) Sends the request
 * 4) Reads response
 * 5) Uses "read_response" to parse output data
 *
 * @param msg_code Code of the message to be created
 * @param ops Transaction specific operations to fill params and read response
 * @param params Structure that will contain input params and hold output values
 * @param interruptible if !=0, the waiting for response can be interrupted by a signal
 * @return 0 on success, error code otherwise
 */
int msg_transaction_do(int msg_code, struct msg_transaction_ops* ops, void* params, int interruptible);

/** Returns 1 if, ppid is equal to userspace director pid */
int is_director_pid(pid_t ppid);

#endif
