#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#define LINE_SIZE 1024
#define MAX_TOKENS 32
#define SHELL_NAME "mysh> "

char line[LINE_SIZE];
char* tokens[MAX_TOKENS];

int tokenize(char* l) {
    char* current = NULL;
    int tkns = 0;
    int ignore_space = 0;
    while (*l != '\0') {
        if (*l == '#' && !ignore_space) {
            *l = '\0';
            break;
        }
        if (current == NULL) {
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

void print_tokens(int tkns) {
    for (int i = 0; i < tkns; i++) {
        printf("Token %d: '%s'\n", i, tokens[i]);
    }
}

int ignore (char *l) {
    while (*l != '\0') {
        if (!isspace(*l)) {
            return 0;
        }
        l++;
    }
    return 1;
}

int main () {
    int iact = (isatty(STDIN_FILENO)) ? 1 : 0;
    if(iact) {
        printf("%s", SHELL_NAME);
    }
    while (fgets(line, LINE_SIZE, stdin) != NULL ) {
        if (!ignore(line)) {
            int len = strlen(line);
            if (line[len-1] == '\n') {
                line[len-1] = '\0';
            }
            printf("Input line: '%s'\n", line);
            print_tokens(tokenize(line));
        }
        if(iact) {
            printf("%s", SHELL_NAME);
        }
    }
    if(iact) {
        printf("\n");
    }
    return 0;
}