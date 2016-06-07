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
#include "EscrituraLectura.h"
#include "OpsUtiles.h"
#include "StructsUtiles.h"

/*
 * Lee datos del socket. Devuelve el numero de bytes leidos o
 * 0 si se cierra fichero o -1 si hay error.
 */

char* recibirDatos(int socket, uint32_t ** op, uint32_t ** id) {
	int leido = 0;
	uint32_t longitud = 3 * sizeof(uint32_t);
	char* buffer = malloc(longitud);
	/*
	 * Comprobacion de que los parametros de entrada son correctos
	 */
	if ((socket < 0) || (buffer == NULL))
		return "ERROR";

	/* HEADER
	 * Se reciben primero los datos necesarios que dan informacion
	 * sobre el verdadero buffer a recibir
	 */
	if ((leido = leer(socket, buffer, longitud)) > 0) {
		if (op == NULL) {
			memcpy(id, buffer + sizeof(uint32_t), sizeof(uint32_t));
			memcpy(&longitud, buffer + (2 * sizeof(uint32_t)), sizeof(uint32_t));
		} else {
			memcpy(op, buffer, sizeof(uint32_t));
			memcpy(id, buffer + sizeof(uint32_t), sizeof(uint32_t));
			memcpy(&longitud, buffer + (2 * sizeof(uint32_t)), sizeof(uint32_t));
		}
		free(buffer);
		buffer = malloc(longitud);
	} else {
		free(buffer);
		return "ERROR";
	}

	if ((leido = leer(socket, buffer, longitud)) < 0) {
		free(buffer);
		return "ERROR";
	}

	if (leido != longitud)
		printf("No se han podido leer todos los datos del socket!!");

	return buffer;
}

/*
 * Escribe dato en el socket cliente. Devuelve numero de bytes escritos,
 * o -1 si hay error.
 */
int enviarDatos(int socket, char* datos, uint32_t tamanio, uint32_t op, uint32_t id) {
	int escrito = 0;

	/*
	 * Comprobacion de los parametros de entrada
	 */
	if ((socket == -1) || (tamanio < 1))
		return -1;

	/* HEADER
	 * Se envian primero los datos necesarios que dan informacion
	 * sobre el verdadero buffer a enviar
	 */
	char* buffer = malloc(tamanio + (3 * sizeof(uint32_t))); //1B de op y 4B de long
	memcpy(buffer, &op, sizeof(uint32_t));
	memcpy(buffer + sizeof(uint32_t), &id, sizeof(uint32_t));
	memcpy(buffer + (2 * sizeof(uint32_t)), &tamanio, sizeof(uint32_t));
	memcpy(buffer + (3 * sizeof(uint32_t)), datos, tamanio);

	/*
	 * Envio de Buffer [tamanio B]
	 */
	escrito = escribir(socket, buffer, tamanio + (3 * sizeof(uint32_t)));
	free(buffer);

	/*
	 * Devolvemos el total de caracteres leidos
	 */
	return escrito;
}

//Finaliza la conexion de un socket
int finalizarConexion(int socket) {
	close(socket);
	return 0;
}

int escribir(int socket, char* buffer, int longitud) {
	int escrito = 0, aux = 0;
	/*
	 * Bucle hasta que hayamos escrito todos los caracteres que nos han
	 * indicado.
	 */
	while (escrito < longitud && escrito != -1) {
		aux = send(socket, buffer + escrito, longitud - escrito, 0);
		if (aux > 0) {
			/*
			 * Si hemos conseguido escribir caracteres, se actualiza la
			 * variable Escrito
			 */
			escrito = escrito + aux;
		} else {
			/*
			 * Si se ha cerrado el socket, devolvemos el numero de caracteres
			 * leidos.
			 * Si ha habido error, devolvemos -1
			 */
			if (aux == 0)
				return escrito;
			else
				switch (errno) {
				case EINTR:
				case EAGAIN:
					usleep(100);
					break;
				default:
					escrito = -1;
				}
		}
	}
	return escrito;
}

int leer(int socket, char* buffer, int longitud) {
	int leido = 0, aux = 0;
	/*
	 * Se recibe el buffer con los datos
	 * Mientras no hayamos leido todos los datos solicitados
	 */
	while (leido < longitud && leido != -1) {
		aux = recv(socket, buffer + leido, longitud - leido, 0);
		if (aux > 0) {
			leido = leido + aux;
		} else {
			if (aux == 0)
				break;
			if (aux == -1) {
				switch (errno) {
				case EINTR:
				case EAGAIN:
					usleep(100);
					break;
				default:
					leido = -1;
				}
			}
		}
	}
	return leido;
}

