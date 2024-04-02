#include "sockets.h"

/**
 * Client
 */

void *serializar_paquete(t_paquete *paquete, int bytes)
{
	void *magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento += sizeof(int);
	printf("DESPLAZAMIENTO 1: %d\n", desplazamiento);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento += sizeof(int);
	printf("DESPLAZAMIENTO 2: %d\n", desplazamiento);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;
	printf("DESPLAZAMIENTO FINAL (TENDRIA Q SER 10): %d\n", desplazamiento);
	return magic;
}

int crear_conexion(t_config *config, char *key_ip, char *key_puerto, t_log *logger)
{
	char *ip = config_get_string_value(config, key_ip);
	char *puerto = config_get_string_value(config, key_puerto);

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &servinfo);

	int sc = socket(servinfo->ai_family,
					servinfo->ai_socktype,
					servinfo->ai_protocol);

	int connection = connect(sc, servinfo->ai_addr, servinfo->ai_addrlen);
	if (connection == -1)
	{
		log_error(logger, "Cliente: Fallo la conexion, se cierra el cliente.");
		return -1;
	}
	log_info(logger, "Cliente: Pude establecer conexion con el servidor");
	freeaddrinfo(servinfo);

	// handshake
	uint32_t handshake = 1;
	uint32_t result;
	log_info(logger, "Cliente: Tratando de hacer el handshake");
	send(sc, &handshake, sizeof(uint32_t), NULL);
	recv(sc, &result, sizeof(uint32_t), MSG_WAITALL);
	if (result == -1)
	{
		log_error(logger, "Cliente: error al hacer el handshake");
		return -1;
	}

	log_info(logger, "Cliente: handshake exitoso");
	return sc;
}

void enviar_mensaje(char *mensaje, int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2 * sizeof(int);
	printf("SIZE BYTES: %d\n", bytes);
	void *a_enviar = serializar_paquete(paquete, bytes);

	int c = send(socket_cliente, a_enviar, bytes, 0);
	printf("BYTES ENVIADOS: %d\n", c);
	free(a_enviar);
	eliminar_paquete(paquete);
}

void crear_buffer(t_paquete *paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete *crear_paquete(void)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), &valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete *paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2 * sizeof(int);
	void *a_enviar = serializar_paquete(paquete, bytes);
	printf("BUFFER ANTES DE ENVIAR: %d\n", paquete->buffer->size);
	printf("BYTES ANTES DE ENVIAR: %d\n", bytes);
	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete *paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}

/**
 * Server
 */

int iniciar_servidor(t_config *config, char *key_puerto)
{
	int socket_servidor;
	char *ip = config_get_string_value(config, "IP");
	char *puerto = config_get_string_value(config, key_puerto);

	// init address info
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &servinfo);

	// init socket
	socket_servidor = socket(servinfo->ai_family,
							 servinfo->ai_socktype,
							 servinfo->ai_protocol);

	// bind socket to the server's addrinfo
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	// starts listening socket
	listen(socket_servidor, SOMAXCONN);

	// frees addrinfo mem space
	freeaddrinfo(servinfo);

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL);
	// log_info(logger, "Se conecto un cliente!");

	// handshake
	uint32_t handshake;
	uint32_t resultOk = 0;
	uint32_t resultError = -1;

	recv(socket_cliente, &handshake, sizeof(uint32_t), MSG_WAITALL);
	if (handshake == 1)
		send(socket_cliente, &resultOk, sizeof(uint32_t), NULL);
	else
		send(socket_cliente, &resultError, sizeof(uint32_t), NULL);

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	int sexo = recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL);
	if (sexo > 0)
	{
		printf("CODIGO DE OPERACION: %d | CANT BYTES RECV: %d \n", cod_op, sexo);
		int size;
		int asd = recv(socket_cliente, &size, sizeof(int), MSG_WAITALL);
		printf("SIZE DEL SIZE: %d | SIZE: %d\n", asd, size);
		return cod_op;
	}
	close(socket_cliente);
	return -1;
}

void *recibir_buffer(int *size, int socket_cliente)
{
	void *buffer;
	printf("ESTOY ESPERANDO AL RECV\n");
	int asd = recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	printf("SIZE DEL SIZE: %d | SIZE: %d\n", asd, *size);
	(*size) = 2;
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente, t_log *logger, char *buffer)
{
	int size;
	buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

t_list *recibir_paquete(int socket_cliente)
{
	int size, tamanio;
	int desplazamiento = 0;
	void *buffer;
	t_list *valores = list_create();

	buffer = recibir_buffer(&size, socket_cliente);
	while (desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);
		char *valor = malloc(tamanio);
		memcpy(valor, buffer + desplazamiento, tamanio);
		desplazamiento += tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

uint32_t hacer_handshake_servidor(int socket_cliente, t_log *logger)
{
	log_info(logger, "Servidor: Empezando el handshake con el cliente.");
	uint32_t handshake = 0;
	uint32_t resultOk = 0;
	uint32_t resultError = -1;
	log_info(logger, "Servidor: Esperando el handshake.");
	recv(socket_cliente, &handshake, sizeof(uint32_t), MSG_WAITALL);
	if (handshake == 1)
	{
		log_info(logger, "Servidor: Handshake exitoso.");
		send(socket_cliente, &resultOk, sizeof(uint32_t), NULL);
	}
	else
	{
		log_error(logger, "Servidor: Error al querer hacer el handshake con el cliente.");
		send(socket_cliente, &resultError, sizeof(uint32_t), NULL);
		close(socket_cliente);
	}
	return handshake;
}

int recibir_todo(int socket_cliente, char **buffer, t_list *valores)
{
	int cod_op;
	int desplazamiento = recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL);
	if (cod_op != MENSAJE && cod_op != PAQUETE)
	{
		printf("Error al recibir el paquete, codigo operacion: %d\n", cod_op);
		return 0;
	}
	printf("CODIGO DE OPERACION: %d | CANT BYTES RECV: %d \n", cod_op, desplazamiento);
	int size;
	desplazamiento += recv(socket_cliente, &size, sizeof(int), MSG_WAITALL);
	printf("SIZE DEL SIZE: %d | SIZE: %d\n", desplazamiento, size);
	void *aux = malloc(size);
	desplazamiento += recv(socket_cliente, aux, size, MSG_WAITALL);
	printf("SIZE DEL BUFFER: %d | MSJ: %s\n", desplazamiento, (char *)aux);

	if (cod_op == PAQUETE)
	{
		printf("ENTRE AL COSO DEL PAQUETE! |%d\n", desplazamiento);
		int tamanio;
		desplazamiento = 0;
		while (desplazamiento < size)
		{
			memcpy(&tamanio, aux + desplazamiento, sizeof(int));
			desplazamiento += sizeof(int); // consigo el size del paquetito
			int valor;
			memcpy(&valor, aux + desplazamiento, tamanio); // consigo el buffer del paquetito
			desplazamiento += tamanio;
			list_add(valores, valor);
			printf("VALOOOR: %d\n", valor);
		}
		printf("SIZE LISTA: %d\n", list_size(valores));
	}
	else
	{
		*buffer = string_duplicate((char *)aux);
	}
	free(aux);
	return desplazamiento;
}