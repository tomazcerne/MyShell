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
#include <sys/utsname.h>
#include <sys/wait.h>
#include <signal.h>

#define GREEN_START "\033[1;32m"
#define GREEN_END "\033[0m"
char prompt[9];

int debug_level = 0;
int status = 0;
#define ESCAPE_STATUS -1
char procfs_path[128];

#define LINE_SIZE 1024
#define MAX_TOKENS 32
char line[LINE_SIZE];
char subsh_line[LINE_SIZE];
char* tokens[MAX_TOKENS];
char* input_redirect = NULL;
int fd_in;
char* output_redirect = NULL;
int fd_out;
int background = 0;

#define MAX_VAR 1024
char* var_names[MAX_VAR];
char* var_values[MAX_VAR];
int var_count = 0;

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
int f_pid(int arg_count) {
    if (arg_count != 1) return 1;
    printf("%d\n", getpid());
    fflush(stdout);
    return 0;
}
int f_ppid(int arg_count) {
    if (arg_count != 1) return 1;
    printf("%d\n", getppid());
    fflush(stdout);
    return 0;
}
int f_uid(int arg_count) {
    if (arg_count != 1) return 1;
    printf("%d\n", getuid());
    fflush(stdout);
    return 0;
}
int f_euid(int arg_count) {
    if (arg_count != 1) return 1;
    printf("%d\n", geteuid());
    fflush(stdout);
    return 0;
}
int f_gid(int arg_count) {
    if (arg_count != 1) return 1;
    printf("%d\n", getgid());
    fflush(stdout);
    return 0;
}
int f_egid(int arg_count) {
    if (arg_count != 1) return 1;
    printf("%d\n", getegid());
    fflush(stdout);
    return 0;
}
int f_sysinfo(int arg_count) {
    if (arg_count != 1) return 1;
    struct utsname sys_info;
    uname(&sys_info);
    printf("Sysname: %s\n", sys_info.sysname);
    printf("Nodename: %s\n", sys_info.nodename);
    printf("Release: %s\n", sys_info.release);
    printf("Version: %s\n", sys_info.version);
    printf("Machine: %s\n", sys_info.machine);
    fflush(stdout);
    return 0;
}
int f_proc(int arg_count) {
    if (arg_count > 2) return 2;
    if (arg_count == 1) {
        printf("%s\n", procfs_path);
        fflush(stdout);
        return 0;
    }
    if (access(tokens[1], F_OK | R_OK) != 0) {
        return 1;
    }
    strcpy(procfs_path, tokens[1]);
    return 0;
}
int cmp(const void* a, const void* b) {
    return *(int*)a - *(int*)b;
}
int* getpids(int* cnt) {
    int count = 0;
    int* pid_list = NULL;
    DIR *dir;
    struct dirent *entry;
    dir = opendir(procfs_path);
    if (dir == NULL) {
        return NULL;
    }
    while ((entry = readdir(dir)) != NULL) {
        int pid = atoi(entry->d_name);
        if (pid > 0) {
            count++;
            pid_list = realloc(pid_list, count * sizeof(int));
            pid_list[count - 1] = pid;
        }
    }
    closedir(dir);
    qsort(pid_list, count, sizeof(int), cmp);
    *cnt = count;
    return pid_list;
}
int f_pids(int arg_count) {
    if (arg_count != 1) return 1;
    int count = 0;
    int* pidlist = getpids(&count);
    if (pidlist == NULL) {
        return 2;
    }
    for (int i = 0; i < count; i++) {
        printf("%d\n", pidlist[i]);
    }
    fflush(stdout);
    free(pidlist);
    return 0;
}
int f_pinfo(int arg_count) {
    if (arg_count != 1) return 1;
    int count = 0;
    int* pidlist = getpids(&count);
    if (pidlist == NULL) {
        return 2;
    }
    printf("%5s %5s %6s %s\n", "PID", "PPID", "STANJE", "IME");
    for (int i = 0; i < count; i++) {
        char path[1024];
        sprintf(path, "%s/%d/stat", procfs_path, pidlist[i]);
        FILE* file = fopen(path, "r");
        if (file == NULL) {
            int err = errno;
            perror("pinfo");
            fflush(stderr);
            return err;
        }
        char pid[64];
        char ppid[64];
        char state[64];
        char name[1024];
        fscanf(file, "%s", pid);
        char ch;
        int in = 0, cnt = 0;
        while ((ch = fgetc(file)) != EOF) {
            if (ch == '(') {
                in++;
                if (in == 1) {
                    continue;
                }  
            }
            if (in) {
                name[cnt++] = ch;
            }
            if (ch == ')') {
                in--;
                if (in == 0) {
                    name[--cnt] = '\0';
                    break;
                }
            }
        }
        fscanf(file, "%s %s", state, ppid);
        fclose(file);
        printf("%5s %5s %6s %s\n", pid, ppid, state, name);
    }
    fflush(stdout);
    free(pidlist);
    return 0;
}
int f_waitone(int arg_count) {
    if (arg_count > 2) return 1;
    int pid = -1;
    if (arg_count == 2) {
        pid = atoi(tokens[1]);
    }
    int stat;
    if (waitpid(pid, &stat, 0) > 0) {
        if (WIFEXITED(stat)) {
            return WEXITSTATUS(stat);
        }
        return 2;
    }
    return 0;
}
int f_waitall(int arg_count) {
    if (arg_count != 1) return 1;
    int pid = 1;
    while (pid > 0) {
        pid = waitpid (-1, NULL, 0);
    }
    return 0;
}
void pipe_execute(int* fds_in, int* fds_out, char* line);
int f_pipes(int arg_count) {
    if (arg_count < 3) return 1;
    char line[LINE_SIZE];
    int fds_in[2]; int* in;
    int fds_out[2]; int *out;
    for (int i = 1; i < arg_count; i++) {
        strcpy(line, tokens[i]);
        fds_in[0] = fds_out[0];
        fds_in[1] = fds_out[1];
        if (i + 1 < arg_count) pipe(fds_out);
        in = (i == 1) ? NULL : fds_in;
        out = (i + 1 == arg_count) ? NULL : fds_out;
        pipe_execute(in, out, line);
        if (i > 1) {
            close(fds_in[0]);
            close(fds_in[1]);
        }
    }
    for (int i = 1; i < arg_count; i++) {
        wait(NULL);
    }
    fflush(stdout);
    return 0;
}
int find_var(char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(name, var_names[i]) == 0) {
            return i;
        }
    }
    return -1;
}
int f_set(int arg_count) {
    if (arg_count != 3) return 1;
    int var_index = find_var(tokens[1]);
    if (var_index == -1) {
        var_index = var_count++;
        var_names[var_index] = malloc((strlen(tokens[1])+1) * sizeof(char));
        strcpy(var_names[var_index], tokens[1]);
    }
    free(var_values[var_index]);
    var_values[var_index] = malloc((strlen(tokens[2])+1) * sizeof(char));
    strcpy(var_values[var_index], tokens[2]);
    return 0;
}
int f_get(int arg_count) {
    if (arg_count != 2) return 1;
    int var_index = find_var(tokens[1]);
    if (var_index < 0) {
        fprintf(stderr, "Variable '%s' does not exist\n", tokens[1]);
        fflush(stderr);
        return 2;
    }
    printf("%s\n", var_values[var_index]);
    fflush(stdout);
    return 0;
}
//********************************************************************

