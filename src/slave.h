#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE

#define BUFFER_SIZE 512
#define ERROR_SIZE  25

#define MD5_CMD     "md5sum "
#define CMD_SIZE    10
#define RD_ONLY     "r"

#define ERROR_CODE  -1

/**
 * @brief función que libera los recursos alocados en memoria y termina el programa
 * 
 * @param exit_status estado de terminación.
 */
void exit_slave(int exit_status);