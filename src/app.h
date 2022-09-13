#define _DEFAULT_SOURCE

#include <sys/mman.h>
#include <sys/select.h>

// Nombre del archivo que será ejecutado como proceso hijo
#define SLAVE_NAME "./slave"
// Cantidad de esclavos que se utilizarán como máximo para procesar todos los
// archivos del programa
#define SLAVES 5
// Cantidad de argumentos que se le transmitirá al esclavo inicialmente (carga
// inicial)
#define LOAD_FACTOR 2

// Nombre y tamaño de la memoria compartida
#define SHM_NAME   "/buffer"
#define SHM_SIZE   (2048 * 5)

#define SEM_NAME   "/sem"

#define BUFFER     256

#define READ_END   0
#define WRITE_END  1

#define ERROR_CODE -1

/**
 * @brief cierra los descriptores de archivos de los 3 pipes y termina la
 * ejecución del programa en caso de error
 *
 */
#define HANDLE_EXIT(p1, p2, p3, s, m)                                          \
    ({                                                                         \
        close_pipes((p1), (s));                                                \
        close_pipes((p2), (s));                                                \
        close_pipes((p3), (s));                                                \
        exit((m));                                                             \
    })

/**
 * @brief crea una memoria compartida, la trunca y la mapea al mapa virtual de
 * direcciones del proceso que llamo a la función
 *
 * @param name nombre de la memoria compartida
 * @param length tamaño en bytes que tendrá la memoria compartida
 * @return int descriptor de archivo de la memoria compartida. ERROR_CODE si
 * hubo un error.
 */
int open_shm(char *name, off_t length);

/**
 * @brief inicializa los pipes a utilizar en app
 *
 * @param master_to_slave matriz de pipes que comunican app con slave
 * @param slave_to_master_stdout matriz de pipes que comunican la salida
 * estandar de slave con app
 * @param slave_to_master_stderr matriz de pipes que comunican la salida de
 * error de slave con app
 * @param count cantidad de pipes a inicializar por matriz
 * @return int EXIT_SUCCESS si no hubo error. ERROR_CODE en caso contrario
 */
int create_pipes(int (*master_to_slave)[2], int (*slave_to_master_stdout)[2],
                 int (*slave_to_master_stderr)[2], size_t count);

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

/**
 * @brief ejecuta el programa SLAVE_NAME, conectando los pipes para que se
 * puedan comunicar el proceso padre con el hijo. En caso de error, abortará el
 * programa
 *
 * @param master_to_slave matriz que contiene el pipe que comunica su descriptor
 * de lectura con la entrada estándar del proceso hijo
 * @param slave_to_master_stdout matriz que contiene el pipe que comunica su
 * descriptor de lectura con la salida estándar del proceso hijo
 * @param slave_to_master_stderr matriz que contiene el pipe que comunica su
 * descriptor de lectura con la salida de error del proceso hijo
 * @param index índice de la matriz que indica los pipes que serán conectados
 * @param count en caso de error, esta variable es requerida para cerrar todos
 * los descriptores de los pipes
 */
void execute(int (*master_to_slave)[2], int (*slave_to_master_stdout)[2],
             int (*slave_to_master_stderr)[2], size_t index, size_t count);
