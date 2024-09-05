#ifndef MY_STDIO
#define MY_STDIO

void print_line(char *str);
void print_error(char* message);
int read_line(char** str, int fd);
void print_int(int number);

#endif