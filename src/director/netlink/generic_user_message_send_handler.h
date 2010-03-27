#ifndef GENERIC_USER_MESSAGE_SEND_HANDLER_H
#define GENERIC_USER_MESSAGE_SEND_HANDLER_H

#include "../handlers.h"

void register_send_generic_user_message_handler(send_generic_user_message_handler_t handler);

int handle_send_generic_user_message(struct sk_buff *skb, struct genl_info *info);

#endif