#define BUILTIN_COUNT 40
char* builtin_cmd_names[] = {"debug", "prompt", "status", "exit", "help", 
"print", "echo", "len", "sum", "calc", "basename", "dirname",
"dirch", "dirwd", "dirmk", "dirrm", "dirls",
"rename", "unlink", "remove", "linkhard", "linksoft", "linkread", "linklist", "cpcat",
"pid", "ppid", "uid", "euid", "gid", "egid", "sysinfo",
"proc", "pids", "pinfo",
"waitone", "waitall",
"pipes",
"set", "get"};
int (*builtin_functions[])(int) = { f_debug, f_prompt, f_status, f_exit, f_help,
f_print, f_echo, f_len, f_sum, f_calc, f_basename, f_dirname, 
f_dirch, f_dirwd, f_dirmk, f_dirrm, f_dirls, 
f_rename, f_unlink, f_remove, f_linkhard, f_linksoft, f_linkread, f_linklist, f_cpcat,
f_pid, f_ppid, f_uid, f_euid, f_gid, f_egid, f_sysinfo,
f_proc, f_pids, f_pinfo,
f_waitone, f_waitall,
f_pipes,
f_set, f_get};

int tokenize(char* l);
int find_builtin(char* cmd);
void pipe_execute(int* fds_in, int* fds_out, char* line) {
    fflush(stdin); fflush(stdout);
    if (!fork()) {
        if (fds_in) {
            dup2(fds_in[0], 0);
            close(fds_in[0]);
            close(fds_in[1]);
        }
        if (fds_out) {
            dup2(fds_out[1], 1);
            close(fds_out[0]);
            close(fds_out[1]);
        }
        int count = tokenize(line);
        int index = find_builtin(tokens[0]);
        if (index >= 0) {
            exit(builtin_functions[index](count));
        }
        else {
            tokens[count] = NULL;
            execvp(tokens[0], tokens);
            perror("exec");
            fflush(stderr);
            exit(127);
        }
    }
}

