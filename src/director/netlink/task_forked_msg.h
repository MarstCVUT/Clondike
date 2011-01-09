#ifndef TASK_FORKED_MSG_H
#define TASK_FORKED_MSG_H

#include <linux/types.h>

/**
 * Called, when a task forks
 *
 * @param pid Pid of the task that exists
 * @param ppid Pid of a parent task that was forked
 * @return 0 on success, error code otherwise. In case of error, output params are not valid!
 */
int task_forked(pid_t pid, pid_t ppid);

#endif
