#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "my_stdio.h"
#include "my_string.h"

/* 
 * search target word in a line from start_idx and return location index
 * return -1 when it can't find the target
*/
int search_word(char* line, int start_idx, char* target) {
    int line_len = strlength(line);
    
    char* extract_word = (char*) malloc((line_len + 1) * sizeof(char));   // maximum size of a phrase
    if (!extract_word) {
        print_error("malloc error in search_word()");
        return -1;
    }

    for (int i = start_idx; i < line_len;) {
        // Skip leading spaces/tabs to find the start of the next word
        while ((line[i] == ' ' || line[i] == '\t') && line[i] != '\0') {
            i++;
        }
        if (line[i] == '\0') break;

        int word_idx = i;   // Start index of the word
        int j = 0;          // Index for extract_word
        while (line[i] != ' ' && line[i] != '\t' && line[i] != '\0') {
            extract_word[j++] = line[i++];
        }
        extract_word[j] = '\0';

        // 추출한 단어와 타겟 단어 비교 (case insensitive)
        if (is_matching(target, extract_word)) {
            free(extract_word);
            return word_idx;
        }
    }
    
    // 일치하는 단어 없음
    free(extract_word);
    return -1;
}

/* 
 * search target pharase in a line from start_idx and return location index
 * return -1 when it can't find the target
*/
int search_phrase(char* line, int start_idx, char* target) {
    int line_len = strlength(line);
    int target_len = strlength(target);
    
    for (int line_idx = start_idx; line_idx <= line_len; line_idx++) {
        int match_found = 1; // Assume a match is found, prove otherwise
        for (int target_idx = 0; target_idx < target_len; target_idx++) {
            if (!is_matching_char(target[target_idx], line[line_idx + target_idx])) {
                match_found = 0; // Mismatch found
                break;
            }
        }
        if (match_found 
            && (line[line_idx + target_len] == ' ' || line[line_idx + target_len] == '\t' || line[line_idx + target_len] == '\0')
            && (line_idx == 0 || (line[line_idx - 1] == ' ' || line[line_idx - 1] == '\t'))) {
            return line_idx; // Successful match found
        }
    }
    return -1; // Target phrase not found
}