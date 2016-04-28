//funciones para abrir sockets de servidor
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
int AbrirSocketServidor(int puerto);
int AceptarConexionCliente(int socket);

//abro un servidor de tipo AF_INET y me devuelve descriptor del socket servidor o -1 si hay error
int AbrirSocketServidor(int puerto)
{
	struct sockaddr_in socketInfo;
		int socketEscucha;
		int optval = 1;

		// Crear un socket
		socketEscucha = socket (AF_INET, SOCK_STREAM, 0);
		if (socketEscucha == -1)
		 	return -1;

		setsockopt(socketEscucha, SOL_SOCKET, SO_REUSEADDR, &optval,
				sizeof(optval));
		socketInfo.sin_family = AF_INET;
		socketInfo.sin_port = htons(puerto);
		socketInfo.sin_addr.s_addr = INADDR_ANY;
		if (bind (socketEscucha,(struct sockaddr *)&socketInfo,sizeof (socketInfo)) != 0)
		{
			close (socketEscucha);
			return -1;
		}

		/*
		* Se avisa al sistema que comience a atender llamadas de clientes
		*/
		if (listen (socketEscucha, 10) == -1)
		{
			close (socketEscucha);
			return -1;
		}
		/*
		* Se devuelve el descriptor del socket servidor
		*/
		return socketEscucha;
	}

int AceptarConexionCliente(int socket)
{
	socklen_t longitudCliente;//esta variable tiene inicialmente el tamaño de la estructura cliente que se le pase
	struct sockaddr cliente;
	int socketNuevaConexion;//esta variable va a tener la descripcion del nuevo socket que estaria creando
	longitudCliente = sizeof(cliente);
	socketNuevaConexion = accept (socket, &cliente, &longitudCliente);//acepto la conexion del cliente
	if (socketNuevaConexion < 0)
		return -1;

	return socketNuevaConexion;

}

