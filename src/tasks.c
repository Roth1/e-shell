#include "tasks.h"

Tasks create_tasks(void ) {
    return NULL;
}

// Add the new task at the start of the list.
Tasks add_task(Tasks task, pid_t pid, char *cmd) {
    Tasks new_task = malloc(sizeof(*new_task));
    new_task->next = task;
    new_task->pid = pid;
    new_task->cmd = malloc((strlen(cmd)+1)*sizeof(char));
    strcpy(new_task->cmd, cmd);
    gettimeofday(&(new_task->task_time), NULL);
    return new_task;
}
    
// Print the task list.
void print_tasks(Tasks task) {
    while(task!=NULL) {
        printf("%d is %s \n",task->pid, task->cmd);
        task = task->next;
    }
}

Tasks clean_tasks(Tasks task) {
    Tasks tmp = NULL;
    int status = 0;
    pid_t pid = 0;
    if(task == NULL) {
        return NULL;
    }
    // Recursive function, clean the last first.
    task->next = clean_tasks(task->next);
    pid = waitpid(task->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
    if(pid == -1) {// Case task finished
        tmp = task;
        task = task->next;
        free(tmp->cmd);
        free(tmp);
    }
    return task;
}

//Find a task in the list.
//Return its pid if it's present, else return 0.
Tasks find_task(Tasks task, pid_t pid) {
    while(task!=NULL) {
        if(task->pid == pid) {
            return task;
        }
        task = task->next;
    }
    return NULL;
}
