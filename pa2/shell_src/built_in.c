#include "built_in.h"
#include "parser.h"
#include <ctype.h> // isdigit()
#include <errno.h> // errno
#include <fcntl.h>
#include <limits.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h> // signal()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void free_job_table(Job *job_table, int job_index) {
    if (!job_table)
        return; // Safety check

    for (int i = 0; i <= job_index; i++) {
        Job *job = &job_table[i];
        for (int j = 0; j < job->command_count; j++) {
            Command *cmd = &job->commands[j];

            // Free redirection for input if exists
            if (cmd->redirection_in) {
                if (cmd->redirection_in->pathname) {
                    free(cmd->redirection_in->pathname);
                    cmd->redirection_in->pathname = NULL;
                }
                free(cmd->redirection_in);
                cmd->redirection_in = NULL;
            }

            // Free redirection for output if exists
            if (cmd->redirection_out) {
                if (cmd->redirection_out->pathname) {
                    free(cmd->redirection_out->pathname);
                    cmd->redirection_out->pathname = NULL;
                }
                free(cmd->redirection_out);
                cmd->redirection_out = NULL;
            }
        }
        // job->command_count = 0; // Reset the command count to 0
        free(job->input);
    }

    // Finally, free the job table itself
    free(job_table);
}

void built_in_cd(Command *process) {
    if (process->arg_count == 0) {
        chdir(getenv("HOME"));
    } else if (process->arg_count == 1) {
        if (chdir(process->args[0]) < 0) {
            fprintf(stderr, "pa2_shell: cd: %s\n", strerror(errno));
        }
    } else {
        fprintf(stderr, "pa2_shell: cd: too many arguments\n");
    }
}

void built_in_pwd(Command *process) {
    if (process->arg_count == 0) {
        printf("%s\n", getcwd(NULL, PATH_MAX));
        return;
    } else if (process->arg_count == 1 && strcmp(process->args[0], "-L") == 0) {
        printf("%s\n", getenv("PWD"));
        return;
    } else if (process->arg_count == 1 && strcmp(process->args[0], "-P") == 0) {
        printf("%s\n", getcwd(NULL, PATH_MAX));
        return;
    } else {
        fprintf(stderr, "pa2_shell: pwd: wrong arguments\n");
    }
}

void built_in_exit(int arg_count, char **args, Job *job_table, int job_index, int free_flag) {
    // arg_count >= 2
    if (arg_count > 1) {
        fprintf(stderr, "pa2_shell: exit: too many arguments\n");
        return;
    }
    if (free_flag) {
        free_job_table(job_table, job_index);
    }

    // arg_count == 0
    if (arg_count == 0) {
        exit(0);
    }
    // arg_count == 1
    // numeric character check
    for (int i = 0; i < strlen(args[0]); i++) {
        if (!isdigit(args[0][i])) {
            fprintf(stderr, "pa2_shell: '%s': numberic argument required\n", args[0]);
            exit(2);
        }
    }
    int tmp;
    if ((tmp = atoi(args[0])) > 255) {
        exit(255);
    } else {
        exit(tmp);
    }
}

char *get_state(JobState state) {
    switch (state) {
    case RUNNING:
        return "Running";
    case STOPPED:
        return "Stopped";
    case TERMINATED:
        return "Done";
    }
    return NULL;
}

void built_in_jobs(Job *job_table, int job_index) {
    int bg_count = 0;
    for (int i = 0; i < job_index; i++) {
        if (job_table[i].is_foreground == 0) {
            bg_count++;
        }
    }
    Job *bg_jobs = malloc(bg_count * sizeof(Job));
    int job_i = 0;
    for (int bg_i = 0; bg_i < bg_count; bg_i++) {
        while (job_table[job_i].is_foreground) {
            job_i++;
        }
        bg_jobs[bg_i] = job_table[job_i];
        job_i++;
    }

    if (bg_count == 0)
        return;
    if (bg_count == 1) {
        fprintf(stdout, "[%d]+\t%s\t\t%s\n", bg_jobs[0].pgid, get_state(bg_jobs[0].state), bg_jobs[0].input);
    } else if (bg_count == 2) {
        fprintf(stdout, "[%d]-\t%s\t\t%s\n", bg_jobs[0].pgid, get_state(bg_jobs[0].state), bg_jobs[0].input);
        fprintf(stdout, "[%d]+\t%s\t\t%s\n", bg_jobs[1].pgid, get_state(bg_jobs[1].state), bg_jobs[1].input);
    } else {
        int print_i;
        for (print_i = 0; print_i < bg_count - 2; print_i++) {
            fprintf(stdout, "[%d]\t%s\t\t%s\n", bg_jobs[print_i].pgid, get_state(bg_jobs[print_i].state), bg_jobs[print_i].input);
        }
        fprintf(stdout, "[%d]-\t%s\t\t%s\n", bg_jobs[print_i].pgid, get_state(bg_jobs[print_i].state), bg_jobs[print_i].input);
        print_i++;
        fprintf(stdout, "[%d]+\t%s\t\t%s\n", bg_jobs[print_i].pgid, get_state(bg_jobs[print_i].state), bg_jobs[print_i].input);
    }
}