#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define BUFFER      512
#define LOAD_FACTOR 2

int main(int argc, char *argv[]) {

    if (fork() == 0) {
        argv[0] = "md5sum";
        execv("/bin/md5sum", argv);
    }

    while (1) {
        char path[BUFFER];
        int num_bytes = read(0, path, BUFFER * LOAD_FACTOR);
        path[num_bytes] = '\0';

        if (fork() == 0) {
            execl("/bin/md5sum", "md5sum", path, (char *)NULL);
        }
    }

    return 0;
}

/*
 printf("slave: ");
    for (size_t i = 0; i < argc; i++) {

        printf("%s ", argv[i]);
    }
    printf("\n");

 */