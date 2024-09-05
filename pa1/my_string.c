#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

size_t strlength(char *str) {
    char *s;
    for (s = str; *s; ++s) {}
    return (s - str);
}

/*
* change integer to ascii
* you should free the return char pointer
*/
char* itoa(int num) {
    char* str;

    // check zero
    if (num == 0) {
        str = (char*)malloc(2 * sizeof(char));
        str[0] = '0';
        str[1] = '\0';
        return str;
    }
    
    // count length of number and allocate str
    int num_len = 0;
    int temp = num;
    while (temp > 0) {
        num_len++;
        temp /= 10;
    }
    str = (char*)malloc((num_len + 1) * sizeof(char));

    // convert integer to character in reverse order
    str[num_len] = '\0';
    for (int i = num_len - 1; i >= 0; i--) {
        str[i] = '0' + (num % 10);
        num /= 10;
    }
    
    return str;
}

int is_identical(char *str1, char *str2) {
    int str1_len = strlength(str1);
    if (str1_len != strlength(str2)) {
        return 0;
    }
    for (int i = 0; i < str1_len; i++) {
        if (str1[i] != str2[i]) {
            return 0;
        }
    }
    return 1;
}

int is_matching(char *str1, char *str2) {
    int str1_len = strlength(str1);
    if (str1_len != strlength(str2)) {
        return 0;
    }
    // change all characters to lowercase)
    for (int i = 0; i < str1_len; i++) {
        char char1 = str1[i];
        char char2 = str2[i];
        if (str1[i] >= 'A' && str1[i] <= 'Z') {
            char1 = str1[i] + 32;
        }
        if (str2[i] >= 'A' && str2[i] <= 'Z') {
            char2 = str2[i] + 32;
        }

        if (char1 != char2) {
            return 0;
        }
    }
    return 1;
}

int is_matching_char(char char1, char char2) {
        if (char1 >= 'A' && char1 <= 'Z') {
            char1 += 32;
        }
        if (char2 >= 'A' && char2 <= 'Z') {
            char2 += 32;
        }
        if (char1 == char2) return 1;
        else return 0;
}