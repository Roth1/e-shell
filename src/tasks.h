#ifndef __TASKS_H
#define __TASKS_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>

typedef struct tasks_list {
    struct tasks_list *next;
    pid_t pid;
    char *cmd;
    struct timeval task_time;
} *Tasks;

Tasks create_tasks(void);
Tasks add_task(Tasks task, pid_t pid, char *cmd);
void print_tasks(Tasks task);
Tasks clean_tasks(Tasks task);
Tasks find_task(Tasks task, pid_t pid);

#endif
