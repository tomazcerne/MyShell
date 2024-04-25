#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

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
        fflush(stdout);
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
        fflush(stdout);
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
        fflush(stdout);
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
    fflush(stdout);
    return 0;
}
int f_print(int arg_count) {
    for (int i = 1; i < arg_count; i++) {
        if (i > 1) printf(" ");
        printf("%s", tokens[i]);
    }
    fflush(stdout);
    return 0;
}
int f_echo(int arg_count) {
    f_print(arg_count);
    printf("\n");
    fflush(stdout);
    return 0;
}
int f_len(int arg_count) {
    int len = 0;
    for (int i = 1; i < arg_count; i++) {
        len += strlen(tokens[i]);
    }
    printf("%d\n", len);
    fflush(stdout);
    return 0;
}
int f_sum(int arg_count) {
    int sum = 0;
    for (int i = 1; i < arg_count; i++) {
        sum += atoi(tokens[i]);
    }
    printf("%d\n", sum);
    fflush(stdout);
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
    fflush(stdout);
    return 0;
}
int f_basename(int arg_count) {
    if (arg_count != 2) return 1;
    char* last_slash = strrchr(tokens[1], '/');
    if (last_slash == NULL) {
        printf("%s\n", tokens[1]);
    }
    else if (strcmp(tokens[1], "/") == 0){
        printf("/\n");
    }
    else {
        printf("%s\n", last_slash+1);
    }
    fflush(stdout);
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
    fflush(stdout);
    return 0;
}
int f_dirch(int arg_count) {
    if (arg_count > 2) return 1;
    char* path = (arg_count == 1) ? "/" : tokens[1];
    if (chdir(path) != 0) {
        int err = errno;
        perror("dirch");
        fflush(stderr);
        return err;
    }
    return 0;
}
int f_dirwd(int arg_count) {
    if (arg_count > 2) return 1;
    char* mode = (arg_count == 1) ? "base" : tokens[1];
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        int err = errno;
        perror("dirwd");
        fflush(stderr);
        return err;
    }
    if (strcmp(mode, "base") == 0) {
        tokens[1] = cwd;
        return f_basename(2);
    }
    else if (strcmp(mode, "full") == 0) {
        printf("%s\n", cwd);
        fflush(stdout);
        return 0;
    }   
    return 2;
}
int f_dirmk(int arg_count) {
    if (arg_count != 2) return 1;
    if (mkdir(tokens[1], 0777) != 0) {
        int err = errno;
        perror("dirmk");
        fflush(stderr);
        return err;
    }
    return 0;
}
int f_dirrm(int arg_count) {
    if (arg_count != 2) return 1;
    if (rmdir(tokens[1]) != 0) {
        int err = errno;
        perror("dirrm");
        fflush(stderr);
        return err;
    }
    return 0;
}
int f_dirls(int arg_count) {
    if (arg_count > 2) return 1;
    char* dirname = (arg_count == 1) ? "." : tokens[1];
    DIR *dir;
    struct dirent *entry;
    dir = opendir(dirname);
    if (dir == NULL) {
        int err = errno;
        perror("dirls");
        fflush(stderr);
        return err;
    }
    int f = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (f) printf("  ");
        f = 1;
        printf("%s", entry->d_name);
    }
    printf("\n");
    fflush(stdout);
    closedir(dir);
    return 0;
}
int f_rename(int arg_count) {
    if (arg_count != 3) return 1;
    if (rename(tokens[1], tokens[2]) != 0) {
        int err = errno;
        perror("rename");
        fflush(stderr);
        return err;
    }
    return 0;
}
int f_unlink(int arg_count) {
    if (arg_count != 2) return 1;
    if (unlink(tokens[1]) != 0) {
        int err = errno;
        perror("unlink");
        fflush(stderr);
        return err;
    }
    return 0;
}
int f_remove(int arg_count) {
    if (arg_count != 2) return 1;
    if (remove(tokens[1]) != 0) {
        int err = errno;
        perror("remove");
        fflush(stderr);
        return err;
    }
    return 0;
}
int f_linkhard(int arg_count) {
    if (arg_count != 3) return 1;
    if (link(tokens[1], tokens[2]) != 0) {
        int err = errno;
        perror("linkhard");
        fflush(stderr);
        return err;
    }
    return 0;
}
int f_linksoft(int arg_count) {
    if (arg_count != 3) return 1;
    if (symlink(tokens[1], tokens[2]) != 0) {
        int err = errno;
        perror("linksoft");
        fflush(stderr);
        return err;
    }
    return 0;
}
int f_linkread(int arg_count) {
    if (arg_count != 2) return 1;
    char target[1024];
    int len = readlink(tokens[1], target, sizeof(target)-1);
    if (len == -1) {
        int err = errno;
        perror("linkread");
        fflush(stderr);
        return err;
    }
    target[len] = '\0';
    printf("%s\n", target);
    fflush(stdout);
    return 0;
}
int f_linklist(int arg_count) {
    if (arg_count != 2) return 1;
    DIR *dir;
    struct dirent *entry;
    dir = opendir(".");
    if (dir == NULL) {
        int err = errno;
        perror("linklist");
        fflush(stderr);
        return err;
    }
    int inode = -1;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(tokens[1], entry->d_name) == 0) {
            inode = entry->d_ino;
            break;
        }
    }
    if (inode == -1) return 2;
    closedir(dir);

    dir = opendir(".");
    if (dir == NULL) {
        int err = errno;
        perror("linklist");
        fflush(stderr);
        return err;
    }
    int f = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (inode == entry->d_ino) {
            if (f) printf("  ");
            f = 1;
            printf("%s", entry->d_name);
        }  
    }
    closedir(dir);
    printf("\n");
    fflush(stdout);
    return 0;
}
int f_cpcat(int arg_count) {
    if (arg_count > 3) return 1;
    int fd_in = (arg_count < 2 || tokens[1][0] == '-') ? 0 :
        open(tokens[1], O_RDONLY);
    if (fd_in < 0) {
        int err = errno;
        perror("cpcat");
        fflush(stderr);
        return err;
    }
    int fd_out = (arg_count < 3) ? 1 : open(tokens[2], O_CREAT | O_WRONLY | O_TRUNC, 0777);
    if (fd_out < 0) {
        int err = errno;
        perror("cpcat");
        fflush(stderr);
        return err;
    }

    char buffer[1024];
    int size = 1;
    while (size > 0) {
        size = read(fd_in, buffer, sizeof(buffer));
        if (size < 0) {
            int err = errno;
            perror("cpcat");
            fflush(stderr);
            return err;
        }
        int wr = write(fd_out, buffer, size);
        if (wr < 0) {
            int err = errno;
            perror("cpcat");
            fflush(stderr);
            return err;
        }
    }
    if (fd_in > 0) close(fd_in);
    if (fd_out > 1) close(fd_out);
    return 0;
}
//********************************************************************

