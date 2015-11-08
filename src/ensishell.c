/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <wordexp.h>
#include <errno.h>
#include <signal.h>

#include "variante.h"
#include "readcmd.h"
#include "tasks.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#if USE_GUILE == 1
#include <libguile.h>

Tasks task_list;

pid_t creer_processus(char ***seq, char *in, char *out) {
	int i = 0;
	/* limited to 2 commands ! */
	int pipe_in[2];
	int pipe_out[2];
	pid_t dernier_pid;
	while(seq[i] != NULL) {
		if(i >= 2) {
			close(pipe_in[0]);
			close(pipe_in[1]);
		}
		pipe_in[0] = pipe_out[0];
		pipe_in[1] = pipe_out[1];
		/* if there is a next task */
		if(seq[i+1]) {
			/* create a pipe for interprocess communication */ 
			//pipe(pipe_out);
			if (pipe(pipe_out) == -1) {
				perror("pipe");
				exit(EXIT_FAILURE);
			}
		}
		/* fork the process */
		pid_t pid_fils = fork();
		/* error while forking */
		if(pid_fils == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		/* fork successful */
		if(pid_fils == 0) {
			/* first command and input */
			if(i == 0 && in) {
				/* open gives us a descriptor - using read-only flag */
				int descripteur_in = open(in, O_RDONLY);
				/* copy file descriptor */
				dup2(descripteur_in, 0);
				close(descripteur_in);
			}
			/* subsequent commands */
			if(i != 0) {
				/* copy file descriptor */
				dup2(pipe_in[0], 0);
				/* close pipes -> input */
				close(pipe_in[0]);
				close(pipe_in[1]);
			}
			/* there is a next command */
			if(seq[i+1]) {
				/* copy file descriptor */
				dup2(pipe_out[1], 1);
				/* close pipes -> output */
				close(pipe_out[0]);
				close(pipe_out[1]);
			/* no next command but output */ 
			} else if(out) {
				/* open gives us a descriptor - using write-only and create flag */
				int descripteur_out = open(out, O_WRONLY | O_CREAT);
				/* copy file descriptor */
				dup2(descripteur_out, 1);
				close(descripteur_out);
			}
			/* if execvp returns -1, it failed */
			if(execvp(seq[i][0], seq[i]) == -1) {
				perror("execvp");
				exit(EXIT_FAILURE);
			}
		/* pid_fils is added to list of tasks */
		} else {
			task_list = (Tasks) add_task(task_list, pid_fils, seq[i][0]);
			/* no next command -> set last process */
			if(!seq[i+1]) {
				dernier_pid = pid_fils;
			}
		}
		/* no next command and multiple commands */
		if(!seq[i+1] && i > 0) {
			close(pipe_in[0]);
			close(pipe_in[1]);
		}
		/* deal with the next process */
		i++;
	}
	return dernier_pid;
}

int executer(char *line)
{
	/* Insert your code to execute the command line
	 * identically to the standard execution scheme:
	 * parsecmd, then fork+execvp, for a single command.
	 * pipe and i/o redirection are not required.
	 */
	int etat_fils;
	pid_t pid_fils;
	struct cmdline *l;
	l = parsecmd(&line);
	/* test for syntax errors */
	if(l->err) {
		printf("%s\n", l->err);
		return 1;
	}
	/* create a process according to given parameters */
	pid_fils = creer_processus(l->seq, l->in, l->out);
	/* if we have a "&" then we don't have to wait for the process to finish */
	if(!l->bg) {
		/* father has to wait for his child to terminate */
		pid_t tpid ;
		do {
			tpid = waitpid(pid_fils, &etat_fils, 0);
		} while(tpid != pid_fils);
		/* status of the child should be 0 */
	}
	return 0;
}

SCM executer_wrapper(SCM x)
{
        return scm_from_int(executer(scm_to_locale_stringn(x, 0)));
}

void timeHandler(int signum, siginfo_t *data, void *context) {
	Tasks task = find_task(task_list, data->si_pid);
	/* no corresponding task/command was found */
	if(!task) {
		printf("pid %d terminated.\n", data->si_pid);
	} else {
		struct timeval time_terminated;
		gettimeofday(&time_terminated, NULL);
		printf("pid %d:%s terminated (%ldms).\n", task->pid, task->cmd, (-task->task_time.tv_sec + time_terminated.tv_sec)*1000+(-task->task_time.tv_usec+time_terminated.tv_usec)/1000);
	}
}

#endif


void terminate(char *line) {
#ifdef USE_GNU_READLINE
	/* rl_clear_history() does not exist yet in centOS 6 */
	clear_history();
#endif
	if (line)
	  free(line);
	printf("exit\n");
	exit(0);
}


int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#ifdef USE_GUILE
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif
	
	/* create the list of tasks/commands to execute */
	task_list = create_tasks();
	
	/* sigaction handling done here */
	struct sigaction SIG_ACTION = {};
	memset(&SIG_ACTION, '\0', sizeof(SIG_ACTION));
	SIG_ACTION.sa_sigaction = &timeHandler;
	SIG_ACTION.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;
	if(sigaction(SIGCHLD, &SIG_ACTION, NULL) == -1){
		perror("SIGACTION");
	}
	
	/* infinite loop - waiting for commands */
	while (1) {
		//struct cmdline *l;
		char *line=0;
		//int i, j;
		char *prompt = "ensishell>";

		/* Readline use some internal memory structure that
		   can not be cleaned at the end of the program. Thus
		   one memory leak per command seems unavoidable yet */
		line = readline(prompt);
		if (line == 0 || ! strncmp(line,"exit", 4)) {
			terminate(line);
			continue;
		} else if(!strncmp(line,"jobs", 4)) {
			/* deal with "jobs" -> give process to list of processes */
			task_list = clean_tasks(task_list);
			print_tasks(task_list);
			continue;
		}
		
#ifdef USE_GNU_READLINE
		add_history(line);
#endif

#ifdef USE_GUILE
		/* The line is a scheme command */
		if (line[0] == '(') {
			char catchligne[strlen(line) + 256];
			sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
			scm_eval_string(scm_from_locale_string(catchligne));
			free(line);
                        continue;
                }
#endif
		/* execute the given command then loop back */
		executer(line);
	}
}
