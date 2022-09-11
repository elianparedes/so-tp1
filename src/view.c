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

#define SHM_NAME          "/buffer"
#define SHM_SIZE          1024
#define BUFFER_SIZE       512

#define SEM_PRODUCER_NAME "/myproducer"
#define SEM_CONSUMER_NAME "/myconsumer"

int main(int argc, char const *argv[]) {

    char shm_name[BUFFER_SIZE];
    int readb = read(0, shm_name, BUFFER_SIZE);
    shm_name[readb] = '\0';

    sem_t *s1 = sem_open(SEM_PRODUCER_NAME, O_CREAT, 0660, 0);
    if (s1 == SEM_FAILED) {
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }

    sem_t *s2 = sem_open(SEM_CONSUMER_NAME, O_CREAT, 0660, 0);
    if (s2 == SEM_FAILED) {
        perror("sem_open/consumer");
        exit(EXIT_FAILURE);
    }

    int fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (fd == -1)
        perror("shm_open parent");

    char *addr_child =
        mmap(NULL, SHM_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);

    if (addr_child == MAP_FAILED)
        perror("mmap child");

    while (1) {
        sem_wait(s1);
        printf("%s", addr_child);
        addr_child += strlen(addr_child);
        // sem_post(s2);
    }

    sem_close(s2);
    sem_close(s1);

    return 0;
}
