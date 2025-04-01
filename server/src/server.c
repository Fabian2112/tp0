#include "server.h"

void iterator(void* value) {
	log_info(logger,"%s", (char*)value);
}

int main(void) {
	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	if (logger == NULL) {
        perror("Error al crear el logger");
        return EXIT_FAILURE;
    }

	int server_fd = iniciar_servidor();
	if (server_fd == -1) {
        log_error(logger, "No se pudo iniciar el servidor");
        return EXIT_FAILURE;
    }
	
	
	log_info(logger, "Servidor listo para recibir al cliente");
	int cliente_fd = esperar_cliente(server_fd);
	
	if (cliente_fd < 0) {
        log_error(logger, "Error al aceptar cliente");
        return EXIT_FAILURE;
    }

	t_list* lista;

	while (1) {
		int cod_op = recibir_operacion(cliente_fd);
		if (cod_op == -1) {
            log_error(logger, "El cliente se desconectó. Terminando servidor");
            break;  // Se cierra el servidor si el cliente se desconecta.
        }
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(cliente_fd);
			break;
		case PAQUETE:
			lista = recibir_paquete(cliente_fd);
			if (lista != NULL) {
				log_info(logger, "Me llegaron los siguientes valores:\n");
				list_iterate(lista, iterator);
				list_destroy_and_destroy_elements(lista, free);
			}
			break;
		default:
			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}

	close(cliente_fd);
    close(server_fd);

    log_destroy(logger);
	
	return EXIT_SUCCESS;
}


