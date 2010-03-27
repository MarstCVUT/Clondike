#ifndef IMMIGRATION_REQUEST_H
#define IMMIGRATION_REQUEST_H

struct nl_msg;

int handle_immigration_request(struct nl_msg *req_msg);

#endif
