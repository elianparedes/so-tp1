#include <stdio.h>
#include <unistd.h>
#define BUFFER 512
#define LOAD_FACTOR 2

int main(void)
{       
    char path[BUFFER*LOAD_FACTOR];
    while (1){
        int num_bytes= read(0, path, BUFFER*LOAD_FACTOR);
        path[num_bytes]='\0';
        if ( fork()==0){
            execl("/bin/md5sum", "md5sum", path, (char *)NULL);
        }
    }
    return 0;
}
