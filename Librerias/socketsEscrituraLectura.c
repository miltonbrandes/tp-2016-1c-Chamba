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

int leer(int socket, char* buffer, int longitud);
int escribir(int socket, char* buffer, int longitud);


//recibo datos de un socket pasado por parametros
int recibirDatos(int socket, char** datos, char* tamañoOcupa) {
	int leido = 0;
	int longitud = sizeof(uint32_t);
	char* buffer = malloc(longitud + 1);

	/*
	 * Comprobacion de que los parametros de entrada son correctos
	 */
	if ((socket < 0) || (buffer == NULL ))
		return -1;

	/* HEADER
	 * Se reciben primero los datos necesarios que dan informacion
	 * sobre el verdadero buffer a recibir
	 */
	if ((leido = leer(socket, buffer, longitud + sizeof(char))) > 0) {
		if (tamañoOcupa == NULL ) {
			memcpy(&longitud, buffer + 1, sizeof(uint32_t));
		} else {
			memcpy(tamañoOcupa, buffer, sizeof(char));
			memcpy(&longitud, buffer + 1, sizeof(uint32_t));
		}
		free(buffer);
		buffer = malloc(longitud);
	} else {
		free(buffer);
		return -1;
	}

	/*
	 * Se recibe el buffer con los datos
	 */
	if ((leido = leer(socket, buffer, longitud)) < 0) {
		free(buffer);
		return -1;
	}

	if (leido != longitud)
		printf("No se han podido leer todos los datos del socket!!");

	*datos = (char*) malloc(longitud);
	memcpy(*datos, buffer, longitud);
	free(buffer);

	/*
	 * Se devuelve el total de los caracteres leidos
	 */
	return leido;
}
/*Escribe dato en el socket cliente. Devuelve numero de bytes escritos,
* o -1 si hay error.
*/
int enviarDatos(int socket, char** datos, uint32_t tamanio, char op) {
	int escrito = 0;

	/*
	 * Comprobacion de los parametros de entrada
	 */
	if ((socket == -1) || (*datos == NULL )|| (tamanio < 1))return -1;

	/* HEADER
	 * Se envian primero los datos necesarios que dan informacion
	 * sobre el verdadero buffer a enviar
	 */
	char* buffer = malloc(tamanio + sizeof(char)+sizeof(uint32_t)); //1B de op y 4B de long
	memcpy(buffer, &op, sizeof(char));
	memcpy(buffer + sizeof(char), &tamanio, sizeof(uint32_t));
	memcpy(buffer + sizeof(char)+sizeof(uint32_t), *datos, tamanio);

	/*
	 * Envio de Buffer [tamanio B]
	 */
	escrito = escribir(socket, buffer, tamanio + sizeof(char)+sizeof(uint32_t));
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
		escribimos todos los caracteres que nos hayan pasado
	 */
	while (escrito < longitud && escrito != -1) {
		aux = send(socket, buffer + escrito, longitud - escrito, 0);
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
			}
		}
	}
	return leido;
}


