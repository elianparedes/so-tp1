// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "app.h"

#include <fcntl.h> /* For O_* constants */
#include <math.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h> /* For mode constants */
#include <unistd.h>

static int slaves;
static int files_in_hash;
static int total_files;

/**
 * @brief programa que recibe nombres de archivos por parámetros y los hashea con el programa md5sum utilizando procesos hijos.
 * Guarda el resultado en una shared memory con el nombre SHM_NAME.
 * Imprime por salida estándar el nombre de la shared memory y su tamaño en bytes.
 * Imprime por error estándar un mensaje indicando el problema si hubiera alguno en la ejecución.
 *
 * @param argc cantidad de argumentos pasadas como parámetros
 * @param argv paths de los archivos a hashear
 * @return int EXIT_SUCCESS si el programa fue exitoso. EXIT_FAILURE en caso contrario
 */
int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "%s", "app | missing arguments\n");
        exit(EXIT_FAILURE);
    }

    total_files = argc - 1;

    // La cantidad de esclavos a utilizar se define como el mínimo entre la variable SLAVES y la cantidad necesaria de esclavos para los archivos
    // pasados como parámetros, para poder optimizar la asignación de recursos.
    slaves = SLAVES * LOAD_FACTOR > total_files
                 ? ceil((double)(total_files) / (double)LOAD_FACTOR)
                 : SLAVES;

    // Pipes para los procesos. Uno para llevar información del master al slave, y dos para llevar información del slave
    // al master(salida estandar y error estandar)
    int master_to_slave[slaves][2];
    int slave_to_master_stdout[slaves][2];
    int slave_to_master_stderr[slaves][2];

    files_in_hash = 1;
    int files_in_shm = 0;

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);

    // Se crea el semáforo para utilizarlo posteriormente.

    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0660, 0);
    if (sem == SEM_FAILED)
    {
        perror("app | sem_open");
        exit(EXIT_FAILURE);
    }

    // TODO: ERRORES
    open_pipes(master_to_slave, slaves);
    open_pipes(slave_to_master_stdout, slaves);
    open_pipes(slave_to_master_stderr, slaves);

    int fd_shm = open_shm(SHM_NAME, BUFFER);
    if (fd_shm == ERROR_CODE)
    {
        perror("app | open_shm ");
        exit(EXIT_FAILURE);
    }

    // Se esperan dos segundos para tener la oportunidad de conectar el programa view.c
    // Luego se imprime por salida estandar la información necesaria para conectar el programa app.c con el view.c
    sleep(2);
    printf("%s %d", SHM_NAME, BUFFER);
    if (fflush(stdout) == EOF)
    {
        perror("app | fflush");
        exit(EXIT_FAILURE);
    }

    // Se crean los procesos hijos y se le transmite por los pipes la cantidad de argumentos inicial
    // que tendran que procesar (initial_load)

    char initial_load[BUFFER * slaves + slaves];

    for (int i = 0; i < slaves; i++)
    {
        
        copy_arguments(initial_load, argv);

        write(master_to_slave[i][WRITE_END], initial_load,
              strlen(initial_load) + 1);

        int ret;
        if ((ret = fork()) == 0)
        {
            execute(master_to_slave, slave_to_master_stdout, slave_to_master_stderr, i);
        }
        else if (ret == ERROR_CODE)
        {
            close_pipes(master_to_slave, slaves);
            close_pipes(slave_to_master_stdout, slaves);
            close_pipes(slave_to_master_stderr, slaves);
            exit(EXIT_FAILURE);
        }
    }

    // Luego de crear los procesos hijos, el proceso ./app se queda esperando los resultados del hash mediante la función select.
    // Cuando un proceso se libera, se le envia el próximo path a hashear.
    // El proceso se quedará esperando hasta que todos los resultados hayan sido procesados (copiados a la memoria compartida o impreso
    // en error estándar)
    size_t entries;

    if (slaves < SLAVES)
    {
        entries = slaves;
    }
    else
    {
        if (argc - 1 < slaves * LOAD_FACTOR)
        {
            entries = slaves + 1;
        }
        else
        {
            entries = argc - slaves - 1;
        }
    }

    fd_set read_set_fds;
    FD_ZERO(&read_set_fds);

    while (files_in_shm < entries)
    {

        // Se prepara el struct de read_set_fds para el select
        for (int i = 0; i < slaves; i++)
        {
            if (slave_to_master_stdout[i][READ_END] != ERROR_CODE)
            {
                FD_SET(slave_to_master_stdout[i][READ_END], &read_set_fds);
                FD_SET(slave_to_master_stderr[i][READ_END], &read_set_fds);
            }
        }

        if (select(FD_SETSIZE, &read_set_fds, NULL, NULL, NULL) < 0)
        {
            perror("app | select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < slaves; i++)
        {
            // Recepción de hasheos exitosos de los procesos.
            if (FD_ISSET(slave_to_master_stdout[i][READ_END], &read_set_fds))
            {
                char hash[BUFFER];
                ssize_t nbytes;

                if ((nbytes = read(slave_to_master_stdout[i][READ_END], hash,
                                   BUFFER)) != ERROR_CODE)
                {
                    write(fd_shm, hash, nbytes);
                    files_in_shm++;
                    sem_post(sem);

                    if (files_in_hash < argc)
                    {
                        char input_buffer[BUFFER];

                        strncpy(input_buffer, argv[files_in_hash++], BUFFER - 1);

                        if (write(master_to_slave[i][WRITE_END], input_buffer,
                                  strlen(input_buffer) + 1) == ERROR_CODE)
                        {
                            perror("app | write");
                        }
                    }
                }
                else
                {
                    close_pipe(slave_to_master_stderr[i]);
                    close_pipe(slave_to_master_stdout[i]);
                    FD_CLR(slave_to_master_stdout[i][READ_END], &read_set_fds);
                    FD_CLR(slave_to_master_stderr[i][READ_END], &read_set_fds);
                    slave_to_master_stdout[i][READ_END] = ERROR_CODE;
                    slave_to_master_stderr[i][READ_END] = ERROR_CODE;
                }
            }

            // Recepción de errores de los procesos. Si el error no está relacionado con
            // el programa md5sum, se abortará el proceso y se cerrarán sus pipes, para no enviarle más archivos.
            if (FD_ISSET(slave_to_master_stderr[i][READ_END], &read_set_fds))
            {
                files_in_shm++;

                char error[BUFFER];
                ssize_t nbytes = read(slave_to_master_stderr[i][READ_END], error,
                                      BUFFER - 1);
                if (nbytes == ERROR_CODE)
                {
                    perror("app | read stderr");
                    close_pipe(slave_to_master_stderr[i]);
                    close_pipe(slave_to_master_stdout[i]);
                    FD_CLR(slave_to_master_stdout[i][READ_END], &read_set_fds);
                    FD_CLR(slave_to_master_stderr[i][READ_END], &read_set_fds);
                    slave_to_master_stdout[i][READ_END] = ERROR_CODE;
                    slave_to_master_stderr[i][READ_END] = ERROR_CODE;
                }
                else
                {
                    error[nbytes] = '\0';
                    fprintf(stderr, "%s", error);

                    if (strstr(error, MD5_CMD) == NULL)
                    {
                        close_pipe(slave_to_master_stderr[i]);
                        close_pipe(slave_to_master_stdout[i]);
                        FD_CLR(slave_to_master_stdout[i][READ_END], &read_set_fds);
                        FD_CLR(slave_to_master_stderr[i][READ_END], &read_set_fds);
                        slave_to_master_stdout[i][READ_END] = ERROR_CODE;
                        slave_to_master_stderr[i][READ_END] = ERROR_CODE;
                    }
                }
            }
        }
    }

    // Se cierran todos los pipes utilizados. Al cerrar los pipes, los procesos recibirán la terminación en el read y liberarán sus recursos.
    // No hay necesidad de hacer wait
    close_pipes(master_to_slave, slaves);
    close_pipes(slave_to_master_stdout, slaves);
    close_pipes(slave_to_master_stderr, slaves);

    sem_close(sem);

    return EXIT_SUCCESS;
}

int open_shm(char *name, off_t length)
{
    int fd_shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

    if (fd_shm == ERROR_CODE)
    {
        return ERROR_CODE;
    }

    if (ftruncate(fd_shm, length) == ERROR_CODE)
    {
        return ERROR_CODE;
    }

    void *addr_shm =
        mmap(NULL, BUFFER, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if (addr_shm == MAP_FAILED)
    {
        return ERROR_CODE;
    }

    return fd_shm;
}

int open_pipes(int (*pipes)[2], int level)
{
    for (int i = 0; i < level; i++)
    {
        if (pipe(pipes[i]) == ERROR_CODE)
        {
            return ERROR_CODE;
        }
    }
    return EXIT_SUCCESS;
}

void close_pipes(int (*pipes)[2], int slaves)
{
    for (int i = 0; i < slaves; i++)
    {
        close_pipe(pipes[i]);
    }
}

void close_pipe(int pipe[2])
{
    close(pipe[READ_END]);
    close(pipe[WRITE_END]);
}

int copy_arguments(char *dest, const char **source)
{
    strncpy(dest, source[files_in_hash++], BUFFER - 1);

    for (size_t j = 1; j < LOAD_FACTOR && files_in_hash < (total_files + 1); j++)
    {
        strcat(dest, " ");
        strncat(dest, source[files_in_hash++], BUFFER - 1);
    }

    return EXIT_SUCCESS;
}

void execute(int (*master_to_slave)[2], int (*slave_to_master_stdout)[2], int (*slave_to_master_stderr)[2], int level)
{
    close(STDIN_FILENO);
    dup2(master_to_slave[level][READ_END], STDIN_FILENO);

    close(STDOUT_FILENO);
    dup2(slave_to_master_stdout[level][WRITE_END], STDOUT_FILENO);

    close(STDERR_FILENO);
    dup2(slave_to_master_stderr[level][WRITE_END], STDERR_FILENO);

    close(master_to_slave[level][WRITE_END]);
    close(slave_to_master_stdout[level][READ_END]);
    close(slave_to_master_stderr[level][READ_END]);

    if (execl(SLAVE_NAME, SLAVE_NAME, (char *)NULL) == ERROR_CODE)
    {
        close_pipes(master_to_slave, slaves);
        close_pipes(slave_to_master_stdout, slaves);
        close_pipes(slave_to_master_stderr, slaves);
        exit(EXIT_FAILURE);
    }
}