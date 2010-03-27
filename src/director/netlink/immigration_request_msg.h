#ifndef IMMIGRATION_REQUEST_MSG_H
#define IMMIGRATION_REQUEST_MSG_H

#include <linux/types.h>

/**
 * Request to immigrate process from core node specified by slot_index to this node.
 *
 * @param slot_index Index of the remote core node manager that requests immigration to this node
 * @param uid Uid of user on the remote node that is requesting the migration
 * @param name Name of the binary the process (executable) that should be immigrated
 * @param accept Output param. 0 if immigration is rejected, everything else if it is accepted
 * @return 0 on success, error code otherwise. In case of error, output params are not valid!
 */
int immigration_request(int slot_index, uid_t uid, const char* name, int* accept);

#endif
