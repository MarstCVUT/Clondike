#ifndef IMMIGRATION_CONFIRMED_MSG_H
#define IMMIGRATION_CONFIRMED_MSG_H

#include <linux/types.h>

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
int immigration_confirmed(int slot_index, uid_t uid, const char* name, pid_t local_pid, pid_t remote_pid);

#endif
