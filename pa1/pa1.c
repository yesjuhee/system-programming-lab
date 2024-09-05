#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "my_stdio.h"
#include "my_string.h"
// #include <stdio.h>

#define MAX_INPUT_LEN 4096

int find_case_num(char* input_buffer);
void case1(char* file_name, char* input_buffer);
void case2(char* file_name, char* input_buffer);
void case3(char* file_name, char* input_buffer);
void case4(char* file_name, char* input_buffer);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        char *error_msg = "Usage: ./pa1 <filename>\n";
        write(2, error_msg, strlength(error_msg));
        return 1;
    }

    char* file_name = argv[1];
    
   

    while(1) {
        // get input
        char input_buffer[MAX_INPUT_LEN + 1];
        ssize_t nbytes;
        if((nbytes = read(0, input_buffer, MAX_INPUT_LEN + 1)) < 0) {
            print_error("read input error");
            exit(EXIT_FAILURE);
        }
        // change \n to \0
        input_buffer[--nbytes] = '\0';

        // check termination
        if (is_identical(input_buffer, "PA1EXIT")) {
            break;
        }

        // Separate cases
        int case_num = find_case_num(input_buffer);
        switch (case_num) {
            case 1:
                case1(file_name, input_buffer);
                break;
            case 2:
                case2(file_name, input_buffer);
                break;
            case 3:
                case3(file_name, input_buffer);
                break;
            case 4:
                case4(file_name, input_buffer);
                break;            
        }
        
        
    }

   

    return 0;
}

int find_case_num(char* input_buffer) {
    int i = 0;
    while (input_buffer[i] != '\0') {
        if (input_buffer[i] == ' ') return 2;
        if (input_buffer[i] == '"') return 3;
        if (input_buffer[i] == '*') return 4;
        i++;
    }
    return 1;
}