#ifndef DIRECTOR_CALLBACKS_H
#define DIRECTOR_CALLBACKS_H

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

enum npm_msg_response {
	DO_NOT_MIGRATE, 
	MIGRATE, // In this case second return params is target migman id
	MIGRATE_BACK,
	// Following return codes indicate, that we need to provide more info to user mode director for decision
	REQUIRE_ARGS,
	REQUIRE_ENVP,
	REQUIRE_ARGS_AND_ENVP,	
};


typedef void (*npm_check_callback_t)(pid_t pid, uid_t uid, int is_guest, const char* name, struct rusage *rusage, int* decision, int* decision_value);

typedef void (*npm_check_full_callback_t)(pid_t pid, uid_t uid, int is_guest, const char* name, char** args, char** envp, int* decision, int* decision_value);

typedef void (*immigration_request_callback_t)(uid_t uid, int slot_index, const char* exec_name, int* accept);

typedef void (*immigration_confirmed_callback_t)(uid_t uid, int slot_index, const char* exec_name, pid_t local_pid, pid_t remote_pid);

typedef void (*node_connected_callback_t)(char* address, int slot_index, int auth_data_size, const char* auth_data, int* accept);

typedef void (*node_disconnected_callback_t)(int slot_index, int slot_type, int reason);

typedef void (*task_exitted_callback_t)(pid_t pid, int exit_code, struct rusage *rusage);

typedef void (*task_forked_callback_t)(pid_t pid, pid_t ppid);

typedef void (*migrated_home_callback_t)(pid_t pid);

typedef void (*emigration_failed_callback_t)(pid_t pid);

typedef void (*generic_user_message_callback_t)(int node_id, int slot_type, int slot_index, int user_data_size, char* user_data);

#endif
