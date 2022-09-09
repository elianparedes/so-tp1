#define _BSD_SOURCE

#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <string.h>

#define SLAVES 5
#define SHM_NAME "/buffer"
#define SHM_SIZE 1024
#define BUFFER 512
#define LOAD_FACTOR 2

int main(int argc, char const *argv[])
{

    int ms_pipe[SLAVES][2];
    int sm_pipe[SLAVES][2];
    int files_hashed= 1;

    fd_set read_set_fds;

    FD_ZERO(&read_set_fds);

    for (int i=0; i < SLAVES; i++){
        pipe(sm_pipe[i]);
    }

    int fd_shm= shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_shm == -1) perror("shm | shm_open");

    if (ftruncate(fd_shm, SHM_SIZE) == -1) perror("shm | truncate");

    void *addr_parent = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm ,0);
    if(addr_parent == MAP_FAILED) perror("shm | mmap");

    char buffer[SLAVES][BUFFER*LOAD_FACTOR] = {0};
    for (int i=0; i < SLAVES && files_hashed < argc; i++){
        for (int j=0; j < LOAD_FACTOR && files_hashed < argc; j++){
            strcat(buffer[i], argv[files_hashed++]);
            if (j != LOAD_FACTOR-1){
                strcat(buffer[i], " ");
            }
        }
    }

    for (int i=0; i < SLAVES; i++){
        if (fork()==0){
            pipe(ms_pipe[i]);
            close(0);
            dup2(ms_pipe[i][0], 0);
            write(ms_pipe[i][1], buffer[i], BUFFER*LOAD_FACTOR);    

            close(1);
            dup2(sm_pipe[i][1], 1);
            
            execl("./slave", "./slave", (char *) NULL);
        }
    }

    while(files_hashed < (argc-1)){
        for (int i=0; i < SLAVES; i++){
            FD_SET(sm_pipe[i][0], &read_set_fds);
        }

        if ( select(FD_SETSIZE, &read_set_fds, NULL, NULL, NULL) < 0 ) {
            perror("shm | select");
            exit(0);
        }
        
        for (int i=0; i < SLAVES; i++){
            if (FD_ISSET(sm_pipe[i][0], &read_set_fds)){
                char hash[BUFFER];
                int numBytes= read(sm_pipe[i][0], hash, BUFFER);
                hash[numBytes]='\0';
                write(fd_shm, hash, numBytes +1);

                write(ms_pipe[i][1], argv[files_hashed++], BUFFER);
            }
        }
        
    }
    return 0;
}
