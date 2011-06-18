#include "director.h"
#include "netlink/comm.h"
#include "netlink/npm_msg.h"
#include "netlink/node_connected_msg.h"
#include "netlink/node_disconnected_msg.h"
#include "netlink/generic_user_message_recv_msg.h"
#include "netlink/generic_user_message_send_handler.h"
#include "netlink/task_exitted_msg.h"
#include "netlink/task_forked_msg.h"
#include "netlink/immigration_request_msg.h"
#include "netlink/immigration_confirmed_msg.h"
#include "netlink/emigration_failed_msg.h"
#include "netlink/migrated_home_msg.h"

#include <linux/module.h>
#include <dbg.h>

MODULE_LICENSE("GPL");

int director_npm_check(pid_t pid, uid_t uid, int is_guest, const char* name, 
		char* __user * __user argv, char* __user * __user envp, 
		int* migman_to_use, int* migrate_home, struct rusage *rusage) {
	int res, decision, decision_value;

	// TODO: If full is not too expensive do full every time? Or perhaps some "learning" for which processes we should do full immediately and for which not?

	res = npm_check(pid, uid, is_guest, name, &decision, &decision_value, rusage);
	minfo(INFO4, "Npm check [%s]. Decision: %d, Res %d", name, decision, res);
	if ( res )
		return res;

	if ( decision == REQUIRE_ARGS || decision == REQUIRE_ENVP || decision == REQUIRE_ARGS_AND_ENVP ) {
		res = npm_check_full(pid, uid, is_guest, name, argv, envp, &decision, &decision_value);

		minfo(INFO4, "Npm check full [%s]. Decision: %d, Res %d", name, decision, res);
		
		if ( res )
			return res;
	}

	minfo(INFO4, "Npm result [%s]. Decision %d, migman %d", name, decision, decision_value);
	if ( decision == MIGRATE_BACK ) {
		*migrate_home = 1;
		return 1;
	} else if ( decision == MIGRATE ) {
		*migman_to_use = decision_value;
		return 1;
	} else if ( decision == DO_NOT_MIGRATE ) {
		return 0;
	}

	minfo(ERR1, "How did we get here %d?", decision);

	return 0;
};

EXPORT_SYMBOL(director_npm_check);

int director_immigration_request(int slot_index, uid_t uid, const char* name, int* accept) {
	return immigration_request(slot_index, uid, name, accept);
}

EXPORT_SYMBOL(director_immigration_request);

int director_immigration_confirmed(int slot_index, uid_t uid, const char* name, pid_t local_pid, pid_t remote_pid) {
	return immigration_confirmed(slot_index, uid, name, local_pid, remote_pid);
}
EXPORT_SYMBOL(director_immigration_confirmed);

int director_node_connected(const char* address, int slot_index, int auth_data_size, char* auth_data,  int* accept) {
	return node_connected(address, slot_index, auth_data_size, auth_data, accept);
}

EXPORT_SYMBOL(director_node_connected);

int director_node_disconnected(int slot_index, int detached, int reason) {
	minfo(ERR1, "Node disconnected being called");
	
	return node_disconnected(slot_index, detached, reason);
}

EXPORT_SYMBOL(director_node_disconnected);

int director_generic_user_message_recv(int node_id, int is_core_node, int slot_index, int user_data_size, char* user_data) {
	return generic_user_message_recv(node_id, is_core_node, slot_index, user_data_size, user_data);
}

EXPORT_SYMBOL(director_generic_user_message_recv);

void director_register_send_generic_user_message_handler(send_generic_user_message_handler_t handler) {
	register_send_generic_user_message_handler(handler);
}

EXPORT_SYMBOL(director_register_send_generic_user_message_handler);

int director_task_exit(pid_t pid, int exit_code, struct rusage *rusage) {
	return task_exitted(pid, exit_code, rusage);	
}

EXPORT_SYMBOL(director_task_exit);

int director_task_fork(pid_t pid, pid_t ppid) {
	/** Do not notify directory about its own forks, as this would lead to a lock-out of netlink communications */
	if ( is_director_pid(ppid) )
	    return 0;
	
	return task_forked(pid, ppid);	
}

EXPORT_SYMBOL(director_task_fork);

int director_emigration_failed(pid_t pid) {
	return emigration_failed(pid);
}

EXPORT_SYMBOL(director_emigration_failed);

int director_migrated_home(pid_t pid) {
	return migrated_home(pid);
}

EXPORT_SYMBOL(director_migrated_home);

static int __init init_director_module(void) {
	init_director_comm();
	return 0;
}

static void __exit exit_director_module(void) {	
	destroy_director_comm();
}

module_init(init_director_module);
module_exit(exit_director_module);
