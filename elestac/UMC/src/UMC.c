/*
 * UMC.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <sockets/ServidorFunciones.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>

#define MAX_BUFFER_SIZE 4096

//Socket que recibe conexiones de Nucleo y CPU
int socketReceptorNucleoCPU;
int socketSwap;

//Archivo de Log
t_log* ptrLog;

//Variables de configuracion
int puertoTCPRecibirConexiones;
int puertoReceptorSwap;
char *ipSwap;

//Metodos para Iniciar valores de la UMC
int crearLog() {
	ptrLog = log_create("../UMC/log.txt", "UMC", 1, 0);
	if (ptrLog) {
		return 1;
	} else {
		return 0;
	}
}

int iniciarUMC(t_config* config) {
	int marcos, marcosSize, marcoXProc, entradasTLB, retardo;

	if (config_has_property(config, "MARCOS")) {
		marcos = config_get_int_value(config, "MARCOS");
	} else {
		log_info(ptrLog,
				"El archivo de configuracion no contiene la clave MARCOS");
		return 0;
	}

	if (config_has_property(config, "MARCOS_SIZE")) {
		marcosSize = config_get_int_value(config, "MARCOS_SIZE");
	} else {
		log_info(ptrLog,
				"El archivo de configuracion no contiene la clave MARCOS_SIZE");
		return 0;
	}

	if (config_has_property(config, "MARCO_X_PROC")) {
		marcoXProc = config_get_int_value(config, "MARCO_X_PROC");
	} else {
		log_info(ptrLog,
				"El archivo de configuracion no contiene la clave MARCO_X_PROC");
		return 0;
	}

	if (config_has_property(config, "ENTRADAS_TLB")) {
		entradasTLB = config_get_int_value(config, "ENTRADAS_TLB");
	} else {
		log_info(ptrLog,
				"El archivo de configuracion no contiene la clave ENTRADAS_TLB");
		return 0;
	}

	if (config_has_property(config, "RETARDO")) {
		retardo = config_get_int_value(config, "RETARDO");
	} else {
		log_info(ptrLog,
				"El archivo de configuracion no contiene la clave RETARDO");
		return 0;
	}

	return 1;
}

int cargarValoresDeConfig() {
	t_config* config;
	config = config_create("../UMC/UMC.txt");

	if (config) {
		if (config_has_property(config, "PUERTO")) {
			puertoTCPRecibirConexiones = config_get_int_value(config, "PUERTO");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave PUERTO");
			return 0;
		}

		if (config_has_property(config, "IP_SWAP")) {
			ipSwap = config_get_string_value(config, "IP_SWAP");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave IP_SWAP");
			return 0;
		}

		if (config_has_property(config, "PUERTO_SWAP")) {
			puertoReceptorSwap = config_get_int_value(config, "PUERTO_SWAP");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave PUERTO_SWAP");
			return 0;
		}

		return iniciarUMC(config);
	} else {
		return 0;
	}
}

int init() {
	if (crearLog()) {
		return cargarValoresDeConfig();
	} else {
		return 0;
	}
}
//Fin Metodos para Iniciar valores de la UMC

void recibirDatos(fd_set *tempSockets, fd_set *sockets, int socketMaximo) {
	char *buffer;
	int bytesRecibidos;
	int socketFor;
	for (socketFor = 0; socketFor < (socketMaximo + 1); socketFor++) {
		if (FD_ISSET(socketFor, tempSockets)) {
			bytesRecibidos = leer(socket, buffer, MAX_BUFFER_SIZE);

			if (bytesRecibidos > 0) {
				buffer[bytesRecibidos] = 0;
				//Aca hacemos algo porque recibimos algo
			} else if (bytesRecibidos == 0) {
				finalizarConexion(socketFor);
				FD_CLR(socketFor, sockets);
				log_info(ptrLog, "No se recibio ningun byte de un socket que solicito conexion");
			}

		}
	}
}

void manejarConexionesRecibidas() {
	fd_set sockets, tempSockets;
	int resultadoSelect;
	int socketMaximo = socketReceptorNucleoCPU + 1;

	FD_ZERO(&sockets);
	FD_SET(socketReceptorNucleoCPU, &sockets);

	do {
		log_info(ptrLog, "Esperando conexiones");
		memcpy(&tempSockets, &sockets, sizeof(tempSockets));

		resultadoSelect = select(socketMaximo, &tempSockets, NULL, NULL, NULL);
		if (resultadoSelect == 0) {
			log_info(ptrLog, "Time out. Volviendo a esperar conexiones");
		} else if (resultadoSelect < 0) {
			log_info(ptrLog, "Ocurrio un error");
		} else {

			if (FD_ISSET(socketReceptorNucleoCPU, &tempSockets)) {

				int nuevoSocketConexion = AceptarConexionCliente(
						socketReceptorNucleoCPU);
				if (nuevoSocketConexion < 0) {
					log_info(ptrLog,
							"Ocurrio un error al intentar aceptar una conexion");
				} else {
					FD_SET(nuevoSocketConexion, &sockets);
					socketMaximo =
							(socketMaximo < nuevoSocketConexion) ?
									nuevoSocketConexion+1 : socketMaximo;
				}

				FD_CLR(socketReceptorNucleoCPU, &tempSockets);
			}

			recibirDatos(&tempSockets, &sockets, socketMaximo);

		}

	} while (1);
}

int main() {
	if (init()) {

		socketSwap = AbrirConexion(ipSwap, puertoReceptorSwap);
		if (socketSwap < 0) {
			log_info(ptrLog, "No pudo conectarse con Swap");
			return -1;
		}
		log_info(ptrLog, "Se conecto con Swap");

		socketReceptorNucleoCPU = AbrirSocketServidor(
				puertoTCPRecibirConexiones);
		if (socketReceptorNucleoCPU < 0) {
			log_info(ptrLog, "No se pudo abrir el Socket Servidor UMC");
			return -1;
		}
		log_info(ptrLog, "Se abrio el socket Servidor UMC");

		manejarConexionesRecibidas();

	} else {
		log_info(ptrLog, "La UMC no pudo inicializarse correctamente");
		return -1;
	}

	return EXIT_SUCCESS;
}
