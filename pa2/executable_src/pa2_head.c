#include <ctype.h>
#include <errno.h>
#include <fcntl.h> // open()
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // read()

#define command "pa2_head"

int read_line(char **str, int fd);

int main(int argc, char *argv[]) {
    int lines = 10; // difault prints 10 lines
    char *filename;

    struct option long_options[] = {
        { "lines", required_argument, NULL, 'n' },
        { NULL, 0, NULL, 0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "n:", long_options, NULL)) != -1) {
        switch (opt) {
        case 'n':
            lines = atoi(optarg);
            // Error check
            if (lines == 0) {
                fprintf(stderr, "%s: invalid number of lines: '%s'\n", command, optarg);
                exit(1);
            }
            break;
        default:
            fprintf(stderr, "%s: wrong option(s)\n", command);
            exit(1);
        }
    }

    int fd;
    if (argc == optind || strcmp(argv[optind], "-") == 0) {
        fd = STDIN_FILENO;
    } else {
        filename = argv[optind];
        // Open files
        if ((fd = open(filename, O_RDONLY)) < 0) {
            // Error
            fprintf(stderr, "%s: cannot open '%s' for reading: %s\n", command, filename, strerror(errno));
            exit(1);
        }
    }

    char *line;
    int line_length;
    // prints up to lines
    for (int i = 0; i < lines; i++) {
        line = NULL;
        line_length = read_line(&line, fd);
        if (line_length == -1) { // EOF
            break;
        } else if (line_length == -2) { // Error
            fprintf(stderr, "%s: %s\n", command, strerror(errno));
            return -1;
        }
        // prints
        fprintf(stdout, "%s\n", line);
    }
    if (close(fd) < 0) {
        fprintf(stderr, "%s: %s\n", command, strerror(errno));
    }
    free(line);

    exit(0);
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
            return -1;       // Indicate EOF
        }
        // Handle case where file does not end with a newline
        (*str)[line_len] = '\0';
        return line_len;
    }
    // read error
    free(*str);
    *str = NULL;
    return -2;
}