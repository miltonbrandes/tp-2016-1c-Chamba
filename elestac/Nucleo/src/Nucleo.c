
/*

 * nucleo.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/string.h>
#include <sockets/ServidorFunciones.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>

#define MAX_BUFFER_SIZE 4096

typedef struct {
	char *nombre;
	int tiempoSleep;
} t_dispositivo_io;

typedef struct {
	char* nombre;
	int valor;
} t_semaforo;

typedef struct {
	char *nombre;
	int valor;
} t_variable_compartida;

//Socket que recibe conexiones de CPU y Consola
int socketReceptorCPU, socketReceptorConsola;
int socketUMC;
t_list* listaProgramasConsola;
//Archivo de Log
t_log* ptrLog;

//Variables de configuracion para establecer Conexiones
int puertoReceptorCPU, puertoReceptorConsola;
int puertoConexionUMC;
char *ipUMC;

//Variables propias del Nucleo
int quantum, quantumSleep;
t_list *listaDispositivosIO, *listaSemaforos, *listaVariablesCompartidas;

//Metodos para iniciar valores de Nucleo
int crearLog() {
	ptrLog = log_create("../Nucleo/log.txt", "Nucleo", 1, 0);
	if (ptrLog) {
		return 1;
	} else {
		return 0;
	}
}

void crearListaSemaforos(char **semIds, char **semInitValues) {
	listaSemaforos = list_create();
	int i;
	for (i = 0; semIds[i] != NULL; i++) {
		t_semaforo* semaforo = malloc(sizeof(t_semaforo));
		semaforo->nombre = semIds[i];
		semaforo->valor = atoi(semInitValues[i]);
		list_add(listaSemaforos, semaforo);
	}
}

void crearListaDispositivosIO(char **ioIds, char **ioSleepValues) {
	listaDispositivosIO = list_create();
	int i;
	for (i = 0; ioIds[i] != NULL; i++) {
		t_dispositivo_io* dispositivoIO = malloc(sizeof(t_dispositivo_io));
		dispositivoIO->nombre = ioIds[i];
		dispositivoIO->tiempoSleep = atoi(ioSleepValues[i]);
		list_add(listaDispositivosIO, dispositivoIO);
	}
}

void crearListaVariablesCompartidas(char **sharedVars) {
	listaVariablesCompartidas = list_create();
	int i;
	for (i = 0; sharedVars[i] != NULL; i++) {
		t_variable_compartida* variableCompartida = malloc(sizeof(t_semaforo));
		variableCompartida->nombre = sharedVars[i];
		variableCompartida->valor = 0;
		list_add(listaVariablesCompartidas, variableCompartida);
	}
}

int iniciarNucleo() {
	t_config* config;
	config = config_create("../Nucleo/Nucleo.txt");

	if (config) {
		if (config_has_property(config, "PUERTO_SERVIDOR_UMC")) {
			puertoConexionUMC = config_get_int_value(config,
					"PUERTO_SERVIDOR_UMC");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave PUERTO_SERVIDOR_UMC");
			return 0;
		}

		if (config_has_property(config, "IP_SERVIDOR_UMC")) {
			ipUMC = config_get_string_value(config, "IP_SERVIDOR_UMC");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave IP_SERVIDOR_UMC");
			return 0;
		}

		if (config_has_property(config, "PUERTO_PROG")) {
			puertoReceptorConsola = config_get_int_value(config, "PUERTO_PROG");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave PUERTO_PROG");
			return 0;
		}

		if (config_has_property(config, "PUERTO_CPU")) {
			puertoReceptorCPU = config_get_int_value(config, "PUERTO_CPU");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave PUERTO_CPU");
			return 0;
		}

		if (config_has_property(config, "QUANTUM")) {
			quantum = config_get_int_value(config, "QUANTUM");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave QUANTUM");
			return 0;
		}

		if (config_has_property(config, "QUANTUM_SLEEP")) {
			quantumSleep = config_get_int_value(config, "QUANTUM_SLEEP");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave QUANTUM_SLEEP");
			return 0;
		}

		if (config_has_property(config, "SEM_IDS")) {
			char** semIds = config_get_array_value(config, "SEM_IDS");
			if (config_has_property(config, "SEM_INIT")) {
				char ** semInitValues = config_get_array_value(config,
						"SEM_INIT");
				crearListaSemaforos(semIds, semInitValues);
			} else {
				log_info(ptrLog,
						"El archivo de configuracion no contiene la clave SEM_INIT");
				return 0;
			}
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave SEM_IDS");
			return 0;
		}

		if (config_has_property(config, "IO_IDS")) {
			char** ioIds = config_get_array_value(config, "IO_IDS");
			if (config_has_property(config, "IO_SLEEP")) {
				char ** ioSleepValues = config_get_array_value(config,
						"IO_SLEEP");
				crearListaDispositivosIO(ioIds, ioSleepValues);
			} else {
				log_info(ptrLog,
						"El archivo de configuracion no contiene la clave IO_SLEEP");
				return 0;
			}
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave IO_IDS");
			return 0;
		}

		if (config_has_property(config, "SHARED_VARS")) {
			char** sharedVarsIds = config_get_array_value(config,
					"SHARED_VARS");
			crearListaVariablesCompartidas(sharedVarsIds);
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave SHARED_VARS");
			return 0;
		}
	} else {
		return 0;
	}

	return 1;
}

int init() {
	if (crearLog()) {
		return iniciarNucleo();
	} else {
		return 0;
	}
}
//Fin Metodos para Iniciar valores de la UMC

int enviarMensajeAUMC() {
	char buffer[MAX_BUFFER_SIZE] = "Eh UMC contestame algo";
	log_info(ptrLog, buffer);
	int bytesEnviados = escribir(socketUMC, buffer, sizeof(buffer));
	if(bytesEnviados < 0) {
		log_info(ptrLog, "Ocurrio un error al enviar mensajes a UMC");
		return -1;
	}else if(bytesEnviados == 0){
		log_info(ptrLog, "No se envio ningun byte a UMC");
	}else{
		log_info(ptrLog, "Mensaje a UMC enviado");
	}
	return 1;
}


int escucharAUMC() {
	char buffer[MAX_BUFFER_SIZE];
	do {
		log_info(ptrLog, "Esperando respuesta de UMC");
		int bytesRecibidos = leer(socketUMC, buffer, MAX_BUFFER_SIZE);
		if(bytesRecibidos < 0) {
			log_info(ptrLog, "Ocurrio un error al recibir mensajes de UMC");
			return -1;
		}else if(bytesRecibidos == 0){
			log_info(ptrLog, "No se obtuvo respuesta de UMC y se cerro la conexion");
			return -1;
		}else{
			log_info(ptrLog, buffer);
		}
	}while(1);
}
int escucharAConsola(int socket) {
	char buffer[MAX_BUFFER_SIZE];
	do {
		log_info(ptrLog, "Esperando que me hable la consola");
		int bytesRecibidos = leer(socket, buffer, MAX_BUFFER_SIZE);
		if(bytesRecibidos < 0) {
			log_info(ptrLog, "Ocurrio un error al recibir mensajes de consola");
			return -1;
		}else if(bytesRecibidos == 0){
			log_info(ptrLog, "No se obtuvo nada de la consola y se cerro la conexion");
			return -1;
		}else{
			log_info(ptrLog, buffer);
		}
	}while(1);
}
int enviarMensajeAConsola(int socket) {
	char buffer[MAX_BUFFER_SIZE] = "consola aca terminas papa";
	log_info(ptrLog, buffer);
	int bytesEnviados = escribir(socket, buffer, sizeof(buffer));
	if(bytesEnviados < 0) {
		log_info(ptrLog, "Ocurrio un error al enviar mensajes a la consola");
		return -1;
	}else if(bytesEnviados == 0){
		log_info(ptrLog, "No se envio ningun byte a la consola");
	}else{
		log_info(ptrLog, "Mensaje a la consola enviado");
	}
	return 1;
}

void *comenzarEscucharConsolas()
{
	while(1)
	{
		fd_set sockets;	// Descriptores de interes para select()
		fd_set tempSockets; //descriptor master
		int maximo; //descriptor mas grande
		listaProgramasConsola = list_create();
		socketReceptorConsola = AbrirSocketServidor(puertoReceptorConsola);
		if (socketReceptorConsola < 0) {
			log_info(ptrLog, "No se pudo abrir el Socket Servidor Consola");
		}
		log_info(ptrLog, "Se abrio el Socket Servidor Consola");
		FD_ZERO(&sockets);
		FD_ZERO(&tempSockets);
		FD_SET(socketReceptorConsola, &tempSockets);
		maximo = socketReceptorConsola;
		log_info(ptrLog, "Esperando conexiones");
		//memcpy(&tempSockets, &sockets, sizeof(sockets));

		/*ahora espero que alguno de los que se conectan tenga algo para decir*/
		if(select(maximo +1, &sockets, NULL, NULL, NULL) < 0)
		{
			log_info(ptrLog, "Error en el select");
		}
		//ahora me fijo si me mandan algo
		int i = 0;
		for(i = 0; i <= maximo; i++)
		{
			if(FD_ISSET(i, &sockets))
			{
				int newfd;
				if(i == socketReceptorConsola)
				{
					newfd = AceptarConexionCliente(socketReceptorConsola);
					if(newfd == -1)
					{
						log_info(ptrLog, "Error al aceptar consola");

					}else
					{
						log_info(ptrLog, "Se conecto una consola!!!!");
						FD_SET(newfd, &tempSockets);
						FD_SET(newfd, &sockets);
						if(newfd > maximo)
						{
							maximo = newfd;
						}
					}
				}
				escucharAConsola(newfd);
				//aca respondo a la consola
				enviarMensajeAConsola(newfd);
				FD_CLR(i, &tempSockets);
				if(i == socketReceptorCPU)
				{
					//aca deberia estar la logica de conexion con cpu desde nucleo me parece no?
				}
			}

		}
	}
}
int main() {
	int returnInt = EXIT_SUCCESS;

	if (init()) {

		socketUMC = AbrirConexion(ipUMC, puertoConexionUMC);
		if (socketUMC < 0) {
			log_info(ptrLog, "No pudo conectarse con UMC");
			return -1;
		}
		log_info(ptrLog, "Se conecto con UMC");

		socketReceptorCPU = AbrirSocketServidor(puertoReceptorCPU);
		if (socketReceptorCPU < 0) {
			log_info(ptrLog, "No se pudo abrir el Socket Servidor CPU");
			return -1;
		}
		log_info(ptrLog, "Se abrio el Socket Servidor CPU");

		returnInt = enviarMensajeAUMC();
		if(returnInt) {
			returnInt = escucharAUMC();
		}
		comenzarEscucharConsolas();


	} else {
		log_info(ptrLog, "El Nucleo no pudo inicializarse correctamente");
		return -1;
	}

	finalizarConexion(socketUMC);
	finalizarConexion(socketReceptorConsola);
	finalizarConexion(socketReceptorCPU);

	return returnInt;

}
