#define _DEFAULT_SOURCE

#include <fcntl.h> /* For O_* constants */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h> /* For mode constants */
#include <unistd.h>
#include <signal.h>

#define SLAVES      5
#define SHM_NAME    "/buffer"
#define SHM_SIZE    1024
#define BUFFER      512
#define LOAD_FACTOR 2

int main(int argc, char const *argv[]) {

    int ms_pipe[SLAVES][2];
    int sm_pipe[SLAVES][2];
    int files_hashed = 1;
    int files_in_mem = 0;

    fd_set read_set_fds;

    FD_ZERO(&read_set_fds);

    for (int i = 0; i < SLAVES; i++) {
        pipe(sm_pipe[i]);
        pipe(ms_pipe[i]);
    }

    int fd_shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_shm == -1)
        perror("shm | shm_open");

    if (ftruncate(fd_shm, SHM_SIZE) == -1)
        perror("shm | truncate");

    void *addr_parent =
        mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if (addr_parent == MAP_FAILED)
        perror("shm | mmap");

    for (int i = 0; i < SLAVES; i++) {

        char *exec_argv[LOAD_FACTOR + 2] = {NULL};
        exec_argv[0] = "./slave";

        for (size_t j = 1; j <= LOAD_FACTOR; j++) {
            exec_argv[j] = argv[files_hashed++];
        }

        if (fork() == 0) {
            close(0);
            dup2(ms_pipe[i][0], 0);

            close(1);
            dup2(sm_pipe[i][1], 1);

            execv("./slave", exec_argv);
        }
    }

    while (files_in_mem < argc - 1 - SLAVES) {
        for (int i = 0; i < SLAVES; i++) {
            FD_SET(sm_pipe[i][0], &read_set_fds);
        }

        if (select(FD_SETSIZE, &read_set_fds, NULL, NULL, NULL) < 0) {
            perror("shm | select");
            exit(0);
        }

        for (int i = 0; i < SLAVES; i++) {
            if (FD_ISSET(sm_pipe[i][0], &read_set_fds)) {
                char hash[BUFFER];
                int numBytes = read(sm_pipe[i][0], hash, BUFFER);
                hash[numBytes] = '\0';

                write(fd_shm, hash, numBytes);
                files_in_mem++;
                
                if (files_hashed < argc)
                    write(ms_pipe[i][1], argv[files_hashed++], BUFFER);
            }
        }
    }

    return 0;
}
