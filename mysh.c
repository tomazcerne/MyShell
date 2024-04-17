#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#define GREEN_START "\033[1;32m"
#define GREEN_END "\033[0m"
char prompt[9];

int debug_level = 0;
int status = 0;
#define ESCAPE_STATUS -1

#define LINE_SIZE 1024
#define MAX_TOKENS 32
char line[LINE_SIZE];
char* tokens[MAX_TOKENS];
char* input_redirect = NULL;
char* output_redirect = NULL;
int background = 0;

// built-in functions :
//********************************************************************
int f_debug(int arg_count) {
    if (arg_count == 1) {
        printf("%d\n", debug_level);
        return 0;
    }
    if (arg_count == 2) {
        debug_level = atoi(tokens[1]);
        return 0;
    }
    return 1;
}
int f_prompt(int arg_count) {
    if (arg_count == 1) {
        printf("%s\n", prompt);
        return 0;
    }
    if (arg_count == 2) {
        if (strlen(tokens[1]) > 8) {
            return 1;
        }
        strcpy(prompt, tokens[1]);
        return 0;
    }
    return 2;
}
int f_status(int arg_count) {
    if (arg_count == 1) {
        printf("%d\n", status);
        return ESCAPE_STATUS;
    }
    return 1;
}
int f_exit(int arg_count) {
    if (arg_count == 1) {
        exit(status);
    }
    if (arg_count == 2) {
        exit(atoi(tokens[1]));
    }
    return 1;
}
int f_help(int arg_count) {
    printf("Supported Built-in Commands:\n");
    printf("> debug [level]\n");
    printf("   changes the debug_level (default: 0) to the value specified by the argument 'level'\n");
    printf("   if no argument is provided, it prints the current debug_level\n");
    printf("> prompt [name]\n");
    printf("   changes the shell prompt (default: 'mysh') to the value specified by the argument 'name'\n");
    printf("   the name is limited to 8 characters\n");
    printf("   if no argument is provided, it prints the current prompt\n");
    printf("> status\n");
    printf("   prints the status returned from the previously executed command\n");
    printf("> exit [status]\n");
    printf("   exits the shell and returns the value specified by the argument 'status'\n");
    printf("   if no argument is provided, it returns the status from the previously executed command\n");
    printf("> help\n");
    printf("   prints a list of all built-in commands along with their explanations\n");
    return 0;
}
int f_print(int arg_count) {
    for (int i = 1; i < arg_count; i++) {
        if (i > 1) printf(" ");
        printf("%s", tokens[i]);
    }
    return 0;
}
int f_echo(int arg_count) {
    f_print(arg_count);
    printf("\n");
    return 0;
}
int f_len(int arg_count) {
    int len = 0;
    for (int i = 1; i < arg_count; i++) {
        len += strlen(tokens[i]);
    }
    printf("%d\n", len);
    return 0;
}
int f_sum(int arg_count) {
    int sum = 0;
    for (int i = 1; i < arg_count; i++) {
        sum += atoi(tokens[i]);
    }
    printf("%d\n", sum);
    return 0;
}
int f_calc(int arg_count) {
    if (arg_count != 4) return 1;
    char* arg1 = tokens[1];
    char* op = tokens[2];
    char* arg2 = tokens[3];
    if (strcmp(op, "+") == 0) {
        printf("%d\n", atoi(arg1) + atoi(arg2));
    }
    else if (strcmp(op, "-") == 0) {
        printf("%d\n", atoi(arg1) - atoi(arg2));
    }
    else if (strcmp(op, "*") == 0) {
        printf("%d\n", atoi(arg1) * atoi(arg2));
    }
    else if (strcmp(op, "/") == 0) {
        printf("%d\n", atoi(arg1) / atoi(arg2));
    }
    else if (strcmp(op, "%") == 0) {
        printf("%d\n", atoi(arg1) % atoi(arg2));
    }
    else {
        return 2;
    }
    return 0;
}
int f_basename(int arg_count) {
    if (arg_count != 2) return 1;
    char* last_slash = strrchr(tokens[1], '/');
    if (last_slash == NULL) {
        printf("%s\n", tokens[1]);
    }
    else {
        printf("%s\n", last_slash+1);
    }
    return 0;
}
int f_dirname(int arg_count) {
    if (arg_count != 2) return 1;
    char* last_slash = strrchr(tokens[1], '/');
    if (last_slash == NULL) {
        printf(".\n");
    }
    else if (last_slash == tokens[1]){
        printf("/\n");
    }
    else {
        *last_slash = '\0';
        printf("%s\n", tokens[1]);
    }
    return 0;
}
//********************************************************************

