#define _DEFAULT_SOURCE

#include <fcntl.h> /* For O_* constants */
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h> /* For mode constants */
#include <unistd.h>

#define SLAVES            5
#define SHM_NAME          "/buffer"
#define SLAVE_NAME        "./slave"
#define SHM_SIZE          2048
#define BUFFER            512
#define LOAD_FACTOR       2
#define READ_END          0
#define WRITE_END         1
#define ERROR_CODE        -1

#define SEM_PRODUCER_NAME "/myproducer"
#define SEM_CONSUMER_NAME "/myconsumer"

int main(int argc, char const *argv[]) {
    int master_to_slave[SLAVES][2];
    int slave_to_master[SLAVES][2];
    int files_in_hash = 1;
    int files_in_shm = 0;

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_PRODUCER_NAME);
    sem_unlink(SEM_CONSUMER_NAME);

    sem_t *s1 = sem_open(SEM_PRODUCER_NAME, O_CREAT, 0660, 1);
    if (s1 == SEM_FAILED) {
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }

    /*sem_t *s2 = sem_open(SEM_CONSUMER_NAME, O_CREAT, 0660, 1);
    if (s2 == SEM_FAILED) {
        perror("sem_open/consumer");
        exit(EXIT_FAILURE);
    }*/

    fd_set read_set_fds;
    FD_ZERO(&read_set_fds);

    for (int i = 0; i < SLAVES; i++) {
        pipe(slave_to_master[i]);
        pipe(master_to_slave[i]);
    }

    sleep(2);
    printf("/buffer");
    fflush(stdout);

    int fd_shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_shm == ERROR_CODE) {
        perror("app | shm_open");
        _exit(EXIT_FAILURE);
    }
    if (ftruncate(fd_shm, SHM_SIZE) == ERROR_CODE) {
        perror("app | ftruncate");
        _exit(EXIT_FAILURE);
    }

    void *addr_parent =
        mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if (addr_parent == MAP_FAILED) {
        perror("app | mmap");
        _exit(EXIT_FAILURE);
    }

    for (int i = 0; i < SLAVES; i++) {
        char initial_load[BUFFER];
        initial_load[0] = '\0';

        for (size_t j = 1; j <= LOAD_FACTOR; j++) {
            strcat(initial_load, argv[files_in_hash++]);
            if (j < LOAD_FACTOR) {
                strcat(initial_load, " ");
            }
        }
        strcat(initial_load, "\n");

        if (fork() == 0) {
            close(STDIN_FILENO);
            dup2(master_to_slave[i][READ_END], STDIN_FILENO);

            write(master_to_slave[i][WRITE_END], initial_load,
                  strlen(initial_load) + 1);

            close(STDOUT_FILENO);
            dup2(slave_to_master[i][WRITE_END], STDOUT_FILENO);

            execl(SLAVE_NAME, SLAVE_NAME, (char *)NULL);
        }
    }

    while (files_in_shm < (argc - 1) - SLAVES) {
        for (int i = 0; i < SLAVES; i++) {
            FD_SET(slave_to_master[i][READ_END], &read_set_fds);
        }

        if (select(FD_SETSIZE, &read_set_fds, NULL, NULL, NULL) < 0) {
            perror("app | select");
            _exit(EXIT_FAILURE);
        }

        for (int i = 0; i < SLAVES; i++) {
            if (FD_ISSET(slave_to_master[i][READ_END], &read_set_fds)) {
                char hash[BUFFER];
                int numBytes = read(slave_to_master[i][READ_END], hash, BUFFER);
                hash[numBytes] = '\0';

                // sem_wait(s2);
                sleep(1);

                write(fd_shm, hash, numBytes);
                files_in_shm++;

                sem_post(s1);

                if (files_in_hash < argc) {
                    char input_buffer[BUFFER];

                    strcpy(input_buffer, argv[files_in_hash++]);
                    strcat(input_buffer, "\n");

                    if (write(master_to_slave[i][WRITE_END], input_buffer,
                              strlen(input_buffer) + 1) == ERROR_CODE) {
                        perror("app | write");
                    }
                }
            }
        }
    }

    // sem_close(s2);
    sem_close(s1);

    return 0;
}
