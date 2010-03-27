#ifndef NPM_CHECK_H
#define NPM_CHECK_H

struct nl_msg;

int handle_npm_check(struct nl_msg *req_msg);

int handle_npm_check_full(struct nl_msg *req_msg);

#endif
