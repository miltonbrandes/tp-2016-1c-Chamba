#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

/*
* Conecta con un servidor remoto a traves de socket INET
  y me devuelve el descriptor al socket creado.
*/
int AbrirConexion(char *ip, int puerto)
{
	struct sockaddr_in socket_info;
	int nuevoSocket;
	// Se carga informacion del socket
	socket_info.sin_family = AF_INET;
	socket_info.sin_addr.s_addr = inet_addr(ip);
	socket_info.sin_port = htons(puerto);

	// Crear un socket:
	// AF_INET, SOCK_STREM, 0
	nuevoSocket = socket (AF_INET, SOCK_STREAM, 0);
	if (nuevoSocket < 0)
		return -1;
	// Conectar el socket con la direccion 'socketInfo'.
	if (connect (nuevoSocket,(struct sockaddr *)&socket_info,sizeof (socket_info)) != 0)
	{
		return -1;
	}

	return nuevoSocket;
}
