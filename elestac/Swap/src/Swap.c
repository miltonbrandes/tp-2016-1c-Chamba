/*
 * Swap.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <sockets/ServidorFunciones.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <sockets/EscrituraLectura.h>

#define MAX_BUFFER_SIZE 4096
//Socket que recibe conexiones de Nucleo y CPU
int socketReceptorUMC;

//Archivo de Log
t_log* ptrLog;



//Variables de configuracion
int puertoTCPRecibirConexiones;
int retardoCompactacion;

//Metodos para Iniciar valores de la UMC
int crearLog() {
	ptrLog = log_create(getenv("SWAP_LOG"), "Swap", 1, 0);
	if (ptrLog) {
		return 1;
	} else {
		log_info(ptrLog,
				"Error al crear Log");
		return 0;
	}
}

int cargarValoresDeConfig() {
	t_config* config;
	config = config_create(getenv("SWAP_CONFIG"));

	int tamanoPagina, cantidadPaginas;

	if (config) {
		if (config_has_property(config, "PUERTO_ESCUCHA")) {
			puertoTCPRecibirConexiones = config_get_int_value(config,
					"PUERTO_ESCUCHA");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave PUERTO_ESCUCHA");
			return 0;
		}

		if (config_has_property(config, "NOMBRE_SWAP")) {
			char* nombreSwap = config_get_string_value(config, "NOMBRE_SWAP");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave NOMBRE_SWAP");
			return 0;
		}

		if (config_has_property(config, "CANTIDAD_PAGINAS")) {
			cantidadPaginas = config_get_int_value(config,
					"CANTIDAD_PAGINAS");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave CANTIDAD_PAGINAS");
			return 0;
		}

		if (config_has_property(config, "TAMANO_PAGINA")) {
			tamanoPagina = config_get_int_value(config, "TAMANO_PAGINA");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave TAMANO_PAGINA");
			return 0;
		}

		if (config_has_property(config, "RETARDO_COMPACTACION")) {
			retardoCompactacion = config_get_int_value(config,
					"RETARDO_COMPACTACION");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave RETARDO_COMPACTACION");
			return 0;
		}
	} else {
		log_info(ptrLog,
				"Error al crear Archivo de Configuracion");
		return 0;
	}

	return 1;
}

int init() {
	if (crearLog()) {
		return cargarValoresDeConfig();
	} else {
		return 0;
	}
}
//Fin Metodos para Iniciar valores de la UMC

void manejarConexionesRecibidas(int socketUMC) {
	do {
		log_info(ptrLog, "Esperando recibir alguna peticion");
		char buffer[MAX_BUFFER_SIZE];
		int bytesRecibidos = leer(socketUMC, buffer, MAX_BUFFER_SIZE);

		if(bytesRecibidos < 0) {
			log_info(ptrLog, "Ocurrio un error al Leer datos de UMC\0");
			finalizarConexion(socketUMC);
			return;
		}else if(bytesRecibidos == 0) {
			log_info(ptrLog, "No se recibio ningun byte de UMC");
			//Aca matamos Socket UMC
			finalizarConexion(socketUMC);
			return;
		}else{
			log_info(ptrLog, buffer);
			//Envio algo a UMC esperando con un retardo de compactacion
			log_info(ptrLog, "Esperando Compactacion");
			sleep(retardoCompactacion);
			char mensajeUMC[MAX_BUFFER_SIZE] = "Toma la pagina que te pidio nucleo\0";
			int sendBytes = escribir(socketUMC, mensajeUMC, MAX_BUFFER_SIZE);
			log_info(ptrLog, "Pagina enviada");
		}

	}while(1);
}

int main() {

	if (init()) {

		socketReceptorUMC = AbrirSocketServidor(puertoTCPRecibirConexiones);
		if (socketReceptorUMC < 0) {
			log_info(ptrLog, "No se pudo abrir el Socket Servidor Swap");
			return -1;
		}
		log_info(ptrLog, "Se abrio el socket Servidor Swap");

		do {
			log_info(ptrLog, "Esperando conexiones");
			int socketUMC = AceptarConexionCliente(socketReceptorUMC);
			log_info(ptrLog, "Conexion de UMC aceptada");
			manejarConexionesRecibidas(socketUMC);
		}while(1);

	} else {
		log_info(ptrLog, "El SWAP no pudo inicializarse correctamente");
		return -1;
	}

	return EXIT_SUCCESS;
}
