# PA2 Report

---

## 파일 구성

- `make` 하기 전 기본 구성 : `executable_src`와 `shell_src`로 나뉘어져 있다.
- `make` 한 후의 구성 : `/bin` 레포지토리 아래에 바이너리 파일들이 생성되고, `shell_src` 아래에 오브젝트 파일이 추가로 생성된다.

## Executable Source

주어진 조건에 따라 `pa2_cat`, `pa2_cp`, `pa2_head`, `pa2_tail`, `pa2_mv`, `pa2_rm` 여섯가지 기능을 모두 구현하였다. 옵션과 입력 조건들 모두 과제 안내물과 일치한다.

## Shell Source

기능에 따라 크게 3개의 소스 파일로 나뉘어져 있다.

- `parser.c` : `pa2_shell`의 lexer와 parser 역할을 하고 있다. 입력 스트링을 토큰으로 구분하고, `Job` 구조체를 채워서 입력 값을 해석한다.
- `built_in.c` : `pa2_shell`의 built_in 함수들이 작성되어 있다. (`cd`, `pwd`, `exit`, `jobs`)(`bg`와 `fg`는 구현되지 않음.
- `pa2.c` : `pa2_shell`의 메인 함수와, 주요 실행부 코드가 있다.

### parser

```c
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
```

- `Job`: 하나의 Job을 나타내는 구조체
- `Command`: 하나의 Job에서 Pipeline으로 연결된 Process들을 나타내는 구조체

### built in

```c
#ifndef BUILTIN
#define BUILTIN

#include "pa2.h"
#include "parser.h"

void free_job_table(Job *job_table, int job_index);
void built_in_exit(int arg_count, char **args, Job *job_table, int job_index, int free_flag);
void built_in_pwd(Command *process);
void built_in_cd(Command *process);
void built_in_jobs(Job *job_table, int job_index);

#endif
```

### pa2

```c
#ifndef PA2
#define PA2

#include "parser.h"

#define MAX_JOBS 8182
#define PATH_MAX 4096

void init_pa2_shell();
void print_job(Job job);
void launch_job(Job *job_table, int job_index);
void set_redirection(Command *process);
void execute_process(Command *process, Job *job_table, int job_index);

#endif
```

- `init_pa2_shell` : 맨 처음 쉘의 시그널을 설정하는 함수
- `print_job`: 디버깅용 함수. 실제로는 사용되지 않음.
- `launch_job`: `Job` 하나를 실행하는 함수. Job에 포함된 Process들을 파이프라인으로 연결한다. foreground job 일 경우에는 `tcsetpgrep`을 이용하여 터미널의 컨트롤을 넘겨준다. `Job`을 시작하면서 새로운 그룹 프로세스 아이디를 부여하고, 시그널들을 default로 설정한다.
- `set_redirection`: Process를 실행하기 전에 `Command` 구조체의 `Redirection` 정보를 통해서 리다이렉션을 설정한다.
- `execute_process`: Process 하나를 실행하는 함수. `Command`의 명령어를 통해 명령어의 종류를 구분하고, 종류에 맞게 실행한다.
