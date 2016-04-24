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

#define MAX_BUFFER_SIZE 4096

t_log* ptrLog;
t_config* config;

int crearLog(){
	ptrLog = log_create("../ProcesoCpu/log.txt", "ProcesoCpu", 1, 0);
		if(ptrLog){
			 return 1;
		 }else{
			 return 0;
		}
}

int crearSocketCliente(char* direccion,int puerto);

int main() {

	t_config* config;
	config = config_create("../ProcesoCpu/ProcesoCpu.txt");
	//leo del archivo de configuracion el puerto y el ip
	char *direccionUmc = config_get_string_value(config, "IP_UMC");
	int puertoUmc = config_get_int_value(config, "PUERTO_UMC");
	crearSocketCliente(direccionUmc,puertoUmc);

	char *direccionNucleo = config_get_string_value(config, "IP_NUCLEO");
	int puertoNucleo = config_get_int_value(config, "PUERTO_NUCLEO");
	crearSocketCliente(direccionNucleo,puertoNucleo);

	return EXIT_SUCCESS;
}

int crearSocketCliente(char* direccion,int puerto){

	int socketConexion;
	socketConexion = AbrirConexion(direccion, puerto);
	if (socketConexion < 0) {
		//aca me deberia mostrar por log que hubo un error
		log_info(ptrLog, "Error en la conexion con el nucleo");
		return -1;
	}

	char* buff = "Hola como estas? Soy Cpu\n";
	char* respuestaServidor;
	log_info(ptrLog, "Se conecto con el nucleo");
	//aca se conecto con el nucleo/Umc

	if (escribir(socketConexion, buff, sizeof(buff) + 1) < 0) {
		//error, no pudo escribir
		return -1;
		}
	//si pasa este if se conecta correctamente al socket servidor
	printf("Se conecto correctamente\n");
	log_info(ptrLog, "Mensaje Enviado al servidor");

	//Respuesta del socket servidor
	int bytesRecibidos = leer(socketConexion, respuestaServidor, sizeof(respuestaServidor)+1);
	if (bytesRecibidos < 0) {
		log_info(ptrLog,"Error en al lectura del mensaje del servidor");
		//no pude recibir nada del nucleo/umc
		return -1;
		}

	log_info(ptrLog, respuestaServidor);
	//free(respuestaServidor);

	printf("Recibi %s\n", respuestaServidor);
	finalizarConexion(socketConexion);
	return socketConexion;
}

