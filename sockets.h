#ifndef SOCKETS_UTILS_H_
#define SOCKETS_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
// #include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>

// shared

typedef enum
{
	MENSAJE,
	PAQUETE
} op_code;

typedef struct
{
	int size;
	void *stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer *buffer;
} t_paquete;

// client
int crear_conexion(t_config *config, char *key_ip, char *key_puerto, t_log *logger);
void enviar_mensaje(char *mensaje, int socket_cliente);
t_paquete *crear_paquete(void);
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
void enviar_paquete(t_paquete *paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete *paquete);

// server

void *recibir_buffer(int *, int);

int iniciar_servidor(t_config *config, char *key_puerto);
int esperar_cliente(int);
t_list *recibir_paquete(int);
void recibir_mensaje(int, t_log *logger, char *buffer);
int recibir_operacion(int);
uint32_t hacer_handshake_servidor(int socket_cliente, t_log *logger);
int recibir_todo(int socket_cliente, char **buffer, t_list *valores);

#define MENSAJE 0
#define PAQUETE 1
#define DESCONEXION -1

#endif /* SOCKETS_UTILS_H_ */
