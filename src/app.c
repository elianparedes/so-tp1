// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
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
        exit(EXIT_FAILURE);
    }

    int slaves = SLAVES * LOAD_FACTOR > argc - 1
                     ? ceil((double)(argc - 1) / (double)LOAD_FACTOR)
                     : SLAVES;

    int master_to_slave[slaves][2];
    int slave_to_master_stdout[slaves][2];
    int slave_to_master_stderr[slaves][2];

    int files_in_hash = 1;
    int files_in_shm = 0;

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);

    sem_t *s1 = sem_open(SEM_NAME, O_CREAT, 0660, 0);
    if (s1 == SEM_FAILED) {
        perror("app | sem_open");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < slaves; i++) {
        pipe(master_to_slave[i]);
        pipe(slave_to_master_stdout[i]);
        pipe(slave_to_master_stderr[i]);
    }

    int fd_shm = open_shm(SHM_NAME, SHM_SIZE);
    if (fd_shm == ERROR_CODE) {
        perror("app | open_shm ");
        exit(EXIT_FAILURE);
    }

    sleep(2);
    printf("%s %d", SHM_NAME, SHM_SIZE);
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

        int ret;
        if ((ret = fork()) == 0) {
            close(STDIN_FILENO);
            dup2(master_to_slave[i][READ_END], STDIN_FILENO);

            close(STDOUT_FILENO);
            dup2(slave_to_master_stdout[i][WRITE_END], STDOUT_FILENO);

            close(STDERR_FILENO);
            dup2(slave_to_master_stderr[i][WRITE_END], STDERR_FILENO);

            close(master_to_slave[i][WRITE_END]);
            close(slave_to_master_stdout[i][READ_END]);
            close(slave_to_master_stderr[i][READ_END]);

            execl(SLAVE_NAME, SLAVE_NAME, (char *)NULL);
        } else if (ret == ERROR_CODE) {
            close_pipes(master_to_slave, slaves);
            close_pipes(slave_to_master_stdout, slaves);
            close_pipes(slave_to_master_stderr, slaves);
            exit(EXIT_FAILURE);
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

    fd_set read_set_fds;
    FD_ZERO(&read_set_fds);

    while (files_in_shm < entries) {

        for (int i = 0; i < slaves; i++) {
            FD_SET(slave_to_master_stdout[i][READ_END], &read_set_fds);
            FD_SET(slave_to_master_stderr[i][READ_END], &read_set_fds);
        }

        if (select(FD_SETSIZE, &read_set_fds, NULL, NULL, NULL) < 0) {
            perror("app | select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < slaves; i++) {
            if (FD_ISSET(slave_to_master_stdout[i][READ_END], &read_set_fds)) {
                char hash[BUFFER];
                ssize_t nbytes;

                if ((nbytes = read(slave_to_master_stdout[i][READ_END], hash,
                                  BUFFER)) != -1) {
                    write(fd_shm, hash, nbytes);
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

            // Handling of errors in slave processes
            if (FD_ISSET(slave_to_master_stderr[i][READ_END], &read_set_fds)) {

                files_in_shm++;

                char error[BUFFER];
                ssize_t nbytes = read(slave_to_master_stderr[i][READ_END], error,
                                     BUFFER - 1);
                error[nbytes] = '\0';
                fprintf(stderr, "%s", error);

                if (strstr(error, MD5_CMD) == NULL) {
                    close_pipe(slave_to_master_stderr[i]);
                    close_pipe(slave_to_master_stdout[i]);
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
        return ERROR_CODE;
    }

    if (ftruncate(fd_shm, length) == ERROR_CODE) {
        return ERROR_CODE;
    }

    void *addr_parent =
        mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if (addr_parent == MAP_FAILED) {
        return ERROR_CODE;
    }

    return fd_shm;
}

void close_pipes(int (*pipes)[2], int slaves) {
    for (int i = 0; i < slaves; i++) {
        close_pipe(pipes[i]);
    }
}

void close_pipe(int pipe[2]) {
    close(pipe[READ_END]);
    close(pipe[WRITE_END]);
}