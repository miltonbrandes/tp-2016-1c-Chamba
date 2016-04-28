
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
int *listaConsolas;
int *listaCpus;
int i = 0;
int j = 0;
//t_list* listaSocketsConsola;
//t_list* listaSocketsCPUs;

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
int enviarMensajeACpu(socket)
{
	char *buffer[MAX_BUFFER_SIZE];
	*buffer = "estoy recibiendo una nueva consola";
	log_info(ptrLog, buffer);
	int bytesEnviados = escribir(socket, buffer, MAX_BUFFER_SIZE);
	if(bytesEnviados < 0) {
			log_info(ptrLog, "Ocurrio un error al enviar mensajes a CPU");
			return -1;
		}else if(bytesEnviados == 0){
			log_info(ptrLog, "No se envio ningun byte a CPU");
		}else{
			log_info(ptrLog, "Mensaje a CPU enviado");
		}
		return 1;
}


int enviarMensajeAUMC(char buffer[]) {
	log_info(ptrLog, buffer);
	int bytesEnviados = escribir(socketUMC, buffer, MAX_BUFFER_SIZE);
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

void recibirDatos(fd_set* tempSockets, fd_set* sockets, int socketMaximo) {
	char *buffer[MAX_BUFFER_SIZE];
	int bytesRecibidos;
	int socketFor;
	for (socketFor = 0; socketFor < (socketMaximo + 1); socketFor++) {
		if (FD_ISSET(socketFor, tempSockets)) {
			bytesRecibidos = leer(socketFor, buffer, MAX_BUFFER_SIZE);

			if (bytesRecibidos > 0) {
				buffer[bytesRecibidos] = 0;
				log_info(ptrLog, "Mensaje recibido: ", buffer);

				//Esta parte esta para que UMC mande mensaje a Swap, solo para probar la funcionalidad.
				//Hay que ver bien que hacer cuando se recibe un paquete

				//Fin

			} else if (bytesRecibidos == 0) {
				//Ver que hay que hacer porque puede venir de CPU, de Consola, o de UMC
				finalizarConexion(socketFor);
				FD_CLR(socketFor, tempSockets);
				FD_CLR(socketFor, sockets);
				log_info(ptrLog, "No se recibio ningun byte de un socket que solicito conexion.");
			}else if (bytesRecibidos < 0){
				finalizarConexion(socketFor);
				FD_CLR(socketFor, sockets);
				log_info(ptrLog, "Ocurrio un error al recibir los bytes de un socket");
			}

		}
	}
}

int obtenerSocketMaximoInicial() {
	int socketMaximoInicial = 0;

	if(socketReceptorCPU > socketReceptorConsola) {
		if(socketReceptorCPU > socketUMC) {
			socketMaximoInicial = socketReceptorCPU;
		}else if(socketUMC > socketReceptorCPU) {
			socketMaximoInicial = socketUMC;
		}
	}else if(socketReceptorConsola > socketUMC) {
		socketMaximoInicial = socketReceptorConsola;
	} else {
		socketMaximoInicial = socketUMC;
	}

	return socketMaximoInicial;
}

void datosEnSocketReceptorCPU(int nuevoSocketConexion) {
	char *buffer[MAX_BUFFER_SIZE];
	int bytesRecibidos = leer(nuevoSocketConexion, buffer, MAX_BUFFER_SIZE);

	if(bytesRecibidos < 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos en un Socket CPU");
	} else if(bytesRecibidos == 0) {
		log_info(ptrLog, "No se recibieron datos en el Socket CPU");
	} else {
		log_info(ptrLog, "Bytes recibidos desde una CPU: ", buffer);
		char mensajeParaCPU[MAX_BUFFER_SIZE] = "Este es un mensaje para vos, CPU\0";
		int bytesEnviados = escribir(nuevoSocketConexion, mensajeParaCPU, MAX_BUFFER_SIZE);
	}
}

void datosEnSocketReceptorConsola(int nuevoSocketConexion) {
	char *buffer[MAX_BUFFER_SIZE];
	int bytesRecibidos = leer(nuevoSocketConexion, buffer, MAX_BUFFER_SIZE);

	if(bytesRecibidos < 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos en un Socket Consola");
	} else if(bytesRecibidos == 0) {
		log_info(ptrLog, "No se recibieron datos en el Socket Consola");
	} else {
		log_info(ptrLog, "Bytes recibidos desde una Consola: ", buffer);
		*buffer = "Este es un mensaje para vos, Consola\0";
		int bytesEnviados = escribir(nuevoSocketConexion, buffer, MAX_BUFFER_SIZE);
	}

}

int datosEnSocketUMC() {
	char buffer[MAX_BUFFER_SIZE];
	int bytesRecibidos = leer(socketUMC, buffer, MAX_BUFFER_SIZE);

	if(bytesRecibidos < 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos en Socket UMC");
	} else if(bytesRecibidos == 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos en Socket UMC");
		finalizarConexion(socketUMC);
		return -1;
	} else {
		log_info(ptrLog, "Bytes recibidos desde UMC: ", buffer);
	}

	return 0;
}

void escucharPuertos() {
	fd_set sockets, tempSockets; //descriptores
	int socketMaximo = obtenerSocketMaximoInicial(); //descriptor mas grande

	//listaSocketsCPUs = list_create();
	//listaSocketsConsola = list_create();

	FD_SET(socketReceptorCPU, &sockets);
	FD_SET(socketReceptorConsola, &sockets);
	FD_SET(socketUMC, &sockets);

	while(1)
	{
		FD_ZERO(&tempSockets);
		memcpy(&tempSockets, &sockets, sizeof(sockets));

		log_info(ptrLog, "Esperando conexiones");

		int resultadoSelect = select(socketMaximo+1, &tempSockets, NULL, NULL, NULL);
		if (resultadoSelect == 0) {
			log_info(ptrLog, "Time out. Volviendo a esperar conexiones");
		} else if (resultadoSelect < 0) {
			log_info(ptrLog, "Ocurrio un error");
		} else {

			if (FD_ISSET(socketReceptorCPU, &tempSockets)) {

				int nuevoSocketConexion = AceptarConexionCliente(socketReceptorCPU);
				if (nuevoSocketConexion < 0) {
					log_info(ptrLog, "Ocurrio un error al intentar aceptar una conexion de CPU");
				} else {
					FD_SET(nuevoSocketConexion, &sockets);
					FD_SET(nuevoSocketConexion, &tempSockets);
					socketMaximo = (socketMaximo < nuevoSocketConexion) ? nuevoSocketConexion : socketMaximo;
				}
				listaCpus[i] = nuevoSocketConexion;
				i++;
				FD_CLR(socketReceptorCPU, &tempSockets);
				datosEnSocketReceptorCPU(nuevoSocketConexion);

			} else if(FD_ISSET(socketReceptorConsola, &tempSockets)) {

				int nuevoSocketConexion = AceptarConexionCliente(socketReceptorConsola);
				if (nuevoSocketConexion < 0) {
					log_info(ptrLog, "Ocurrio un error al intentar aceptar una conexion de Consola");
				} else {
					FD_SET(nuevoSocketConexion, &sockets);
					FD_SET(nuevoSocketConexion, &tempSockets);
					socketMaximo = (socketMaximo < nuevoSocketConexion) ? nuevoSocketConexion : socketMaximo;
				}
				listaConsolas[j] = nuevoSocketConexion; j++;

				FD_CLR(socketReceptorConsola, &tempSockets);
				datosEnSocketReceptorConsola(nuevoSocketConexion);
				enviarMensajeACpu(listaCpus[i-1]);

			} else if(FD_ISSET(socketUMC, &tempSockets)) {

				//Ver como es aca porque no estamos aceptando una conexion cliente, sino recibiendo algo de UMC
				int returnDeUMC = datosEnSocketUMC();
				if(returnDeUMC == -1) {
					//Aca matamos Socket UMC
					FD_CLR(socketUMC, &sockets);
				}

			} else{

				//Ver que hacer aca, se esta recibiendo algo de un socket en particular
				recibirDatos(&tempSockets, &sockets, socketMaximo);

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

		socketReceptorConsola = AbrirSocketServidor(puertoReceptorConsola);
		if (socketReceptorCPU < 0) {
			log_info(ptrLog, "No se pudo abrir el Socket Servidor Consola");
			return -1;
		}
		log_info(ptrLog, "Se abrio el Socket Servidor Consola");

		returnInt = enviarMensajeAUMC("Mensaje desde Nucleo\0");
		listaConsolas = (int *)malloc(10*sizeof(int));
		listaCpus = (int *)malloc(10*sizeof(int));
		escucharPuertos();


	} else {
		log_info(ptrLog, "El Nucleo no pudo inicializarse correctamente");
		return -1;
	}

	finalizarConexion(socketUMC);
	finalizarConexion(socketReceptorConsola);
	finalizarConexion(socketReceptorCPU);

	return returnInt;

}
