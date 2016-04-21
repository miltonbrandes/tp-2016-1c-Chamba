/*
 * procesoCpu.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */
#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>

int crearSocketCliente(char* direccion,int puerto);

int main() {

	t_config* config;
	config = config_create("../ProcesoCpu/ProcesoCpu.txt");
	//leo del archivo de configuracion el puerto y el ip
	char *direccion = config_get_string_value(config, "IP");
	int puerto = config_get_int_value(config, "PUERTO");
	crearSocketCliente(direccion,puerto);

	return EXIT_SUCCESS;
}

int crearSocketCliente(char* direccion,int puerto){

	int socketConexion;
	socketConexion = AbrirConexion(direccion, puerto);
	if (socketConexion < 0) {
		//aca me deberia mostrar por log que hubo un error
		return -1;
		}

	char* buff = "Hola como estas? Soy Cpu\n";
	char* respuestaServidor;
	//aca se conecto con el nucleo

	if (escribir(socketConexion, buff, sizeof(buff) + 1) < 0) {
		//error, no encontro el servidor
		return -1;
		}
	//si pasa este if se conecta correctamente al socket servidor
	printf("Se conecto correctamente\n");
	//Respuesta del socket servidor
	if (leer(socketConexion, respuestaServidor,
		sizeof(respuestaServidor) + 1) < 0) {
		//no pude recibir nada del nucleo
		return -1;
		}

	printf("Recibi %s\n", respuestaServidor);
	finalizarConexion(socketConexion);
	return socketConexion;
}

