#ifndef PA2
#define PA2

#include "parser.h"

#define MAX_JOBS 8182
#define PATH_MAX 4096

void init_pa2_shell();
void print_job(Job job);
void launch_job(Job *job_table, int job_index);
void execute_process(Command *process, Job *job_table, int job_index);
void set_redirection(Command *process);

#endif