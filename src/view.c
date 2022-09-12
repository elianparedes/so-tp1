// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
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

#define SHM_NAME    "/buffer"
#define SHM_SIZE    1024
#define BUFFER_SIZE 512

#define SEM_NAME    "/sem"

int main(int argc, char const *argv[]) {

    char shm_name[BUFFER_SIZE];

    int readb;
    if ((readb = read(STDIN_FILENO, shm_name, BUFFER_SIZE - 1)) == -1) {
        perror("view | read");
        exit(EXIT_FAILURE);
    }
    shm_name[readb] = '\0';

    sem_t *s1 = sem_open(SEM_NAME, O_CREAT, 0660, 0);
    if (s1 == SEM_FAILED) {
        perror("view | sem_open");
        exit(EXIT_FAILURE);
    }

    int fd = shm_open(shm_name, O_RDWR, 0);
    if (fd == -1)
        perror("view | shm_open");

    char *shm_buffer =
        mmap(NULL, SHM_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);

    if (shm_buffer == MAP_FAILED)
        perror("view | mmap");

    while (1) {
        sem_wait(s1);

        int shm_buffer_size = strlen(shm_buffer);
        write(STDOUT_FILENO, shm_buffer, shm_buffer_size);
        shm_buffer += shm_buffer_size;
    }

    sem_close(s1);

    return 0;
}