#define BUILTIN_COUNT 12
char* builtin_cmd_names[] = {"debug", "prompt", "status", "exit", "help", 
"print", "echo", "len", "sum", "calc", "basename", "dirname"};
int (*builtin_functions[])(int) = { f_debug, f_prompt, f_status, f_exit, f_help,
f_print, f_echo, f_len, f_sum, f_calc, f_basename, f_dirname };

void globals_reset() {
    input_redirect = NULL;
    output_redirect = NULL;
    background = 0;
}

int tokenize(char* l) {
    char* current = NULL;
    int tkns = 0;
    int ignore_space = 0;
    while (*l != '\0') { 
        if (current == NULL) {
            if (*l == '#' && !ignore_space) {
                *l = '\0';
                break;
            }
            if (!ignore_space && *l == '"') {
                ignore_space = 1;
            }
            else if (!isspace(*l) || ignore_space) {
                current = l;
            }
        }
        else {
            if (!ignore_space && isspace(*l) || ignore_space && *l == '"') {
                *l = '\0';
                tokens[tkns++] = current;
                current = NULL;
                if (ignore_space) ignore_space = 0;
            }
        }
        l++;
    }
    if (current != NULL) {
        tokens[tkns++] = current;
    }
    return tkns;
}

void debug_print(int tkns) {
    for (int i = 0; i < tkns; i++) {
        printf("Token %d: '%s'\n", i, tokens[i]);
    }
    if (input_redirect != NULL) {
        printf("Input redirect: '%s'\n", input_redirect);
    }
    if (output_redirect != NULL) {
        printf("Output redirect: '%s'\n", output_redirect);
    }
    if (background) {
        printf("Background: 1\n");
    }
}

int parse_tokens(int tokens_count) {
    if (tokens_count > 1 && tokens[tokens_count - 1][0] == '&') {
        background = 1;
        tokens_count--;
    }
    if (tokens_count > 1 && tokens[tokens_count - 1][0] == '>') {
        output_redirect = &tokens[tokens_count - 1][1];
        tokens_count--;
    }
    if (tokens_count > 1 && tokens[tokens_count - 1][0] == '<') {
        input_redirect = &tokens[tokens_count - 1][1];
        tokens_count--;
    }
    return tokens_count;
}

int find_builtin(char* cmd) {
    for (int i = 0; i < BUILTIN_COUNT; i++) {
        if (strcmp(cmd, builtin_cmd_names[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int execute_builtin(int index, int arg_count) {
    if (debug_level > 0) {
        printf("Executing builtin '%s' in foreground\n", tokens[0]);
    }
    return builtin_functions[index](arg_count);
}

int execute_external(int arg_count) {
    printf("External command '");
    for (int i = 0; i < arg_count; i++) {
        if (i>0) printf(" ");
        printf("%s", tokens[i]);
    }
    printf("'\n");
    return 0;
}

int main () {
    strcpy(prompt, "mysh");
    int iact = (isatty(STDIN_FILENO)) ? 1 : 0;
    if(iact) {
        printf("%s%s> %s", GREEN_START, prompt, GREEN_END);
    }
    while (fgets(line, LINE_SIZE, stdin) != NULL ) {
        int len = strlen(line);
        if (line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        if (debug_level > 0) printf("Input line: '%s'\n", line);
        int tokens_count = tokenize(line);
        if (tokens_count > 0) {
            int arg_count = parse_tokens(tokens_count);
            if (debug_level > 0) debug_print(tokens_count);
            int builtin_index = find_builtin(tokens[0]);
            int s;
            if (builtin_index >= 0) {
                s = execute_builtin(builtin_index, arg_count);
            }
            else {
                s = execute_external(arg_count);
            }
            status = (s == ESCAPE_STATUS) ? status : s;
            if (s > 0) {
                //printf("napaka: %d\n", status);
            }
        }
        if(iact) {
            printf("%s%s> %s", GREEN_START, prompt, GREEN_END);
        }
        globals_reset();
    }
    if(iact) {
        printf("\n");
    }
    return status;
}