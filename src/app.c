// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#define _DEFAULT_SOURCE
#include "app.h"

#include <fcntl.h> /* For O_* constants */
#include <math.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h> /* For mode constants */
#include <unistd.h>

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "%s", "app | missing arguments\n");
    }

    int slaves = SLAVES * LOAD_FACTOR > argc - 1
                     ? ceil((double)(argc - 1) / (double)LOAD_FACTOR)
                     : SLAVES;

    int master_to_slave[slaves][2];
    int slave_to_master_stdout[slaves][2];
    int slave_to_master_stderr[slaves][2];

    int files_in_hash = 1;
    int files_in_shm = 0;

    fd_set read_set_fds;

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);

    sem_t *s1 = sem_open(SEM_NAME, O_CREAT, 0660, 0);
    if (s1 == SEM_FAILED) {
        perror("app | sem_open");
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&read_set_fds);

    for (int i = 0; i < slaves; i++) {
        pipe(master_to_slave[i]);
        pipe(slave_to_master_stdout[i]);
        pipe(slave_to_master_stderr[i]);
    }

    int fd_shm = open_shm(SHM_NAME, SHM_SIZE);

    sleep(2);
    printf(SHM_NAME);
    fflush(stdout);

    char initial_load[BUFFER * slaves + slaves];

    for (int i = 0; i < slaves; i++) {
        strncpy(initial_load, argv[files_in_hash++], BUFFER - 1);

        for (size_t j = 1; j < LOAD_FACTOR && files_in_hash < argc; j++) {
            strcat(initial_load, " ");
            strncat(initial_load, argv[files_in_hash++], BUFFER - 1);
        }

        write(master_to_slave[i][WRITE_END], initial_load,
              strlen(initial_load) + 1);

        if (fork() == 0) {
            close(STDIN_FILENO);
            dup2(master_to_slave[i][READ_END], STDIN_FILENO);

            close(STDOUT_FILENO);
            dup2(slave_to_master_stdout[i][WRITE_END], STDOUT_FILENO);

            close(master_to_slave[i][WRITE_END]);
            close(slave_to_master_stdout[i][READ_END]);

            execl(SLAVE_NAME, SLAVE_NAME, (char *)NULL);
        }
    }

    size_t entries;

    if (slaves < SLAVES) {
        entries = slaves;
    } else {
        if (argc - 1 < slaves * LOAD_FACTOR) {
            entries = slaves + 1;
        } else {
            entries = argc - slaves - 1;
        }
    }

    while (files_in_shm < entries) {
        for (int i = 0; i < slaves; i++) {
            FD_SET(slave_to_master_stdout[i][READ_END], &read_set_fds);
        }

        if (select(FD_SETSIZE, &read_set_fds, NULL, NULL, NULL) < 0) {
            perror("app | select");
            _exit(EXIT_FAILURE);
        }

        for (int i = 0; i < slaves; i++) {
            if (FD_ISSET(slave_to_master_stdout[i][READ_END], &read_set_fds)) {
                char hash[BUFFER];
                int numBytes;

                if ((numBytes = read(slave_to_master_stdout[i][READ_END], hash,
                                     BUFFER)) != -1) {
                    write(fd_shm, hash, numBytes);
                    files_in_shm++;
                    sem_post(s1);
                }

                if (files_in_hash < argc) {
                    char input_buffer[BUFFER];

                    strncpy(input_buffer, argv[files_in_hash++], BUFFER - 1);

                    if (write(master_to_slave[i][WRITE_END], input_buffer,
                              strlen(input_buffer) + 1) == ERROR_CODE) {
                        perror("app | write");
                    }
                }
            }
        }
    }

    sem_close(s1);

    return 0;
}

int open_shm(char *name, off_t length) {
    int fd_shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

    if (fd_shm == ERROR_CODE) {
        perror("app | shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(fd_shm, length) == ERROR_CODE) {
        perror("app | ftruncate");
        exit(EXIT_FAILURE);
    }

    void *addr_parent =
        mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if (addr_parent == MAP_FAILED) {
        perror("app | mmap");
        exit(EXIT_FAILURE);
    }

    return fd_shm;
}
