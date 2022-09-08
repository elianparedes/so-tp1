#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <stdlib.h>
#include <wordexp.h>

#define LOAD 1
#define SLAVES 5
#define SHM_NAME "/buffer"
#define COUNT 256

int main(int argc, char const *argv[])
{
    wordexp_t p;
    char **w;
    int i;

    wordexp("files/*", &p, 0);
    w = p.we_wordv;

    int pipes[5][2];
    int fd= shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd== -1){
        perror("app | shm_open parent: ");
    }
    for (int i=0; i < SLAVES; i++){
        if (fork()==0){
            pipe(pipes[i]);
            close(0);
            write(pipes[i][1], w[i], 256);
            dup2(pipes[i][0], 0);
            execl("./slave", "./slave", (char *) NULL);
        }
    }

    wordfree(&p);
    return 0;
}
