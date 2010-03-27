#include "msgs.h"
#include "genl_ext.h"
#include "comm.h"

#include <dbg.h>

#include <linux/skbuff.h>
#include "generic_user_message_send_handler.h"

/** Handler instance */
static send_generic_user_message_handler_t generic_user_message_handler = NULL;

void register_send_generic_user_message_handler(send_generic_user_message_handler_t handler) {
	generic_user_message_handler = handler;
};

int handle_send_generic_user_message(struct sk_buff *skb, struct genl_info *info) {
	struct nlattr* attr;
	char* user_data;
	int user_data_length, target_slot_index, target_slot_type;

	// No handler registered?
	if ( !generic_user_message_handler ) {
		return -EINVAL;
	}

	attr = nlmsg_find_attr(info->nlhdr, sizeof(struct genlmsghdr), DIRECTOR_A_SLOT_INDEX);
	if ( attr == NULL ) {
		return -1;
	}
	target_slot_index = nla_get_u32(attr);

	attr = nlmsg_find_attr(info->nlhdr, sizeof(struct genlmsghdr), DIRECTOR_A_SLOT_TYPE);
	if ( attr == NULL ) {
		return -1;
	}
	target_slot_type = nla_get_u32(attr);

	attr = nlmsg_find_attr(info->nlhdr, sizeof(struct genlmsghdr), DIRECTOR_A_LENGTH);
	if ( attr == NULL ) {
		return -1;
	}
	user_data_length = nla_get_u32(attr);

	attr = nlmsg_find_attr(info->nlhdr, sizeof(struct genlmsghdr), DIRECTOR_A_USER_DATA);
	if ( attr == NULL ) {
		return -1;
	}
	user_data = nla_data(attr);

	return generic_user_message_handler(target_slot_type, target_slot_index, user_data_length, user_data);
};
