#define _DEFAULT_SOURCE

#include <sys/mman.h>

#define SLAVE_NAME "./slave"
#define SLAVES 5
#define LOAD_FACTOR 2

#define SHM_NAME "/buffer"
#define SHM_SIZE 2048

#define SEM_NAME "/sem"

#define BUFFER 256

#define READ_END 0
#define WRITE_END 1

#define ERROR_CODE -1

int open_shm(char *name, off_t length);