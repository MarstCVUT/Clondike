#ifndef MSG_COMMON_H
#define MSG_COMMON_H

#include <netlink/netlink.h>
#include <netlink/msg.h>
#include <linux/genetlink.h>

#define __u64 uint64_t
#define __u32 uint32_t
#define __u16 uint16_t
#define __u8 uint8_t

#define __s32 uint32_t

struct nl_handle;

/* Callback method to be called for processing of specific command when the user daemon is running in a server mode */
typedef int (*cmd_handler)(struct nl_msg *msg);

/* Generic Netlink message header 
struct genlmsghdr {
  uint8_t cmd;
  uint8_t version;
  uint16_t reserved;
};*/

/* Helper function for generic netlink header extraction */
static inline struct genlmsghdr* nl_msg_genlhdr(struct nl_msg *msg) {
  return (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
}

int prepare_request_message(struct nl_handle* hndl, uint8_t cmd, uint16_t type, struct nl_msg** result_message);
int prepare_response_message(struct nl_handle* hndl, uint8_t cmd, uint16_t type, uint32_t seq, struct nl_msg** result_message);
int send_request_message(struct nl_handle* hndl, struct nl_msg* msg, int requires_ack);
int send_response_message(struct nl_handle* hndl, struct nl_msg* msg);
int read_message(struct nl_handle* hndl, struct nl_msg** result_message);
void send_error_message(int err, int seq, uint8_t cmd);
int send_ack_message(struct nl_msg* req_msg);

#endif
