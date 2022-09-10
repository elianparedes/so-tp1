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

    scanf("%s", shm_name);

    sem_unlink(SEM_PRODUCER_NAME);
    sem_unlink(SEM_CONSUMER_NAME);

    sem_t *sem_prod = sem_open(SEM_PRODUCER_NAME, O_CREAT, 0660, 1);
    if (sem_prod == SEM_FAILED) {
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }

    sem_t *sem_cons = sem_open(SEM_CONSUMER_NAME, O_CREAT, 0660, 1);
    if (sem_cons == SEM_FAILED) {
        perror("sem_open/consumer");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];

    int fd = shm_open(shm_name, O_RDWR, 0);
    if (fd == -1)
        perror("shm_open parent");

    char *addr_child =
        mmap(NULL, SHM_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);

    if (addr_child == MAP_FAILED)
        perror("mmap child");

    while (1) {
        sem_wait(sem_prod);
        printf("%s\n", addr_child);
        sem_post(sem_cons);
    }

    sem_close(sem_cons);
    sem_close(sem_prod);

    return 0;
}
