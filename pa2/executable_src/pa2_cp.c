#include <ctype.h>
#include <errno.h>
#include <fcntl.h> // open()
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h> // read()

#define command "pa2_cp"
#define BUFSIZE 1024

void cp_file(char *src, char *target);

int main(int argc, char *argv[]) {
    struct stat st;
    switch (argc) {
    case 1: // Error
        fprintf(stderr, "%s: missing file operand\n", command);
        exit(1);
    case 2: // Error
        fprintf(stderr, "%s: missing destination file operand after '%s'\n", command, argv[1]);
        exit(1);
    case 3: // pa2_cp SOURCE DEST
        char *src = argv[1];
        char *dest = argv[2];

        if (stat(dest, &st) < 0 && errno != ENOENT) {
            // Error
            fprintf(stderr, "%s: cannot stat '%s': %s\n", command, dest, strerror(errno));
            exit(1);
        }
        if (S_ISDIR(st.st_mode)) { // directory
            char *filename = basename(src);
            char path[BUFSIZE];
            snprintf(path, sizeof(path), "%s/%s", dest, filename); // Safely format the string

            cp_file(src, path);
        } else if (S_ISREG(st.st_mode) || errno == ENOENT) { // no directory
            cp_file(src, dest);
        }
        break;
    default: // pa2_cp SOURCE... DIRECTORY
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
        // copy source(s) to directory
        for (int i = 1; i < argc - 1; i++) {
            char *filename = basename(argv[i]);
            char path[BUFSIZE];
            snprintf(path, sizeof(path), "%s/%s", dir_path, filename); // Safely format the string

            cp_file(argv[i], path);
        }
        break;
    }

    return 0;
}

/*
 * copy file source to target
 * if target does not exits, create the target
 */
void cp_file(char *src, char *target) {
    int fd_src, fd_dest;

    if ((fd_src = open(src, O_RDONLY)) < 0) {
        // Error
        fprintf(stderr, "%s: cannot open '%s' for reading: %s\n", command, src, strerror(errno));
        exit(1);
    }
    if ((fd_dest = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0660)) < 0) {
        // Error
        fprintf(stderr, "%s: cannot open %s: %s\n", command, target, strerror(errno));
        exit(1);
    }

    char buffer[BUFSIZE];
    int nbytes;
    while ((nbytes = read(fd_src, buffer, BUFSIZE)) > 0) { // read from src
        if (write(fd_dest, buffer, nbytes) < 0) {          // write to dest
            fprintf(stderr, "%s: %s\n", command, strerror(errno));
            close(fd_src);
            close(fd_dest);
            exit(1);
        }
    }
    if (nbytes < 0) { // Check for read error after the loop
        fprintf(stderr, "%s: %s\n", command, strerror(errno));
        close(fd_src);
        close(fd_dest);
        exit(1);
    }

    close(fd_src);
    close(fd_dest);
}