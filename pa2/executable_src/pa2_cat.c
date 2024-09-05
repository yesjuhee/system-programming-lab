#include <ctype.h>
#include <errno.h>
#include <fcntl.h> // open()
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // read()

#define command "pa2_cat"
#define BUFSIZE 1024

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        char *filename = argv[i];
        int fd = STDIN_FILENO;
        if (strcmp(filename, "-") != 0) {
            // Open files
            if ((fd = open(filename, O_RDWR)) < 0) {
                // Error
                fprintf(stderr, "%s: '%s': %s\n", command, filename, strerror(errno));
                exit(1);
            }
        }
        // read the file and write to stdout
        ssize_t nbytes;
        char buffer[BUFSIZE];
        while ((nbytes = read(fd, buffer, BUFSIZE)) > 0) {
            if (write(STDOUT_FILENO, buffer, nbytes) != nbytes) {
                fprintf(stderr, "%s: %s\n", command, strerror(errno));
                close(fd);
                exit(1);
            }
        }
        if (nbytes < 0) { // Check for read error after the loop
            fprintf(stderr, "%s: %s\n", command, strerror(errno));
            close(fd);
            exit(1);
        }
    }

    // if no FILE is provided
    if (argc == 1) {
        int fd = STDIN_FILENO;
        // read the file and write to stdout
        ssize_t nbytes;
        char buffer[BUFSIZE];
        while ((nbytes = read(fd, buffer, BUFSIZE)) > 0) {
            if (write(STDOUT_FILENO, buffer, nbytes) != nbytes) {
                fprintf(stderr, "%s: %s\n", command, strerror(errno));
                close(fd);
                exit(1);
            }
        }
        if (nbytes < 0) { // Check for read error after the loop
            fprintf(stderr, "%s: %s\n", command, strerror(errno));
            close(fd);
            exit(1);
        }
    }

    return 0;
}