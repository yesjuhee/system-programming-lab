#include "my_string.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void print_line(char *str) {
    write(1, str, strlength(str));
    write(1, "\n", 1);
}

void print_error(char *message) {
    char *errno_str = itoa(errno);
    write(1, "error(", strlength("error)"));
    write(1, errno_str, strlength(errno_str));
    write(1, "): ", strlength("): "));
    write(1, message, strlength(message));
    write(1, "\n", 1);
    free(errno_str);
}

void print_int(int number) {
    char *string = itoa(number);
    write(1, string, strlength(string));
    free(string);
}

/*
 * return the length of the line (non-negative integer)
 * return -1 if it reach to EOF
 * return -2 to error
 */
int read_line(char **str, int fd) {
    int buf_size = 128; // Initial buffer size
    *str = malloc(buf_size);
    if (!*str) {
        print_error("malloc error in get_line()");
        return -2;
    }

    int line_len = 0;
    char ch;
    int nbytes;
    while ((nbytes = read(fd, &ch, 1)) > 0) {
        // Check for buffer expansion
        if (line_len == buf_size - 1) {
            buf_size *= 2; // Double the buffer size
            char *new_str = realloc(*str, buf_size);
            if (!new_str) {
                free(*str);
                print_error("realloc error in get_line()");
                return -2;
            }
            *str = new_str;
        }

        if (ch == '\n') {
            (*str)[line_len] = '\0';

            return line_len; // Successfully read a line
        }

        (*str)[line_len++] = ch;
    }

    if (nbytes == 0) {
        // EOF
        if (line_len == 0) { // EOF without reading anything
            free(*str);
            return -1; // Indicate EOF
        }

        // Handle case where file does not end with a newline
        (*str)[line_len] = '\0';
        return line_len;
    }

    // read error
    print_error("read file error in get_line()");
    free(*str);
    *str = NULL;
    return -2;
}