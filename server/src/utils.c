#include "utils.h"

t_log* logger;

int iniciar_servidor(void)
{
    int socket_servidor;
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PUERTO, &hints, &servinfo);

    socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socket_servidor == -1) {
        log_error(logger, "Error al crear el socket de servidor.");
        freeaddrinfo(servinfo);
        return -1;
    }

    if (bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        log_error(logger, "Error al asociar el socket con el puerto.");
        close(socket_servidor);
        freeaddrinfo(servinfo);
        return -1;
    }

    if (listen(socket_servidor, SOMAXCONN) == -1) {
        log_error(logger, "Error al escuchar conexiones entrantes.");
        close(socket_servidor);
        freeaddrinfo(servinfo);
        return -1;
    }

    freeaddrinfo(servinfo);
    log_trace(logger, "Listo para escuchar a mi cliente");

    return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
    int socket_cliente = accept(socket_servidor, NULL, NULL);
    if (socket_cliente == -1) {
        log_error(logger, "Error al aceptar la conexión del cliente.");
    } else {
        log_info(logger, "Cliente conectado.");
    }

    return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
    int cod_op;
    ssize_t bytes_recibidos = recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL);

    if (bytes_recibidos <= 0) {
        // Si el cliente se desconectó, logueamos un mensaje específico
        if (bytes_recibidos == 0) {
            log_info(logger, "El cliente se desconectó.");
        } else {
            // En otro caso, es un error relacionado con la recepción de la operación
            log_error(logger, "Error al recibir código de operación.");
        }
        close(socket_cliente);
        return -1;
    }

    return cod_op;
}

void* recibir_buffer(int* size, int socket_cliente)
{
    void *buffer;

    // Recibimos el tamaño del buffer (se espera que el cliente primero envíe el tamaño)
    if (recv(socket_cliente, size, sizeof(int), MSG_WAITALL) <= 0) {
        log_error(logger, "Error al recibir tamaño del buffer.");
        return NULL;
    }

    // Recibimos el contenido completo del buffer
    buffer = malloc(*size);
    if (recv(socket_cliente, buffer, *size, MSG_WAITALL) <= 0) {
        log_error(logger, "Error al recibir el contenido del buffer.");
        free(buffer);
        return NULL;
    }

    return buffer;
}

void recibir_mensaje(int socket_cliente)
{
    int size;
    char* buffer;

    // Recibimos el tamaño y el contenido juntos como un paquete
    if (recv(socket_cliente, &size, sizeof(int), MSG_WAITALL) <= 0 || size <= 0) {
        log_error(logger, "Error al recibir el tamaño del mensaje o tamaño inválido.");
        return;
    }

    buffer = malloc(size + 1); // +1 para el terminador '\0'
    if (recv(socket_cliente, buffer, size, MSG_WAITALL) <= 0) {
        log_error(logger, "Error al recibir el mensaje.");
        free(buffer);
        return;
    }
    buffer[size] = '\0'; // Terminamos correctamente el string

    log_info(logger, "Mensaje recibido: %s", buffer);
    free(buffer);
}

t_list* recibir_paquete(int socket_cliente)
{
    int size;
    int desplazamiento = 0;
    void *buffer;
    t_list* valores = list_create();
    int tamanio;

    // Recibimos el paquete completo (tamaño + contenido)
    buffer = recibir_buffer(&size, socket_cliente);
    if (buffer == NULL) {
        return NULL;
    }

    // Extraemos los valores del paquete
    while (desplazamiento < size) {
        memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
        desplazamiento += sizeof(int);
        char* valor = malloc(tamanio);
        memcpy(valor, buffer + desplazamiento, tamanio);
        desplazamiento += tamanio;
        list_add(valores, valor);
    }

    free(buffer);
    return valores;
}

t_paquete* recibir_paquete_completo(int socket_cliente)
{
    int bytes_recibidos;
    int bytes_a_recibir;
    void* buffer;

    // Primero, recibimos el tamaño total del paquete
    bytes_recibidos = recv(socket_cliente, &bytes_a_recibir, sizeof(int), MSG_WAITALL);
    if (bytes_recibidos <= 0) {
        log_error(logger, "Error al recibir el tamaño del paquete.");
        close(socket_cliente);
        return NULL;
    }

    // Luego, recibimos el paquete completo
    buffer = malloc(bytes_a_recibir);
    bytes_recibidos = recv(socket_cliente, buffer, bytes_a_recibir, MSG_WAITALL);
    if (bytes_recibidos <= 0) {
        log_error(logger, "Error al recibir el contenido del paquete.");
        free(buffer);
        close(socket_cliente);
        return NULL;
    }

    // Deserializamos el paquete recibido
    int desplazamiento = 0;
    int codigo_operacion;
    memcpy(&codigo_operacion, buffer + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);

    int tamano_buffer;
    memcpy(&tamano_buffer, buffer + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);

    void* stream = malloc(tamano_buffer);
    memcpy(stream, buffer + desplazamiento, tamano_buffer);

    // Crear el paquete recibido
    t_paquete* paquete_recibido = malloc(sizeof(t_paquete));
    paquete_recibido->codigo_operacion = codigo_operacion;
    paquete_recibido->buffer = malloc(sizeof(t_buffer));
    paquete_recibido->buffer->size = tamano_buffer;
    paquete_recibido->buffer->stream = stream;

    // Logueamos el mensaje recibido
    log_info(logger, "Paquete recibido con código de operación: %d y tamaño de buffer: %d", codigo_operacion, tamano_buffer);

    free(buffer);
    // Procesamos el paquete recibido
    procesar_paquete(paquete_recibido);

    // Liberamos el paquete recibido
    eliminar_paquete(paquete_recibido);

	return paquete_recibido;
}

void procesar_paquete(t_paquete* paquete)
{
    // Aquí procesamos el paquete recibido según el código de operación.
    if (paquete->codigo_operacion == MENSAJE) {
        log_info(logger, "Mensaje recibido: %s", (char*)paquete->buffer->stream);
    }
}

void eliminar_paquete(t_paquete* paquete)
{
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}
