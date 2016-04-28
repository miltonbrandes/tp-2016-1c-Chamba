

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


//TODO: falta que el umc reciba cuando hay un programa nuevo de cpu!!!!
//casi finalizado el circuito
#define MAX_BUFFER_SIZE 4096



//Socket que recibe conexiones de Nucleo y CPU
int socketReceptorNucleo;
int socketReceptorCPU;
int socketSwap;

//Archivo de Log
t_log* ptrLog;

//Variables de configuracion
int puertoTCPRecibirConexionesCPU;
int puertoTCPRecibirConexionesNucleo;
int puertoReceptorSwap;
char *ipSwap;
int *listaCpus;
int i = 0;
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
		if (config_has_property(config, "PUERTO_CPU")) {
			puertoTCPRecibirConexionesCPU = config_get_int_value(config, "PUERTO_CPU");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave PUERTO_CPU");
			return 0;
		}

		if (config_has_property(config, "PUERTO_NUCLE0")) {
			puertoTCPRecibirConexionesNucleo = config_get_int_value(config, "PUERTO_NUCLE0");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave PUERTO_NUCLE0");
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

void enviarMensajeASwap(char *mensajeSwap) {
	log_info(ptrLog, "Envio mensaje a Swap: ", mensajeSwap);
	int sendBytes = escribir(socketSwap, mensajeSwap, MAX_BUFFER_SIZE);
}

void enviarMensajeACPU(int socketCPU, char* buffer) {
	char mensajeCpu[MAX_BUFFER_SIZE] = "Le contesto a Cpu, soy UMC\0";
	log_info(ptrLog, "Envio mensaje a CPU: ", mensajeCpu);
	int sendBytes = escribir(socketCPU, mensajeCpu, MAX_BUFFER_SIZE);
}

void enviarMensajeANucleo(int socketNucleo, char* buffer) {
	char mensajeNucleo[MAX_BUFFER_SIZE] = "Le contesto a Nucleo, soy UMC\0";
	log_info(ptrLog, "Envio mensaje a Nucleo: ", mensajeNucleo);
	int sendBytes = escribir(socketNucleo, mensajeNucleo, MAX_BUFFER_SIZE);
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
				log_info(ptrLog, buffer);
				//Recibimos algo
				/*if(socketFor == socketReceptorNucleo)
				{

					enviarMensajeASwap("Llego un nuevo programa, tenes que reservar memoria");

				}*/
			} else if (bytesRecibidos == 0) {
				//Aca estamos matando el Socket que esta fallando
				//Ver que hay que hacer porque puede venir de CPU o de Nucleo
				finalizarConexion(socketFor);
				FD_CLR(socketFor, tempSockets);
				FD_CLR(socketFor, sockets);
				log_info(ptrLog, "No se recibio ningun byte de un socket que solicito conexion.");
			}else{
				finalizarConexion(socketFor);
				FD_CLR(socketFor, sockets);
				log_info(ptrLog, "Ocurrio un error al recibir los bytes de un socket");
			}

		}
	}
}

void datosEnSocketReceptorNucleoCPU(int socketNuevaConexion) {
	char buffer[MAX_BUFFER_SIZE];
	int bytesRecibidos = leer(socketNuevaConexion, buffer, MAX_BUFFER_SIZE);

	if(bytesRecibidos < 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos en un Socket Nucleo o CPU");
	} else if(bytesRecibidos == 0) {
		log_info(ptrLog, "No se recibieron datos en el Socket Nucleo o CPU");
	} else {
		log_info(ptrLog, "Bytes recibidos desde Nucleo o CPU: ", buffer);
		enviarMensajeASwap("Necesito que me reserves paginas, Soy la umc");
	}
}

int datosEnSocketSwap() {
	char buffer[MAX_BUFFER_SIZE];
	int bytesRecibidos = leer(socketSwap, buffer, MAX_BUFFER_SIZE);

	if(bytesRecibidos < 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos de Swap");
	} else if(bytesRecibidos == 0) {
		log_info(ptrLog, "No se recibieron datos de Swap. Se cierra la conexion");
		finalizarConexion(socketSwap);
		return -1;
	} else {
		log_info(ptrLog, "Bytes recibidos desde Swap: ", buffer);
	}

	return 0;
}

int obtenerSocketMaximoInicial() {
	int socketMaximoInicial = 0;

	if(socketReceptorCPU > socketReceptorNucleo) {
		if(socketReceptorCPU > socketSwap) {
			socketMaximoInicial = socketReceptorCPU;
		}else if(socketSwap > socketReceptorNucleo) {
			socketMaximoInicial = socketSwap;
		}
	}else if(socketReceptorNucleo > socketSwap) {
		socketMaximoInicial = socketReceptorNucleo;
	} else {
		socketMaximoInicial = socketSwap;
	}

	return socketMaximoInicial;
}


