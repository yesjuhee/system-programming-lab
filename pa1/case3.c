#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "my_stdio.h"
#include "my_string.h"
#include "search.h"

void case3(char* file_name, char* input_buffer) {
    // remove quote(")
    int phrase_len = strlength(input_buffer) - 2;
    char* input_phrase = (char*) malloc((phrase_len + 1) * sizeof(char));
    for (int i = 0; i < phrase_len; i++) {
        input_phrase[i] = input_buffer[i + 1];
    }
    input_phrase[phrase_len] = '\0';

     // open file
    int fd;
    if ((fd = open(file_name, O_RDONLY)) < 0) {
        print_error("file open error");
        exit(EXIT_FAILURE);
    }

     // read file line by line
    int line_num = 1;
    int line_length;
    char *line = NULL;
    while ((line_length = read_line(&line, fd)) >= 0) {
        int start_idx = 0;
        int return_idx;
        while((return_idx = search_phrase(line, start_idx, input_phrase)) != -1) {
            // find the input word in the line
            // stdout : [line number]:[start index of the word]
            print_int(line_num);
            write(1, ":", 1);
            print_int(return_idx);
            write(1, " ", 1);
            // keep read the line
            start_idx = return_idx + strlength(input_phrase); 
        }

        // There are no matching word in the line
        free(line);
        line_num++;
    }
    if (line_length == -2) {
        print_error("Error reading file");
        if (line) free(line);
    }

    write(1, "\n", 1);

     // close file
    if (close(fd) < 0) {
        print_error("file close error");
        exit(EXIT_FAILURE);
    }
}