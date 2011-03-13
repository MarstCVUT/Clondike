#ifndef TASK_EXITTED_MSG_H
#define TASK_EXITTED_MSG_H

#include <linux/types.h>
#include <linux/resource.h>

/**
 * Called, when a task exits
 *
 * @param pid Pid of the task that exists
 * @param exit_code Exit code of the task
 * @param rusage Resource usage of process
 * @return 0 on success, error code otherwise. In case of error, output params are not valid!
 */
int task_exitted(pid_t pid, int exit_code, struct rusage *rusage);

#endif
