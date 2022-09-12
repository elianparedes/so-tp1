// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE
#include "slave.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static char *output_buffer = NULL;

int main(void) {

    char command[BUFFER_SIZE + CMD_SIZE];

    FILE *md5_fd;
    size_t line_buffer = BUFFER_SIZE;

    char input_buffer[BUFFER_SIZE];
    int nbytes;

    while (1) {
        if ((nbytes = read(STDIN_FILENO, input_buffer, BUFFER_SIZE - 1)) ==
            ERROR_CODE) {
            perror("slave | read");
            if (output_buffer != NULL) {
                free(output_buffer);
            }

            exit(EXIT_FAILURE);
        }
        input_buffer[nbytes] = '\0';

        strcpy(command, MD5_CMD);
        strcat(command, input_buffer);

        if ((md5_fd = popen(command, RD_ONLY)) == NULL) {
            perror("popen");

            if (output_buffer != NULL) {
                free(output_buffer);
            }
            exit(EXIT_FAILURE);
        }

        while (getline(&output_buffer, &line_buffer, md5_fd) != EOF) {
            printf("process pid: %d | %s", getpid(), output_buffer);
        }

        if (fflush(stdout) == EOF) {
            perror("slave | fflush");
            exit_slave(EXIT_FAILURE);
        }

        if (pclose(md5_fd) == ERROR_CODE) {
            perror("slave | pclose");
            exit_slave(EXIT_FAILURE);
        }
    }

    exit_slave(EXIT_SUCCESS);
}

void exit_slave(int exit_status) {
    if (output_buffer != NULL) {
        free(output_buffer);
    }

    exit(exit_status);
}