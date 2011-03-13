#ifndef NPM_MSG_H
#define NPM_MSG_H

#include <linux/types.h>
#include <linux/resource.h>

enum npm_msg_response {
	DO_NOT_MIGRATE, 
	MIGRATE, // In this case second return params is target migman id
	MIGRATE_BACK,
	// Following return codes indicate, that we need to provide more info to user mode director for decision
	REQUIRE_ARGS,
	REQUIRE_ENVP,
	REQUIRE_ARGS_AND_ENVP,	
};

/**
 * Checks, whether the process should be non-preemptively migrated to some other node.
 *
 * @param pid Pid of the process
 * @param uid Effective user ID of the process being executed
 * @param is_guest 1 if the process is guest
 * @param name File name, that is being executed
 * @param decision Output parameter.. element of "npm_msg_response" enum
 * @param decision_value Output param .. if result is to perform migration, this will contain slot of the migration manager to be used
 * @return 0 on success, error code otherwise. In case of error, output params are not valid!
 */
int npm_check(pid_t pid, uid_t uid, int is_guest, const char* name, int* decision, int* decision_value, struct rusage *rusage);

/**
 * Checks, whether the process should be non-preemptively migrated to some other node.
 * This version in addition passes args & envp
 *
 * @param pid Pid of the process
 * @param uid Effective user ID of the process being executed
 * @param is_guest 1 if the process is guest
 * @param name File name, that is being executed
 * @param args Args, or NULL if no args are provided
 * @param envp Envinromental properties, or NULL if no envs are provided
 * @param decision Output parameter.. element of "npm_msg_response" enum
 * @param decision_value Output param .. if result is to perform migration, this will contain slot of the migration manager to be used
 * @return 0 on success, error code otherwise. In case of error, output params are not valid!
 */
int npm_check_full(pid_t pid, uid_t uid, int is_guest, const char* name, char __user * __user * args, char __user* __user* envp, int* decision, int* decision_value);

#endif
