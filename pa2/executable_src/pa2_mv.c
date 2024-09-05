#include <ctype.h>
#include <errno.h>
#include <fcntl.h> // open()
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h> // read()

#define command "pa2_mv"
#define BUFSIZE 1024

int main(int argc, char *argv[]) {
    struct stat st;
    switch (argc) {
    case 1: // Error
        fprintf(stderr, "%s: missing file operand\n", command);
        exit(1);
    case 2: // Error
        fprintf(stderr, "%s: missing destination file operand after '%s'\n", command, argv[1]);
        exit(1);
    case 3: // pa2_mv SOURCE DEST
        char *src = argv[1];
        char *dest = argv[2];

        // error check
        if (stat(dest, &st) < 0 && errno != ENOENT) {
            // Error
            fprintf(stderr, "%s: cannot stat '%s': %s\n", command, dest, strerror(errno));
            exit(1);
        }

        // move src to target
        if (S_ISDIR(st.st_mode)) { // directory
            char *filename = basename(src);
            char target[BUFSIZE];
            snprintf(target, sizeof(target), "%s/%s", dest, filename); // Safely format the string
            if (rename(src, target) < 0) {
                fprintf(stderr, "%s: can not move '%s'to '%s': %s\n", command, src, target, strerror(errno));
                exit(1);
            }
        } else if (S_ISREG(st.st_mode) || errno == ENOENT) { // no directory
            if (rename(src, dest) < 0) {
                fprintf(stderr, "%s: can not move '%s' to '%s': %s\n", command, src, dest, strerror(errno));
                exit(1);
            }
        }
        break;
    default: // pa2_mv SOURCE... DIRECTORY
        char *dir_path = argv[argc - 1];
        if (stat(dir_path, &st) < 0) {
            // Error
            fprintf(stderr, "%s: cannot stat '%s': %s\n", command, dir_path, strerror(errno));
            exit(1);
        }
        if (!S_ISDIR(st.st_mode)) { // Error
            fprintf(stderr, "%s: target '%s' is not a directory\n", command, dir_path);
            exit(1);
        }
        // move source(s) to directory
        for (int i = 1; i < argc - 1; i++) {
            char *src = argv[i];
            char *filename = basename(src);
            char target[BUFSIZE];
            snprintf(target, sizeof(target), "%s/%s", dir_path, filename); // Safely format the string

            if (rename(src, target) < 0) {
                fprintf(stderr, "%s: %s\n", command, strerror(errno));
                exit(1);
            }
        }
        break;
    }

    return 0;
}