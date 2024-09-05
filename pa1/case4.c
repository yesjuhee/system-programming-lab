#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "my_stdio.h"
#include "my_string.h"
#include "search.h"

void case4(char* file_name, char* input_buffer) {
    int buffer_len = strlength(input_buffer);
    // split input buffer to two words
    char* first_word = (char*) malloc(buffer_len);
    char* second_word =  (char*) malloc(buffer_len);
    // first word : before *
    int i = 0;
    while (input_buffer[i] != '*') {
        first_word[i] = input_buffer[i];
        i++;
    }
    first_word[i] = '\0';
    // second word : after * before \0
    int j = 0;
    i++;
    while (i < buffer_len) {
        second_word[j++] = input_buffer[i++];
    }
    second_word[j] = '\0';

     // open file
    int fd;
    if ((fd = open(file_name, O_RDONLY)) < 0) {
        print_error("file open error");
        exit(EXIT_FAILURE);
    }

    // read file line by line
    int line_num = 1;
    int line_length;
    int first_len = strlength(first_word);
    int second_len = strlength(second_word);
    char *line = NULL;
    while ((line_length = read_line(&line, fd)) >= 0) {
        // check whether the first word exist
        int first_word_location = search_word(line, 0, first_word);
        if (first_word_location == -1) {
            free(line);
            line_num++;
            continue;
        }
        // check whether the second word exist
        int second_word_location = search_word(line, 0, second_word);
        if (second_word_location == -1) {
            free(line);
            line_num++;
            continue;
        }
        // find the last location of `second word`
        int next_loc = second_word_location;
        while((next_loc = search_word(line, second_word_location + second_len, second_word)) != -1) {
            second_word_location = next_loc;
        }
        if (first_word_location >= second_word_location) {
            free(line);
            line_num++;
            continue;
        }

        // check there is at least a word between first/second
        for (int i = first_word_location + first_len; i < second_word_location; i++) {
            if (line[i] != ' ' && line[i] != '\t') {
                print_int(line_num);
                write(1, " ", 1);    
                break;
            }
        }

        free(line);
        line_num++;
    }
    if (line_length == -2) {
        print_error("Error reading file");
        if (line) free(line);
        free(first_word);
        free(second_word);
    }

    write(1, "\n", 1);
    free(first_word);
    free(second_word);

     // close file
    if (close(fd) < 0) {
        print_error("file close error");
        exit(EXIT_FAILURE);
    }
}