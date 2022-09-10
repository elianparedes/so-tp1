// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>


int main(int argc, char const *argv[]) {

    pid_t pid;

    if ((pid = fork()) == -1)
        perror("fork error");
    else if (pid == 0) {
        // cambiar el stdout

        execl("/bin/md5sum", "md5sum", "./text.txt", NULL);
    }

    return 0;
}
