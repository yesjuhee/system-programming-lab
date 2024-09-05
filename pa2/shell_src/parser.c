#include "parser.h"
#include <fcntl.h>
#include <limits.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int is_special_char(char c) {
    return c == '<' || c == '>' || c == '|' || c == '\'' || c == '"' ||
           c == '\0' || isspace(c) || c == '&';
}

int is_pipe(Token token) {
    return token.type == TOKEN_PIPE;
}

void lex(char *cmd, Token *tokens) {
    int token_i = 0;

    for (const char *curr_char = cmd; *curr_char != '\0'; curr_char++) {
        if (isspace(*curr_char))
            continue;

        switch (*curr_char) {
        case '|':
            tokens[token_i++] = (Token){ .type = TOKEN_PIPE, .data = "|" };
            break;
        case '<':
            tokens[token_i++] = (Token){ .type = TOKEN_REDIRECT_IN, .data = "<" };
            break;
        case '>':
            if (*(curr_char + 1) == '>') {
                tokens[token_i++] =
                    (Token){ .type = TOKEN_REDIRECT_APPEND, .data = ">>" };
                curr_char++;
            } else {
                tokens[token_i++] = (Token){ .type = TOKEN_REDIRECT_OUT, .data = ">" };
            }
            break;
        case '&':
            tokens[token_i++] = (Token){ .type = TOKEN_AND, .data = "&" };
            break;
        default: {
            const char *command_start = curr_char;

            TokenType type = (is_pipe(tokens[token_i - 1]) || token_i == 0)
                                 ? TOKEN_COMMAND
                                 : TOKEN_ARGUMENT;

            while (!is_special_char(*curr_char)) {
                curr_char++;
            }

            tokens[token_i++] =
                (Token){ .type = type,
                         .data = strndup(command_start, curr_char - command_start) };
            curr_char--;
            break;
        }
        }
    }

    tokens[token_i] = (Token){ .type = TOKEN_END, .data = NULL };
}

void free_tokens(Token *tokens) {
    for (int i = 0; tokens[i].type != TOKEN_END; i++) {
        if (tokens[i].type == TOKEN_COMMAND || tokens[i].type == TOKEN_ARGUMENT) {
            free(tokens[i].data);
        }
    }
}

void parse(Token *tokens, Job *job) {
    int command_i = 0;
    int arg_i = 0;
    job->is_foreground = 1;
    job->command_count = 1;
    job->pgid = 0;
    for (int i = 0; tokens[i].type != TOKEN_END; i++) {
        switch (tokens[i].type) {
        case TOKEN_PIPE:
            command_i++;
            job->command_count++;
            arg_i = 0;
            break;
        case TOKEN_COMMAND:
            job->commands[command_i].command = tokens[i].data;
            break;
        case TOKEN_ARGUMENT:
            job->commands[command_i].args[arg_i] = tokens[i].data;
            job->commands[command_i].arg_count++;
            arg_i++;
            break;
        case TOKEN_REDIRECT_IN: {
            Redirection *redirection = malloc(sizeof(Redirection));
            *redirection = (Redirection){ .type = REDIR_IN, .pathname = tokens[++i].data };
            job->commands[command_i].redirection_in = redirection;
            break;
        }
        case TOKEN_REDIRECT_OUT: {
            Redirection *redirection = malloc(sizeof(Redirection));
            *redirection = (Redirection){ .type = REDIR_OUT, .pathname = tokens[++i].data };
            job->commands[command_i].redirection_out = redirection;
            break;
        }
        case TOKEN_REDIRECT_APPEND: {
            Redirection *redirection = malloc(sizeof(Redirection));
            *redirection = (Redirection){ .type = REDIR_APPEND, .pathname = tokens[++i].data };
            job->commands[command_i].redirection_out = redirection;
            break;
        }
        case TOKEN_AND:
            if (tokens[++i].type == TOKEN_END) {
                job->is_foreground = 0;
            }
            i--;
            break;
        case TOKEN_END:
        }
    }
}