#ifndef DIRECTOR_HANDLERS_H
#define DIRECTOR_HANDLERS_H

/**
 * Handler of user message transmission requests. 
 * 
 * @param is_core_node 1 if we shall send to core node manager, 0 if we shall send to detached node manager
 * @param target_slot_index Slot of a manager to sent message to
 * @returns Can return (error return codes go back to user space in error field of ack):
 *
 * ENOENT - Invalid slot_index -> No such a peer
 * ENOMEM - Run out of memory
 * EFAULT - Failed to send message for whatever reason
 */
typedef int (*send_generic_user_message_handler_t)(int is_core_node, int target_slot_index, int user_data_size, char* user_data);

#endif