#define BUILTIN_COUNT 25
char* builtin_cmd_names[] = {"debug", "prompt", "status", "exit", "help", 
"print", "echo", "len", "sum", "calc", "basename", "dirname",
"dirch", "dirwd", "dirmk", "dirrm", "dirls",
"rename", "unlink", "remove", "linkhard", "linksoft", "linkread", "linklist", "cpcat"};
int (*builtin_functions[])(int) = { f_debug, f_prompt, f_status, f_exit, f_help,
f_print, f_echo, f_len, f_sum, f_calc, f_basename, f_dirname, 
f_dirch, f_dirwd, f_dirmk, f_dirrm, f_dirls, 
f_rename, f_unlink, f_remove, f_linkhard, f_linksoft, f_linkread, f_linklist, f_cpcat};

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
    fflush(stdout);
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
        fflush(stdout);
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
    fflush(stdout);
    return 0;
}

int main () {
    strcpy(prompt, "mysh");
    int iact = (isatty(STDIN_FILENO)) ? 1 : 0;
    if(iact) {
        printf("%s%s> %s", GREEN_START, prompt, GREEN_END);
        fflush(stdout);
    }
    while (fgets(line, LINE_SIZE, stdin) != NULL ) {
        int len = strlen(line);
        if (line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        if (debug_level > 0) {
            printf("Input line: '%s'\n", line);
            fflush(stdout);
        }
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
            fflush(stdout);
        }
        globals_reset();
    }
    if(iact) {
        printf("\n");
        fflush(stdout);
    }
    return status;
}