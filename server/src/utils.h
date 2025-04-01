#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <string.h>
#include <assert.h>

#define PUERTO "4444"

// Definición de tipos de operación
typedef enum {
    MENSAJE,
    PAQUETE
} op_code;

// Definición de t_buffer
typedef struct {
    int size;       // Tamaño del buffer
    void* stream;   // Datos del buffer
} t_buffer;

// Definición de t_paquete
typedef struct {
    int codigo_operacion;  // Código de operación
    t_buffer* buffer;      // Buffer que contiene los datos
} t_paquete;

// Declaración de la variable logger
extern t_log* logger;

// Declaración de funciones
int iniciar_servidor(void);
int esperar_cliente(int socket_servidor);
int recibir_operacion(int socket_cliente);
void* recibir_buffer(int* size, int socket_cliente);
void recibir_mensaje(int socket_cliente);
t_list* recibir_paquete(int socket_cliente);

// Declaración de funciones adicionales que puedas necesitar
t_paquete* recibir_paquete_completo(int socket_cliente);
void procesar_paquete(t_paquete* paquete);
void eliminar_paquete(t_paquete* paquete);

#endif /* UTILS_H_ */
