#ifndef DIRECTOR_H
#define DIRECTOR_H

#include <linux/types.h>
#include <linux/resource.h>
#include "handlers.h"

/**
 * This is the main entry point that can be used by TCMI when it needs to consult user space director.
 */


/**
 * Checks, whether the process should be non-preemptively migrated to some other node.
 *
 * @param pid Pid of the process
 * @param uid Effective user ID of the process being executed
 * @param is_guest 1 if the process is guest
 * @param name File name, that is being executed
 * @param argv Args of execve
 * @param envp Envp of execve
 * @param migman_to_use Output parameter.. slot index of migration manager to be used
 * @param migrate_home Output param .. should we migrate home?
 * @param rusage Resource usage structure of process
 *
 * @return 0 on success, if no migration should be performed
 *         1 on success, when a migration should be performed
 *         error code otherwise. In case of error, output params are not valid!
 */
int director_npm_check(pid_t pid, uid_t uid, int is_guest, const char* name, char* __user * __user argv, char* __user * __user envp, int* migman_to_use, int* migrate_home, struct rusage *rusage);

/**
 * Request to immigrate process from core node specified by slot_index to this node.
 *
 * @param slot_index Index of the remote core node manager that requests immigration to this node
 * @param uid Uid of user on the remote node that is requesting the migration
 * @param name Name of the binary the process (executable) that should be immigrated
 * @param accept Output param. 0 if immigration is rejected, everything else if it is accepted
 * @return 0 on success, error code otherwise. In case of error, output params are not valid!
 */
int director_immigration_request(int slot_index, uid_t uid, const char* name, int* accept);

/**
 * Notification about successful imigration
 *
 * @param slot_index Index of the remote core node manager that requests immigration to this node
 * @param uid Uid of user on the remote node that is requesting the migration
 * @param name Name of the binary the process (executable) that should be immigrated
 * @param local_pid Local pid of the task
 * @param remote_pid Pid of the task on core node
 * @return 0 on success, error code otherwise
 */
int director_immigration_confirmed(int slot_index, uid_t uid, const char* name, pid_t local_pid, pid_t remote_pid);

/**
 * Called, when a detached node connects to the core node.
 *
 * @param address Protocol specific address string of a detached node
 * @param slot_index Index in manager that was assigned to this connection
 * @param accept Output parameter. 0 if remote node connection should be rejected, otherwise it is accepted
 * @param auth_data_size Size of authentication data, if any
 * @param auth_data Authentication data, or NULL
 * @return 0 on success, error code otherwise. In case of error, output params are not valid!
 */
int director_node_connected(const char* address, int slot_index, int auth_data_size, char* auth_data, int* accept);

/**
 * Called, when a peer node disconnets
 *
 * @param slot_index Index in manager that was assigned to this connection
 * @param detached Was the disconnected remote peer node was playing role of detached node or of a core node (in the particular connection that was broken)
 * @param reason Reason of disconnection - 0 - local request, 1 remote request
 * @return 0 on success, error code otherwise. In case of error, output params are not valid!
 */
int director_node_disconnected(int slot_index, int detached, int reason);

/**
 * Called, when a generic user message arrives
 *
 * @param node_id Id of remote node
 * @param is_core_node 1 if the slot_index is index of core node manager, 0 if it is indes of detached node manager
 * @param slot_index Index in manager that was assigned to this connection
 * @param user_data_size Size of user data
 * @param user_data User data, or NULL
 * @return 0 on success, error code otherwise. In case of error, output params are not valid!
 */
int director_generic_user_message_recv(int node_id, int is_core_node, int slot_index, int user_data_size, char* user_data);

/**
 * Called, when a task exits
 *
 * @param pid Pid of the task that exists
 * @param exit_code Exit code of the task
 * @param rusage Resource usage structure of process
 * @return 0 on success, error code otherwise. In case of error, output params are not valid!
 */
int director_task_exit(pid_t pid, int exit_code, struct rusage *rusage);

/**
 * Called, when a task forks
 *
 * @param pid Pid of a new task that was forked
 * @param ppid Pid of a parent task that was forked
 * @return 0 on success, error code otherwise. In case of error, output params are not valid!
 */
int director_task_fork(pid_t pid, pid_t ppid);

/**
 * Called, when a task emigration has failed
 *
 * @param pid Pid of a new task that failed to emigrate
 * @return 0 on success, error code otherwise.
 */
int director_emigration_failed(pid_t pid);

/**
 * Called, when a task migrates home
 *
 * @param pid Pid of a new task that returned home
 * @return 0 on success, error code otherwise.
 */
int director_migrated_home(pid_t pid);

/**
 * Registers handler for send generic user message command..
 */
void director_register_send_generic_user_message_handler(send_generic_user_message_handler_t handler);


#endif
