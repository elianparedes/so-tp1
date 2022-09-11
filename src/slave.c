#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER     512

#define MD5_CMD    "md5sum "
#define CMD_SIZE   10
#define RD_ONLY    "r"

#define ERROR_CODE -1

char *output_buffer = NULL;

void exit_program(int exit_status);

int main(void) {

    while (1) {
        char input_buffer[BUFFER];
        char command[BUFFER + CMD_SIZE] = MD5_CMD;

        FILE *md5_fd;
        size_t line_buffer = BUFFER;

        int nbytes;
        if ((nbytes = read(0, input_buffer, BUFFER)) == -1) {
            exit_program(EXIT_FAILURE);
        }

        input_buffer[nbytes] = '\0';

        strcpy(command, MD5_CMD);
        strcat(command, input_buffer);

        if ((md5_fd = popen(command, RD_ONLY)) == NULL) {
            perror("popen");
            exit_program(EXIT_FAILURE);
        }

        while (getline((char **)&output_buffer, &line_buffer, md5_fd) != EOF) {
            printf("process pid: %d | %s", getpid(), output_buffer);
        }

        fflush(stdout);

        if (pclose(md5_fd) == ERROR_CODE) {
            exit_program(EXIT_FAILURE);
        }
    }

    exit_program(EXIT_SUCCESS);
}

void exit_program(int exit_status) {
    if (output_buffer != NULL) {
        free(output_buffer);
    }

    _exit(exit_status);
}