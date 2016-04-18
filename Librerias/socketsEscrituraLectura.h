/*
 * socketsEscrituraLectura.h
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */

#ifndef LIBRERIAS_SOCKETSESCRITURALECTURA_H_
#define LIBRERIAS_SOCKETSESCRITURALECTURA_H_
#include <sys/socket.h>
#include <stdint.h>
int recibirDatos(int socket, char** datos, char* tamañoOcupa);
int leer(int socket, char* buffer, int longitud);
int escribir(int socket, char* buffer, int longitud);

int finalizarConexion(int socket);
#endif /* LIBRERIAS_SOCKETSESCRITURALECTURA_H_ */
