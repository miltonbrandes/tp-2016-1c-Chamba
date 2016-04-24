#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <string.h>
#define MAX_BUFFER_SIZE 4096
int leer(int socket, char* buffer, int longitud);
int escribir(int socket, char* buffer, int longitud);


//recibo datos de un socket pasado por parametros
//Finaliza la conexion de un socket
int finalizarConexion(int socket) {
	close(socket);
	return 0;
}

int escribir(int socket, char** buf, int longitud) {
	//char* buffer = malloc(MAX_BUFFER_SIZE);
	//memcpy(buffer, *buf, longitud);
	int escrito = 0, aux = 0;
	/*
		escribimos todos los caracteres que nos hayan pasado
	 */
	while (escrito < longitud && escrito != -1) {
		aux = send(socket, (void*)buf + escrito, longitud - escrito, 0);
		if (aux > 0) {
			/*
			si funciono bien
			 */
			escrito = escrito + aux;
		} else {
			if (aux == 0)
				return escrito;
			else{
				printf("Error en la escritura");
				}
		}
	}
	//free(buffer);
	return escrito;
}

int leer(int socket, char* buffer, int longitud) {
	int leido = 0, aux = 0;
	/*
	 * Se recibe el buffer con los datos
	 * Mientras no hayamos leido todos los datos solicitados
	 */
	//implementar las variables con malloc y free
	//char* buf = malloc(longitud + 1);
	while (leido < longitud && leido != -1) {
		aux = recv(socket, (void*)buffer + leido, longitud - leido, 0);
		//implementar con malloc y buf
		//free(buf);
		if (aux > 0) {
			/*
			 * Si hemos conseguido leer datos, incrementamos la variable
			 * que contiene los datos leidos hasta el momento
			 */
			leido = leido + aux;
		} else {
			/*
			 si es 0 es que se cerro el socket y devolvemos lo leido hasta el momento
			 */
			if (aux == 0)
				break;
			if (aux == -1) {
				printf("Hubo un error en la lectura");
				leido = -1;
			}
		}
	}
	return leido;
}