void sigchld_handler(int signum) {
    int pid = 1;
    while (pid > 0) {
        pid = waitpid (-1, NULL, WNOHANG);
    }
}

void globals_reset() {
    input_redirect = NULL;
    fd_in = 0;
    output_redirect = NULL;
    fd_out = 1;
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
    if (background) {
        if (debug_level > 0) {
            printf("Executing builtin '%s' in background\n", tokens[0]);
            fflush(stdout);
        }
        fflush(stdin);
        int pid = fork();
        if (pid == 0) {
            fflush(stdin); fflush(stdout);
            if (input_redirect) dup2(fd_in, 0);
            if (output_redirect) dup2(fd_out, 1);
            int stat = builtin_functions[index](arg_count);
            exit(stat);
        }
        else if (pid > 0) {
            return ESCAPE_STATUS;
        }
        else {
            return 1;
        }
    }
    else {
        int std_in, std_out;
        if (input_redirect) {
            std_in = dup(0);
            dup2(fd_in, 0);
        }
        if (output_redirect) {
            std_out = dup(1);
            dup2(fd_out, 1);
        }
        if (debug_level > 0) {
            printf("Executing builtin '%s' in foreground\n", tokens[0]);
            fflush(stdout);
        }
        int stat = builtin_functions[index](arg_count);
        if (input_redirect) dup2(std_in, 0);
        if (output_redirect) dup2(std_out, 1);
        return stat;
    }
}

int execute_external(int arg_count) {
    if (debug_level > 0) {
        char* fgbg = (background) ? "background" : "foreground";
        printf("Executing external '%s' in %s\n", tokens[0], fgbg);
        fflush(stdout);
    }
    fflush(stdin);
    int pid = fork();
    if (pid == 0) {
        fflush(stdin); fflush(stdout);
        if (input_redirect) dup2(fd_in, 0);
        if (output_redirect) dup2(fd_out, 1);
        tokens[arg_count] = NULL;
        execvp(tokens[0], tokens);
        perror("exec");
        fflush(stderr);
        exit(127);
    }
    else if (pid > 0) {
        if (background == 0) {
            int stat;
            waitpid(pid, &stat, 0);
            if (WIFEXITED(stat)) {
                return WEXITSTATUS(stat);
            }
            return 2;
        }
        return ESCAPE_STATUS;
    }
    else {
        return 1;
    }
}

