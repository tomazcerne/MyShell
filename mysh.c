#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#define LINE_SIZE 1024
#define MAX_TOKENS 32
#define SHELL_NAME "mysh> "

char line[LINE_SIZE];
char* tokens[MAX_TOKENS];
char* args[MAX_TOKENS];
char* command = NULL;
char* input_redirect = NULL;
char* output_redirect = NULL;
int background = 0;

void globals_reset() {
    command = NULL;
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
    command = tokens[0];
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
    for (int i = 1; i < tokens_count; i++) {
        args[i-1] = tokens[i];
    }
    return tokens_count - 1;
}

int main () {
    int iact = (isatty(STDIN_FILENO)) ? 1 : 0;
    if(iact) {
        printf("%s", SHELL_NAME);
    }
    while (fgets(line, LINE_SIZE, stdin) != NULL ) {
        int len = strlen(line);
        if (line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        printf("Input line: '%s'\n", line);
        int tokens_count = tokenize(line);
        if (tokens_count > 0) {
            int arg_count = parse_tokens(tokens_count);
            debug_print(tokens_count);
        }
        if(iact) {
            printf("%s", SHELL_NAME);
        }
        globals_reset();
    }
    if(iact) {
        printf("\n");
    }
    return 0;
}