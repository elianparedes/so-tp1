#define _DEFAULT_SOURCE

#include <sys/mman.h>

// Nombre del archivo que será ejecutado como proceso hijo
#define SLAVE_NAME  "./slave"
// Cantidad de esclavos que se utilizarán como máximo para procesar todos los archivos del programa
#define SLAVES      5
// Cantidad de argumentos que se le transmitirá al esclavo inicialmente (carga inicial)
#define LOAD_FACTOR 2

// Nombre y tamaño de la memoria compartida
#define SHM_NAME    "/buffer"
#define SHM_SIZE    2048

#define SEM_NAME    "/sem"

#define BUFFER      256

#define READ_END    0
#define WRITE_END   1

#define ERROR_CODE  -1

#define MD5_CMD "md5sum"

/**
 * @brief crea una memoria compartida, la trunca y la mapea al mapa virtual de direcciones del proceso que llamo a la función
 * 
 * @param name nombre de la memoria compartida
 * @param length tamaño en bytes que tendrá la memoria compartida
 * @return int descriptor de archivo de la memoria compartida
 */
int open_shm(char *name, off_t length);

/**
 * @brief cierra los descriptores de archivos de escritura y lectura de un pipe
 * 
 * @param pipe pipe que será cerrado
 */
void close_pipe(int pipe[2]);

/**
 * @brief cierra todos los pipes de una matriz
 * 
 * @param pipes matriz que contiene los pipes (en sus filas)
 * @param level hasta que fila de la matriz la función cerrará pipes
 */
void close_pipes(int (*pipes)[2], int level);