void insert_subsh(char* start, int len, int n) {
    char temp[LINE_SIZE];
    strcpy(temp, (start + len));
    strcpy(start, subsh_line);
    strcpy((start + n), temp);
}

int parse_subsh();
int execute_subsh(char* start, int len) {
    int fdsub[2];
    pipe(fdsub);
    fflush(stdin); fflush(stdout);
    int pid = fork();
    if (pid == 0) {
        dup2(fdsub[1], 1);
        close(fdsub[0]); close(fdsub[1]);
        strcpy(line, subsh_line);
        int stat = parse_subsh();
        if (stat > 0) exit(stat);
        int count = tokenize(line);
        int index = find_builtin(tokens[0]);
        if (index >= 0) {
            int st = builtin_functions[index](count);
            exit((st == ESCAPE_STATUS ? status : st));
        }
        else {
            tokens[count] = NULL;
            execvp(tokens[0], tokens);
            perror("exec");
            fflush(stderr);
            exit(127);
        }
    }
    close(fdsub[1]);
    int n = read(fdsub[0], subsh_line, LINE_SIZE - 1);
    if (n < 0) {
        perror("read");
        fflush(stderr);
        return 1;
    }
    subsh_line[n] = '\0';
    if (n > 0 && subsh_line[n-1] == '\n') subsh_line[--n] = '\0';
    insert_subsh(start, len, n);
    int stat;
    waitpid(pid, &stat, 0);
    if (WIFEXITED(stat)) {
        return WEXITSTATUS(stat);
    }
    return 2;
}
int parse_subsh() {
    char* l = line;
    char* start = NULL;
    char prev = '\0';
    int in = 0, cnt = 0;
    while (*l != '\0') {
        if ((in || prev == '$') && *l == '(') {
            in++;
            if (in == 1) {
                start = l;
                prev = *l;
                l++;
                continue;
            }  
        }
        if (in) {
            subsh_line[cnt++] = *l;
        }
        if (in && *l == ')') {
            in--;
            if (in == 0) {
                subsh_line[--cnt] = '\0';
                int stat = execute_subsh(start-1, cnt+3);
                return (stat > 0) ? stat : parse_subsh();
            }
        }
        prev = *l;
        l++;
    }
    //printf("%d %s\n", cnt, subsh_line);
    return 0;
}
int execute_cmd() {
    int subsh_stat = parse_subsh();
    if (subsh_stat > 0) return subsh_stat;

    int tokens_count = tokenize(line);
    if (tokens_count <= 0) return ESCAPE_STATUS;

    int arg_count = parse_tokens(tokens_count);
    if (debug_level > 0) debug_print(tokens_count);
    int builtin_index = find_builtin(tokens[0]);

    if (input_redirect) {
        fd_in = open(input_redirect, O_RDONLY);
        if (fd_in < 0) {
            perror("opening input file");
            fflush(stderr);
            return ESCAPE_STATUS;
        }
    }
    if (output_redirect) {
        fd_out = open(output_redirect, O_CREAT | O_WRONLY | O_TRUNC, 0777);
        if (fd_out < 0) {
            perror("opening output file");
            fflush(stderr);
            return ESCAPE_STATUS;
        }
    }
    int stat;
    if (builtin_index >= 0) {
        stat = execute_builtin(builtin_index, arg_count);
    }
    else {
        stat = execute_external(arg_count);
    }
    if (input_redirect) close(fd_in);
    if (output_redirect) close(fd_out);
    globals_reset();
    return stat;
}

int main () {
    strcpy(prompt, "mysh");
    strcpy(procfs_path, "/proc");
    signal(SIGCHLD, sigchld_handler);
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
        int s = execute_cmd();
        status = (s == ESCAPE_STATUS) ? status : s;
        if(iact) {
            printf("%s%s> %s", GREEN_START, prompt, GREEN_END);
            fflush(stdout);
        }
    }
    if(iact) {
        printf("\n");
        fflush(stdout);
    }
    return status;
}