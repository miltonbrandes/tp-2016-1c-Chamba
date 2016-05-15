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
//int recibirDatos(int socket, char** datos, char* tamañoOcupa);

typedef enum  { CONSOLA=1, CPU=2, NUCLEO=3, SWAP=4, UMC=5 } quien_conecto;
enum  {ENVIAR_MENSAJE_CONSOLA=1} operaciones_consola;
enum  {ENVIAR_MENSAJE_NUCLEO=1} operaciones_nucleo;
enum  {ENVIAR_MENSAJE_CPU=1} operaciones_cpu;
enum  {ENVIAR_MENSAJE_UMC=1} operaciones_umc;
enum  {ENVIAR_MENSAJE_SWAP=1} operaciones_swap;

typedef struct {
	int quienConecto;
	int operacion;
	int longitudPayload;
} t_header;

typedef struct {
	t_header* header;
	char* payload;
} t_paquete;


//int escribir(int socket, int id, int longitud, int operacion, void* payload);
//int leer(int socket, int* id, char** payload);

int finalizarConexion(int socket);


#endif /* LIBRERIAS_SOCKETSESCRITURALECTURA_H_ */
