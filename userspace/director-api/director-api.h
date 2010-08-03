#ifndef DIRECTOR_API_H
#define DIRECTOR_API_H

#include "callbacks.h"

/**
 * Registeres current process into a director API
 * @return 0 on success
 */
int initialize_director_api(void);

/** 
 * Runs loop that waits for a new messages and dispatches them 
 *
 * @param @allow_block 0, if no blocking is allowed and the method should return in case it would block 
 *                     (it will process as many messages without blocking as it can and then return)
 */
int run_processing_callback(int allow_block);

/**
 * Returns fd of netlink channel.. this can be used by the caller for waiting on it.
 * Note: It is not nice to expose that, but it was done for ruby API, which is using green threads and needs IO multiplexing to
 * to work properly
 *
 * @return file descriptor of netlink channel
 */
int get_netlink_fd(void);

/** Destroys all related structures and unregisteres from the kernel director module */
void finalize_director_api(void);

/** Registers callback handler for npm_check */
void register_npm_check_callback(npm_check_callback_t callback);
/** Registers callback handler for npm_check_full */
void register_npm_check_full_callback(npm_check_full_callback_t callback);
/** Registers callback handler for node connected message */
void register_node_connected_callback(node_connected_callback_t callback);
/** Registers callback handler for node disconnected message */
void register_node_disconnected_callback(node_disconnected_callback_t callback);
/** Registers callback handler for immigration request message */
void register_immigration_request_callback(immigration_request_callback_t callback);
/** Registers callback handler for task exit message */
void register_task_exitted_callback(task_exitted_callback_t callback);
/** Registers callback handler for generic user message */
void register_generic_user_message_callback(generic_user_message_callback_t callback);
/** Sends a user message to remote node via associated kernel connection. */
int send_user_message(int target_slot_type, int target_slot_index, int data_length, char* data);

#endif
