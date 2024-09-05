#ifndef PARSER
#define PARSER

#define MAX_TOKENS 200
#define MAX_COMMANDS 100
#define MAX_ARGUMENTS 100

typedef enum {
    TOKEN_COMMAND,
    TOKEN_ARGUMENT,
    TOKEN_AND,             // &
    TOKEN_PIPE,            // |
    TOKEN_REDIRECT_IN,     // <
    TOKEN_REDIRECT_OUT,    // >
    TOKEN_REDIRECT_APPEND, // >>
    TOKEN_END,
} TokenType;

typedef struct token {
    TokenType type;
    char *data;
} Token;

typedef enum {
    REDIR_IN,     // <
    REDIR_OUT,    // >
    REDIR_APPEND, // >>
} RedirectionType;

typedef struct {
    RedirectionType type;
    char *pathname;
} Redirection;

typedef enum {
    RUNNING,
    STOPPED,
    TERMINATED,
} JobState;

typedef struct {
    char *command; // argv[0] : executables, builtin, PATH ...
    char *args[MAX_ARGUMENTS];
    int arg_count;
    Redirection *redirection_in;  // default: NULL
    Redirection *redirection_out; // default: NULL
    int pid;
} Command; // Process

typedef struct {
    Command commands[MAX_COMMANDS];
    int is_foreground; // 1: foreground, 0: background
    int command_count;
    int pgid;
    char *input;
    JobState state;
} Job;

int is_special_char(char c);
int is_pipe(Token token);
void lex(char *cmd, Token *tokens);
void free_tokens(Token *tokens);
void parse(Token *tokens, Job *job);

#endif