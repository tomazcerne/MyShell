#include <stdio.h>
#include <stdlib.h>

int main (int argc, char* argv[]) {
    printf("Hello world\n");
    for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }
    exit(1);
    return 0;
}