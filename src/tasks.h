#ifndef _TASKS_H_
#define _TASKS_H_

#include <relevant.h>

typedef struct tasks_list {
    struct tasks_list *next;
    pid_t pid;
    char *cmd;
} *Tasks;

Tasks create_tasks(void );
Tasks add_task(Tasks task, pid_t pid; char *cmd);
void print_tasks(Tasks task);
Tasks clean_tasks(Tasks task);
Tasks find_task(Tasks task, pid_t pid);

#endif
