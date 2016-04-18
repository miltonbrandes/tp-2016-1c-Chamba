/*
 * socketsServidorFunciones.h
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */

#ifndef LIBRERIAS_SOCKETSSERVIDORFUNCIONES_H_
#define LIBRERIAS_SOCKETSSERVIDORFUNCIONES_H_

#include <sys/socket.h>
#include <stdint.h>
int AbrirSocketServidor(int puerto);
int AceptarConexionCliente(int socket);

#endif /* LIBRERIAS_SOCKETSSERVIDORFUNCIONES_H_ */
