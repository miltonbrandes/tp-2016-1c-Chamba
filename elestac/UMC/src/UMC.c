/*
 * UMC.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <commons/log.h>
#include <netinet/in.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <sockets/ServidorFunciones.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>
#include <pthread.h>
#include <semaphore.h>

#define SLEEP 1000000

typedef struct {
	int operacion;
	int PID;
	int pagina_proceso;
	int tamanio_msj;
} t_headerCPU;

// ENTRADAS A LA TLB //
typedef struct {
	int pid;
	int pagina;
	char * direccion_fisica;
	int marco;
} t_tlb;

// STRUCT TABLA PARA CADA PROCESO QUE LLEGA //
typedef struct {
	int pag; // Contiene el numero de pagina del proceso
	char * direccion_fisica; //Contiene la direccion de memoria de la pagina que se esta referenciando
	int marco; // numero de marco (si tiene) en donde esta guardada la pagina
	int accessed;
	int dirty;
	int puntero;
} process_pag;

typedef struct package_iniciar_programa {
	uint32_t programID;
	uint32_t tamanio;
} t_iniciar_programa;

typedef struct _package_cambio_proc_activo {
	uint32_t programID;
} t_cambio_proc_activo;

typedef struct _package_enviarBytes {
	uint32_t base;
	uint32_t offset;
	uint32_t tamanio;
	char* buffer;
} t_enviarBytes;

//PARA INCLUIR EN LA LIBRERIA
typedef struct {
	int numCpu;
	int socket;
	bool enUso;
} t_cpu;

//sem_t semEsperaCPU;

//struct para conexiones

typedef struct Conexiones {
	int socket_escucha;					// Socket de conexiones entrantes
	struct sockaddr_in direccion;
	// Datos de la direccion del servidor
	socklen_t tamanio_direccion;		// Tamaño de la direccion
	//t_cpu CPUS[miContexto.cantHilosCpus];						// Sockets de conexiones ACEPTADAS
	//hago el vector de arriba de forma dinamica
	t_cpu *CPUS; //apunta al primer elemento del vector dinamico
} t_conexiones;

//TODO: falta que el umc reciba cuando hay un programa nuevo de cpu!!!!

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
int marcos, marcosSize, marcoXProc, entradasTLB, retardo, tlbHabilitada,
		hilosCpus;

char *ipSwap;
//int *listaCpus;
int i = 0;

//Variables frames, tlb
t_list * framesOcupados;
t_list * framesVacios;

pthread_t threadSwap;
pthread_t threadNucleo;
pthread_t threadCPU;

//Metodos para Iniciar valores de la UMC
int crearLog() {
	ptrLog = log_create(getenv("UMC_LOG"), "UMC", 1, 0);
	if (ptrLog) {
		return 1;
	} else {
		return 0;
	}
}

int iniciarUMC(t_config* config) {
	//int marcos, marcosSize, marcoXProc, entradasTLB, retardo, tlbHabilitada;

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

	if (config_has_property(config, "TLB_HABILITADA")) {
		tlbHabilitada = config_get_int_value(config, "TLB_HABILITADA");
	} else {
		log_info(ptrLog,
				"El archivo de configuracion no contiene la clave TLB_HABILITADA");
		return 0;
	}

	return 1;
}

int cargarValoresDeConfig() {
	t_config* config;
	config = config_create(getenv("UMC_CONFIG"));

	if (config) {
		if (config_has_property(config, "PUERTO_CPU")) {
			puertoTCPRecibirConexionesCPU = config_get_int_value(config,
					"PUERTO_CPU");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave PUERTO_CPU");
			return 0;
		}

		if (config_has_property(config, "PUERTO_NUCLE0")) {
			puertoTCPRecibirConexionesNucleo = config_get_int_value(config,
					"PUERTO_NUCLE0");
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

void datosEnSocketReceptorNucleoCPU(int socketNuevaConexion) {
	char *buffer;
	uint32_t id;
	uint32_t operacion;
	buffer = recibirDatos(socketSwap, &operacion, &id);
	int bytesRecibidos = strlen(buffer);

	if (bytesRecibidos < 0) {
		log_info(ptrLog,
				"Ocurrio un error al recibir datos en un Socket Nucleo o CPU");
	} else if (bytesRecibidos == 0) {
		log_info(ptrLog, "No se recibieron datos en el Socket Nucleo o CPU");
	} else {
		if(strcmp("ERROR", buffer) == 0) {

		}else{
			log_info(ptrLog, "Bytes recibidos desde Nucleo o CPU: %s", buffer);
		}
	}
}

int datosEnSocketSwap() {
	char* buffer;
	uint32_t id;
	uint32_t operacion;
	buffer = recibirDatos(socketSwap, &operacion, &id);
	int bytesRecibidos = strlen(buffer);

	if (bytesRecibidos < 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos de Swap");
		return -1;
	} else if (bytesRecibidos == 0) {
		log_info(ptrLog,
				"No se recibieron datos de Swap. Se cierra la conexion");
		finalizarConexion(socketSwap);
		return -1;
	} else {
		if(strcmp("ERROR", buffer) == 0) {

		}else{
			log_info(ptrLog, "Bytes recibidos desde Swap: %s", buffer);
		}
	}

	return 0;
}

int obtenerSocketMaximoInicial() {
	int socketMaximoInicial = 0;

	if (socketReceptorCPU > socketReceptorNucleo) {
		if (socketReceptorCPU > socketSwap) {
			socketMaximoInicial = socketReceptorCPU;
		} else if (socketSwap > socketReceptorNucleo) {
			socketMaximoInicial = socketSwap;
		}
	} else if (socketReceptorNucleo > socketSwap) {
		socketMaximoInicial = socketReceptorNucleo;
	} else {
		socketMaximoInicial = socketSwap;
	}

	return socketMaximoInicial;
}

void *escuchar(struct Conexiones* conexion) {
	int i = 1;
	//semEsperaCPU.__align = 0; // inicializa semaforo

	//conexion para el comando cpu
	conexion->CPUS[0].socket = accept(conexion->socket_escucha,
			(struct sockaddr *) &conexion->direccion,
			&conexion->tamanio_direccion);

	while (i <= hilosCpus) //hasta q recorra todos los hilos de cpus habilitados
	{
		//guarda las nuevas conexiones para acceder a ellas desde cualquier parte del codigo
		conexion->CPUS[i].socket = accept(conexion->socket_escucha,
				(struct sockaddr *) &conexion->direccion,
				&conexion->tamanio_direccion);
		if (conexion->CPUS[i].socket == -1) {
			perror("ACCEPT");	//control error
		}
		conexion->CPUS[i].numCpu = i;
		conexion->CPUS[i].enUso = false;
		//ACA ME FIJO QUE OPERACION ES
		//	sem_post(&semEsperaCPU); //avisa que hay 1 CPU disponible
		//puts("NUEVO HILO ESCUCHA!\n");
		//log_info(logger, "CPU %d conectado", i);
		i++;
	}

	return NULL;
}

void manejarConexionesRecibidas() {
	//int handshakeNucleo = 0;
	fd_set sockets, tempSockets;
	//int resultadoSelect;

	int socketMaximo = obtenerSocketMaximoInicial();
	int socketCPUPosta = -121211;

	FD_SET(socketReceptorNucleo, &sockets);
	FD_SET(socketReceptorCPU, &sockets);
	FD_SET(socketSwap, &sockets);

	while (1) {
		FD_ZERO(&tempSockets);
		log_info(ptrLog, "Esperando conexiones");
		tempSockets = sockets;
		int resultadoSelect = select(socketMaximo + 1, &tempSockets, NULL, NULL,
				NULL);
		if (resultadoSelect == 0) {
			log_info(ptrLog, "Time out. Volviendo a esperar conexiones");
		} else if (resultadoSelect < 0) {
			log_info(ptrLog, "Ocurrio un error");
		} else {

			if (FD_ISSET(socketReceptorNucleo, &tempSockets)) {

				int nuevoSocketConexion = AceptarConexionCliente(
						socketReceptorNucleo);
				if (nuevoSocketConexion < 0) {
					log_info(ptrLog,
							"Ocurrio un error al intentar aceptar una conexion de CPU o Nucleo");
				} else {
					FD_SET(nuevoSocketConexion, &sockets);
					FD_SET(nuevoSocketConexion, &tempSockets);
					socketMaximo =
							(socketMaximo < nuevoSocketConexion) ?
									nuevoSocketConexion : socketMaximo;
				}
				FD_CLR(socketReceptorNucleo, &tempSockets);

			} else if (FD_ISSET(socketReceptorCPU, &tempSockets)) {

				int nuevoSocketConexion = AceptarConexionCliente(
						socketReceptorCPU);
				if (nuevoSocketConexion < 0) {
					log_info(ptrLog,
							"Ocurrio un error al intentar aceptar una conexion de CPU");
				} else {
					FD_SET(nuevoSocketConexion, &sockets);
					FD_SET(nuevoSocketConexion, &tempSockets);
					socketMaximo =
							(socketMaximo < nuevoSocketConexion) ?
									nuevoSocketConexion : socketMaximo;
				}
				//aca deberia ir un mensaje hacia swap diciendo que cpu me esta pidiendo espacio, = que con el nucleo usar una lista para cpus!!!

				//agrego las cpus a una nueva lista
				//			conexiones.CPUS[i].socket = nuevoSocketConexion;
				//			conexiones.CPUS[i].numCpu = i++;
				//			conexiones.CPUS[i].enUso = false;
				i++;
				socketCPUPosta = nuevoSocketConexion;

				FD_CLR(socketReceptorCPU, &tempSockets);

			} else if (FD_ISSET(socketSwap, &tempSockets)) {

				//Ver que hacer aca, no se acepta conexion, se recibe algo de Swap.
				int returnDeSwap = datosEnSocketSwap();
				if (returnDeSwap == -1) {
					//Aca matamos Socket SWAP
					FD_CLR(socketSwap, &sockets);
				}

			} else {

				//alojo memoria dinamicamente en tiempo de ejecucion
				//	conexiones.CPUS = (t_cpu*) malloc(sizeof(t_cpu) * ((hilosCpus) + 1));
				//		if (conexiones.CPUS == NULL)
				puts("ERROR MALLOC 1");
				pthread_t hilo_conexiones;
				//		if (pthread_create(&hilo_conexiones, NULL, (void*) escuchar, &conexiones) < 0)
				//				perror("Error HILO ESCUCHAS!");
				//			conexiones.CPUS[0].enUso = true;

				puts("UMC ESPERANDO CONEXIONES....\n\n\n");
				//			sem_wait(&semEsperaCPU); //semaforo espera conexiones

				//Ver que hacer aca, se esta recibiendo algo de un socket en particular
				//recibirDatos(&tempSockets, &sockets, socketMaximo);
				char* buffer;
				uint32_t id;
				uint32_t operacion;
				int bytesRecibidos;
				int socketFor;
				for (socketFor = 0; socketFor < (socketMaximo + 1);
						socketFor++) {
					if (FD_ISSET(socketFor, &tempSockets)) {
						buffer = recibirDatos(socketSwap, &operacion, &id);
						int bytesRecibidos = strlen(buffer);
						if (bytesRecibidos > 0) {
							if(strcmp("ERROR", buffer) == 0) {

							}else{
								buffer[bytesRecibidos] = 0;
								log_info(ptrLog, buffer);
								//ACA ME DEBERIA FIJAR QUIEN ES EL QUE ME LO MANDO MEDIANTE EL ID DEL PROCESO QUE ESTA GUARDADO EN EL BUFFER
								//Recibimos algo
							}
						} else if (bytesRecibidos == 0) {
							//Aca estamos matando el Socket que esta fallando
							//Ver que hay que hacer porque puede venir de CPU o de Nucleo
							finalizarConexion(socketFor);
							FD_CLR(socketFor, &tempSockets);
							FD_CLR(socketFor, &sockets);
							log_info(ptrLog,
									"No se recibio ningun byte de un socket que solicito conexion.");
						} else {
							finalizarConexion(socketFor);
							FD_CLR(socketFor, &sockets);
							log_info(ptrLog,
									"Ocurrio un error al recibir los bytes de un socket");
						}
					}
				}
				if (socketCPUPosta > 0) {
					//	enviarMensajeASwap("Se abrio un nuevo programa por favor reservame memoria");
				}
			}
		}
	}
}

char * reservarMemoria(int cantidadMarcos, int tamanioMarco) {
	// calloc me la llena de \0
	char * memoria = calloc(cantidadMarcos, tamanioMarco);
	return memoria;
}

int main() {
	if (init()) {
		socketSwap = AbrirConexion(ipSwap, puertoReceptorSwap);
		if (socketSwap < 0) {
			log_info(ptrLog, "No pudo conectarse con Swap");
			return -1;
		}
		log_info(ptrLog, "Se conecto con Swap");
		socketReceptorNucleo = AbrirSocketServidor(
				puertoTCPRecibirConexionesNucleo);
		if (socketReceptorNucleo < 0) {
			log_info(ptrLog,
					"No se pudo abrir el Socket Servidor Nucleo de UMC");
			return -1;
		}
		log_info(ptrLog, "Se abrio el socket Servidor Nucleo de UMC");
		socketReceptorCPU = AbrirSocketServidor(puertoTCPRecibirConexionesCPU);
		if (socketReceptorCPU < 0) {
			log_info(ptrLog,
					"No se pudo abrir el Socket Servidor Nucleo de CPU");
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
