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

int main() {
    Token tokens[MAX_TOKENS];
    Job *job_table = malloc(MAX_JOBS * sizeof(Job));
    int job_index = 0;
    init_pa2_shell();

    while (1) {
        char *input;
        if ((input = readline("$ ")) == NULL)
            break;

        add_history(input);
        lex(input, tokens);
        Job *job = &job_table[job_index];
        parse(tokens, job);
        job->input = input;
        if (job->command_count == 1 && strncmp(input, "exit", 4) == 0) {
            built_in_exit(job->commands[0].arg_count, job->commands[0].args, job_table, job_index, 1);
        } else if (job->command_count == 1 && strncmp(input, "pwd", 3) == 0) {
            built_in_pwd(&(job->commands[0]));
        } else if (job->command_count == 1 && strncmp(input, "cd", 2) == 0) {
            built_in_cd(&job->commands[0]);
        } else if (job->command_count == 1 && strncmp(input, "jobs", 4) == 0) {
            built_in_jobs(job_table, job_index);
        } else {
            launch_job(job_table, job_index);
        }

        free_tokens(tokens);
        job_index++;
    }

    return 0;
}

void if (listen(listen_fd, 5) < 0) {
    fprintf(stderr, "listen() failed.\n");
    exit(3);
}
execute_process(Command *process, Job *job_table, int job_index) {
    // executables
    if (strlen(process->command) > 3 && strncmp(process->command, "pa2", 3) == 0) {
        char command[20];
        snprintf(command, sizeof(command), "./bin/%s", process->command);
        process->args[process->arg_count + 1] = NULL;
        for (int i = process->arg_count; i > 0; i--) {
            process->args[i] = process->args[i - 1];
        }
        process->args[0] = command;
        execvp(process->args[0], process->args);
    }
    // built-in
    else if (strcmp(process->command, "bg") == 0) {
        // printf("bg\n");
    } else if (strcmp(process->command, "fg") == 0) {
        // printf("fg\n");
    } else if (strcmp(process->command, "jobs") == 0) {
        built_in_jobs(job_table, job_index);
        _exit(0);
    } else if (strcmp(process->command, "cd") == 0) {
        built_in_cd(process);
        _exit(0);
    } else if (strcmp(process->command, "pwd") == 0) {
        built_in_pwd(process);
        _exit(0);
    } else if (strcmp(process->command, "exit") == 0) {
        built_in_exit(process->arg_count, process->args, job_table, job_index, 0);
        _exit(0);
    }
    // others
    else {
        process->args[process->arg_count + 1] = NULL;
        for (int i = process->arg_count; i > 0; i--) {
            process->args[i] = process->args[i - 1];
        }
        process->args[0] = process->command;
        execvp(process->args[0], process->args);
    }

    fprintf(stderr, "%s: command not found\n", process->command);
    _exit(0);
}

void launch_job(Job *job_table, int job_index) {
    Job *job = &job_table[job_index];
    Command *processes = job->commands;
    int process_count = job->command_count;
    pid_t pid;
    int shell_fd = STDIN_FILENO;
    int pipefd[2], infile_fd, outfile_fd;

    infile_fd = STDIN_FILENO;
    for (int process_i = 0; process_i < process_count; process_i++) {
        Command *process = &processes[process_i];

        if (process_i < process_count - 1) {
            if (pipe(pipefd) < 0) {
                fprintf(stderr, "pa2_shell: %s\n", strerror(errno));
                return;
            }
            outfile_fd = pipefd[1];
        } else { // 마지막 프로세스
            outfile_fd = STDOUT_FILENO;
        }

        switch (pid = fork()) {
        case -1: // fork error
            fprintf(stderr, "pa2_shell: %s\n", strerror(errno));
            exit(1);
            break;
        case 0: // Child
            // Set pgid
            process->pid = getpid();
            if (job->pgid == 0) { // if it is leader process
                job->pgid = process->pid;
            }
            setpgid(process->pid, job->pgid);
            // Set state
            job->state = RUNNING;
            // if it is foreground job
            if (job->is_foreground) {
                // get the control of terminal
                tcsetpgrp(shell_fd, job->pgid);
            }
            // Reset Signals
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            // set redirection
            set_redirection(process);
            // set pipe
            if (infile_fd != STDIN_FILENO) {
                dup2(infile_fd, STDIN_FILENO);
                close(infile_fd);
            }
            if (outfile_fd != STDOUT_FILENO) {
                dup2(outfile_fd, STDOUT_FILENO);
                close(outfile_fd);
            }
            // execute commands
            execute_process(process, job_table, job_index);
            break;
        default: // Parent
            int wait_status;
            int shell_pgid = getpgrp();
            int process_pid = pid;
            process->pid = process_pid;
            // seg pgid of child
            if (job->pgid == 0) { // if it is leader process
                job->pgid = process_pid;
            }
            setpgid(process_pid, job->pgid);

            // signal(SIGCHLD, sigchld_handler);

            // set control of terminal
            if (job->is_foreground) {
                tcsetpgrp(shell_fd, job->pgid);        // give the control to child
                waitpid(pid, &wait_status, WUNTRACED); // wait until child stop or terminate
                tcsetpgrp(shell_fd, shell_pgid);       // get the control again
            }

            if (process_i == process_count - 1 && job->is_foreground == 0) {
                fprintf(stdout, "[%d] %d\n", job->pgid, process_pid);
            }

            break;
        }

        if (infile_fd != STDIN_FILENO) {
            close(infile_fd);
        }
        if (outfile_fd != STDOUT_FILENO) {
            close(outfile_fd);
        }
        infile_fd = pipefd[0];
    }
}

void set_redirection(Command *process) {
    int fd_in, fd_out;
    if (process->redirection_in) {
        // open file
        if ((fd_in = open(process->redirection_in->pathname, O_RDONLY)) == -1) {
            fprintf(stderr, "pa2_shell: %s: %s\n", process->redirection_in->pathname, strerror(errno));
            _exit(1);
        }
        // redirect in
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }
    if (process->redirection_out) {
        if (process->redirection_out->type == REDIR_OUT) {
            if ((fd_out = open(process->redirection_out->pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
                fprintf(stderr, "pa2_shell: %s: %s\n", process->redirection_out->pathname, strerror(errno));
                _exit(1);
            }
            // redirect out
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        } else if (process->redirection_out->type == REDIR_APPEND) {
            if ((fd_out = open(process->redirection_out->pathname, O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1) {
                fprintf(stderr, "pa2_shell: %s: %s\n", process->redirection_out->pathname, strerror(errno));
                _exit(1);
            }
            // redirect out
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
    }
}

void init_pa2_shell() {
    // Ignore Signals
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    // get congrol of the terminal
    tcsetpgrp(STDIN_FILENO, getpid());
}

void print_job(Job job) {
    printf("is_foreground: %d\n", job.is_foreground);
    printf("command_count: %d\n", job.command_count);
    for (int i = 0; i < job.command_count; i++) {
        printf("[%d]: %s\nargs: ", i, job.commands[i].command);
        for (int j = 0; j < job.commands[i].arg_count; j++) {
            printf("%s ", job.commands[i].args[j]);
        }
        printf("\n");
        if (job.commands[i].redirection_in != NULL) {
            printf("redirection in '%s'\n", job.commands[i].redirection_in->pathname);
        }
        if (job.commands[i].redirection_out != NULL) {
            printf("redirection out '%s'\n", job.commands[i].redirection_out->pathname);
        }
        printf("inputs: %s\n", job.input);
    }
}