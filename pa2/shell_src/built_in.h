#ifndef BUILTIN
#define BUILTIN

#include "pa2.h"
#include "parser.h"

void free_job_table(Job *job_table, int job_index);
void built_in_exit(int arg_count, char **args, Job *job_table, int job_index, int free_flag);
void built_in_pwd(Command *process);
void built_in_cd(Command *process);
void built_in_jobs(Job *job_table, int job_index);

#endif