void manejarConexionesRecibidas() {
	int handshakeNucleo = 0;
	fd_set sockets, tempSockets;
	int resultadoSelect;
	int socketMaximo = obtenerSocketMaximoInicial();

	int socketCPUPosta = -121211;

	FD_SET(socketReceptorNucleo, &sockets);
	FD_SET(socketReceptorCPU, &sockets);
	FD_SET(socketSwap, &sockets);

	do {
		FD_ZERO(&tempSockets);
		log_info(ptrLog, "Esperando conexiones");
		memcpy(&tempSockets, &sockets, sizeof(sockets));

		resultadoSelect = select(socketMaximo+1, &tempSockets, NULL, NULL, NULL);
		if (resultadoSelect == 0) {
			log_info(ptrLog, "Time out. Volviendo a esperar conexiones");
		} else if (resultadoSelect < 0) {
			log_info(ptrLog, "Ocurrio un error");
		} else {

			if (FD_ISSET(socketReceptorNucleo, &tempSockets)) {

				int nuevoSocketConexion = AceptarConexionCliente(socketReceptorNucleo);
				if (nuevoSocketConexion < 0) {
					log_info(ptrLog, "Ocurrio un error al intentar aceptar una conexion de CPU o Nucleo");
				} else {
					FD_SET(nuevoSocketConexion, &sockets);
					FD_SET(nuevoSocketConexion, &tempSockets);
					socketMaximo = (socketMaximo < nuevoSocketConexion) ? nuevoSocketConexion : socketMaximo;
				}

				FD_CLR(socketReceptorNucleo, &tempSockets);
				enviarMensajeANucleo(nuevoSocketConexion, "Nucleo no me rompan las pelotas\0");
				//me fijo si el mensaje no es el handshake y si no es se lo tengo que pasar al swap


			} else if (FD_ISSET(socketReceptorCPU, &tempSockets)) {

				int nuevoSocketConexion = AceptarConexionCliente(socketReceptorCPU);
				if (nuevoSocketConexion < 0) {
					log_info(ptrLog, "Ocurrio un error al intentar aceptar una conexion de CPU");
				} else {
					FD_SET(nuevoSocketConexion, &sockets);
					FD_SET(nuevoSocketConexion, &tempSockets);
					socketMaximo = (socketMaximo < nuevoSocketConexion) ? nuevoSocketConexion : socketMaximo;
				}
				//aca deberia ir un mensaje hacia swap diciendo que cpu me esta pidiendo espacio, = que con el nucleo usar una lista para cpus!!!
				//agrego las cpus a una nueva lista
				listaCpus[i] = nuevoSocketConexion;
				socketCPUPosta = nuevoSocketConexion;
				i++;
				FD_CLR(socketReceptorCPU, &tempSockets);
				enviarMensajeACPU(nuevoSocketConexion, "CPU no me rompas las pelotas\0");

			}
			else if(FD_ISSET(socketSwap, &tempSockets)) {

				//Ver que hacer aca, no se acepta conexion, se recibe algo de Swap.
				int returnDeSwap = datosEnSocketSwap();
				if(returnDeSwap == -1) {
					//Aca matamos Socket SWAP
					FD_CLR(socketSwap, &sockets);
				}

			} else {

				//Ver que hacer aca, se esta recibiendo algo de un socket en particular
				recibirDatos(&tempSockets, &sockets, socketMaximo);
				if(socketCPUPosta>0) {
					enviarMensajeASwap("Se abrio un nuevo programa por favor reservame memoria");
				}
			}

		}

	} while (1);
}

int main() {
	if (init()) {
			listaCpus = (int *)malloc (10*sizeof(int));
			socketSwap = AbrirConexion(ipSwap, puertoReceptorSwap);
			if (socketSwap < 0) {
				log_info(ptrLog, "No pudo conectarse con Swap");
				return -1;
			}
			log_info(ptrLog, "Se conecto con Swap");

			socketReceptorNucleo = AbrirSocketServidor(puertoTCPRecibirConexionesNucleo);
			if (socketReceptorNucleo < 0) {
				log_info(ptrLog, "No se pudo abrir el Socket Servidor Nucleo de UMC");
				return -1;
			}
			log_info(ptrLog, "Se abrio el socket Servidor Nucleo de UMC");

			socketReceptorCPU = AbrirSocketServidor(puertoTCPRecibirConexionesCPU);
			if (socketReceptorCPU < 0) {
				log_info(ptrLog, "No se pudo abrir el Socket Servidor Nucleo de CPU");
				return -1;
			}
			log_info(ptrLog, "Se abrio el socket Servidor Nucleo de CPU");

			enviarMensajeASwap("Abrimos bien, soy la umc");
			manejarConexionesRecibidas();

		} else {
			log_info(ptrLog, "La UMC no pudo inicializarse correctamente");
			return -1;
		}

		return EXIT_SUCCESS;
}
