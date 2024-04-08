// esto son solo ejemplos de servidor y cliente, no esta hecho para ser ejecutado
// conste que no solo el cliente es el q puede enviar, el server tmb puede, los sockets son bidireccionales
void ejemplo_servidor()
{
    t_log *logger = iniciar_logger("algo", "algo");
    t_config *config = config_create("algo.config");

    int socket_server = iniciar_servidor(config, "PUERTO");
    int socket_cliente = esperar_cliente(socket_server, logger); // aca esto se puede hacer para q cree un hilo cada vez q se meta un cliente para asi poder tener multiples clientes al mismo tiempo (pero en si no es necesario en el tp)

    int sigo_funcionando = 1;
    while (sigo_funcionando)
    {
        int operacion = recibir_operacion(socket_cliente, logger);

        switch (operacion) // MENSAJE y PAQUETE son del enum op_code de sockets.h
        {
        case MENSAJE:
            char *mensaje = recibir_mensaje(socket_cliente, logger);
            log_info(logger, "Recibi el mensaje: %s.", mensaje);
            // hago algo con el mensaje
            break;
        case PAQUETE:
            t_list *lista = list_create();
            lista = recibir_paquete(socket_cliente);
            log_info(logger, "Recibi un paquete.");
            // hago algo con el mensaje
            break;
        case EXIT: // indica desconeccion
            log_error(logger, "Se desconecto el cliente %d.", socket_cliente);
            sigo_funcionando = 0;
            break;
        default: // recibi algo q no es eso, vamos a suponer q es para terminar
            log_error(logger, "Recibi una operacion rara (%d), termino el servidor.", operacion);
            return EXIT_FAILURE;
        }
    }
    return 0; // EXIT_SUCCESS
}

void ejemplo_cliente()
{
    t_log *logger = iniciar_logger("algo", "algo");
    t_config config = config_create("algo.config");
    int cliente = crear_conexion(config, "IP", "PUERTO", logger);
    // una vez conectado mando mensaje o paquete cuando se me da la gana
    enviar_mensaje("HOLAAAAAAAAA", cliente, logger);
    // paquete
    t_paquete *p = crear_paquete();
    agregar_a_paquete(p, 44, sizeof(int));
    agregar_a_paquete(p, 124123, sizeof(int));
    agregar_a_paquete(p, 1, sizeof(int));
    enviar_paquete(p, cliente, logger);

    liberar_conexion(cliente);
}