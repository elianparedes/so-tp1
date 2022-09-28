// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 512
#define SEM_NAME    "/sem"
#define FIFO_NAME   "/fifofile2"

int main(int argc, char const *argv[]) {

    char shm_name[BUFFER_SIZE];
    int shm_size;

    // Indicamos el tamaño del string a leer para corregir el warning de
    // PVS-STUDIO debido a las limitaciones de fscanf.
    if (fscanf(stdin, "%511s %d", shm_name, &shm_size) == EOF) {
        perror("view | fscanf");
        exit(EXIT_FAILURE);
    }

    int fifo_fd = open(FIFO_NAME, O_RDONLY); 

    if (shm_size <= 0) {
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(SEM_NAME, O_RDONLY, 0);
    if (sem == SEM_FAILED) {
        perror("view | sem_open");
        exit(EXIT_FAILURE);
    }

    int fd = shm_open(shm_name, O_RDWR, 0);
    if (fd == -1)
        perror("view | shm_open");

    char *shm_buffer =
        mmap(NULL, shm_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);

    if (shm_buffer == MAP_FAILED)
        perror("view | mmap");

    char buff[BUFFER_SIZE];

    while (1) {
        sem_wait(sem);
        int rbytes = read(fifo_fd, buff, BUFFER_SIZE);
        printf("%s\n", buff);
        fflush(stdout);
    }

    return 0;
}