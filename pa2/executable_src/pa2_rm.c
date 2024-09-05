#include <ctype.h>
#include <errno.h>
#include <fcntl.h> // open()
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h> // read(), unlink()

#define command "pa2_rm"

int main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(stderr, "%s: missing operans\n", command);
    }
    for (int i = 1; i < argc; i++) {
        char *target = argv[i];
        if (unlink(target) < 0) {
            fprintf(stderr, "%s: cannot remove '%s': %s\n", command, target, strerror(errno));
        }
    }
    return errno;
}