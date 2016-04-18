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
	struct sockaddr_in informacionSocket;
	int socketServidor;
	//creo el socket AFINET(ipv4), SOCKSTREAM(tcp, conexion a internet), 0 (protocolo por defecto)

	socketServidor = socket(AF_INET, SOCK_STREAM, 0);
	if(socketServidor == -1)
		return -1;

	//ahora empiezo a escuchar la conexion de sockets cliente
	informacionSocket.sin_family

	return socketServidor;
}






//se le pasa un socket servidor y acepto si hay una conexion cliente
//es un socket afInet
//devuelvo el descriptor al socket o -1 si hay error




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

