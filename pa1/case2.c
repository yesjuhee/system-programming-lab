#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "my_stdio.h"
#include "my_string.h"
#include "search.h"

void case2(char* file_name, char* input_buffer) {
    int buffer_len = strlength(input_buffer);
    // Count the number of input words
    int words_count = 1;
    for (int i = 0; i < buffer_len; i++) {
        if (input_buffer[i] == ' ') {
            words_count++;
        }
    }

    // Allocate array of strings
    char** input_words = (char**)malloc(sizeof(char*) * words_count);
    if (!input_words) {
        print_error("malloc error in case2()");
        exit(EXIT_FAILURE);
    }
    int front_idx = 0;
    int end_idx = 0;
    for (int i = 0; i < words_count; i++) {
        // Find the start of the next word
        while (input_buffer[front_idx] == ' ' && input_buffer[front_idx] != '\0') {
            front_idx++;
        }
        end_idx = front_idx;

        // Find the end of the word
        while (input_buffer[end_idx] != ' ' && input_buffer[end_idx] != '\0') {
            end_idx++;
        }
        int word_len = end_idx - front_idx;

        input_words[i] = (char*)malloc(sizeof(char) * (word_len + 1));
        for (int j = front_idx, k = 0; j < end_idx; j++, k++) {
            input_words[i][k] = input_buffer[j];
        }
        input_words[i][word_len] = '\0';
        
        front_idx = end_idx + 1;
    }

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
        int search_count = 0;
        for (int i = 0; i < words_count; i++) {
            if (search_word(line, 0, input_words[i]) != -1) {
                search_count++;
            }
        }
        if (search_count == words_count) {
            // it is matching line
            print_int(line_num);
            write(1, " ", 1);
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

    // 문자열 배열 할당 해제하기 (반복문)
    for (int i = 0; i < words_count; i++) {
        if (input_words[i] != NULL) {
            free(input_words[i]); // Free each individual string
        }
    }
    free(input_words);

     // close file
    if (close(fd) < 0) {
        print_error("file close error");
        exit(EXIT_FAILURE);
    }
}