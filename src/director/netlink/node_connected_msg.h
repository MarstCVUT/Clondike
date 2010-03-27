#ifndef NODE_CONNECTED_MSG_H
#define NODE_CONNECTED_MSG_H

#include <linux/types.h>

/**
 * Called, when a detached node connects to the core node.
 *
 * @param address Protocol specific address string of a detached node
 * @param slot_index Index in manager that was assigned to this connection
 * @param auth_data_size Size of authentication data, if any
 * @param auth_data Authentication data, or NULL
 * @param accept Output parameter. 0 if remote node connection should be rejected, otherwise it is accepted
 * @return 0 on success, error code otherwise. In case of error, output params are not valid!
 */
int node_connected(const char* address, int slot_index, int auth_data_size, char* auth_data, int* accept);

#endif
