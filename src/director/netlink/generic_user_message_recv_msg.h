#ifndef GENERIC_USER_MESSAGE_MSG_H
#define GENERIC_USER_MESSAGE_MSG_H

#include <linux/types.h>

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
int generic_user_message_recv(int node_id, int is_core_node, int slot_index, int user_data_size, char* user_data);

#endif
