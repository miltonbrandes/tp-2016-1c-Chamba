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

#define MAX_BUFFER_SIZE 4096

//recibo datos de un socket pasado por parametros
//Finaliza la conexion de un socket
int finalizarConexion(int socket) {
	close(socket);
	return 0;
}

int escribir(int socket, int id, int longitud, int operacion, void* payload) {
	int ret;
		t_header header;
		header.quienConecto = id;
		header.longitudPayload = longitud;
		header.operacion = operacion;

		char* buff = malloc(longitud + sizeof(header));
		memcpy(buff, &header, sizeof(t_header));
		memcpy(buff + sizeof(t_header), payload, longitud);

		if ((ret = send(socket, buff, sizeof(t_header) + longitud, 0)) < 0) {
			perror("Error en el send del paquete");
			exit(1);
		}
		free(buff);

		return ret;
	}

int leer(int socket, int* id, char** payload) {
	t_header header;
	int nbytes;
		//*payload = NULL; //preguntar porque tengo que inicializar esto en NULL

		if ((nbytes = recv(socket, &header, sizeof(header), 0)) <= 0) {
			// error o conexion cerrada por el cliente
			if (nbytes == 0) {
				// conexion cerrada
				printf("Socket %d se murio\n", socket);
			} else {
				perror("Error en el recv del paquete");
				exit(1);
			}
			close(socket); // bye!
		} else {
			*id = header.quienConecto;
			*payload = malloc(header.longitudPayload);
			if (recv(socket, *payload, header.longitudPayload, 0) > 0) {
				//printf("furiosos: paquete recibido de %d [%d|%d|%s]\n", socket, head.id,head.length, *payload);
			} else {
				perror("Error al leer el payload ");
				exit(1);
			}
		}
		/**
		 * el free del segemento que reservo con malloc, buff, no se va a poder liberar aca.
		 * Se va a tener que liberar afuera de esta funcion, por ejemplo:
		 *
		 * 		int id;
		 * 		char* payload;
		 * 		recibir(soket,&id,&payload);
		 *
		 * 		free(payload);
		 *
		 */
		//free(buff);
		return nbytes;
}



