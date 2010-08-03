#ifndef NODE_DISCONNECTED_MSG_H
#define NODE_DISCONNECTED_MSG_H

#include <linux/types.h>

/**
 * Called, when a peer node disconnets
 *
 * @param slot_index Index in manager that was assigned to this connection
 * @param detached Was the disconnected remote peer node was playing role of detached node or of a core node (in the particular connection that was broken)
 * @param reason Reason of disconnection - 0 - local request, 1 remote request
 * @return 0 on success, error code otherwise. In case of error, output params are not valid!
 */
int node_disconnected(int slot_index, int detached, int reason);

#endif
