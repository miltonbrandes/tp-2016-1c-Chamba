/*

 * nucleo.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */
//TODO: le tengo que pasar tambien cual es la primer pagina de mi codigo a la cpu para que se la pueda pedir a la umc??? en principio se lo paso por las dudas
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/string.h>
#include <sockets/ServidorFunciones.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/collections/queue.h>
#include "estructurasNucleo.h"
#include "operacionesNucleo.h"
#include <parser/metadata_program.h>
#define MAX_BUFFER_SIZE 4096
enum CPU {
	QUANTUM = 2,
	IO,
	EXIT,
	IMPRIMIR_VALOR,
	IMPRIMIR_TEXTO,
	LEER_VAR_COMPARTIDA,
	ASIG_VAR_COMPARTIDA,
	WAIT,
	SIGNAL,
	SIGUSR
};
enum UMC{
	NUEVOPROGRAMA = 1,
	LEER,
	ESCRIBIR,
	FINALIZARPROGRAMA,
	HANDSHAKE,
	CAMBIOPROCESOACTIVO
};
enum OPERACIONES_GENERALES {
	ERROR = -1, NOTHING, SUCCESS
};
uint32_t nroProg = 0;
char *ipUMC;
fd_set sockets, tempSockets; //descriptores
int *listaCpus;
int cpuAcerrar = 0;
int i = 0;
int puertoConexionUMC;
//Variables de configuracion para establecer Conexiones
int puertoReceptorCPU, puertoReceptorConsola;
//Variables propias del Nucleo
int quantum, quantumSleep;
//Socket que recibe conexiones de CPU y Consola
int socketReceptorCPU, socketReceptorConsola;
int socketUMC;
int tamanioMarcos = 0;
int tamanioStack;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_exit = PTHREAD_MUTEX_INITIALIZER;
pthread_t hiloCpuOciosa;
pthread_t threadPlanificador;
pthread_t threadSocket;
sem_t semCpuOciosa;
sem_t semMensajeImpresion;
sem_t semNuevoPcbColaReady;
sem_t semNuevoProg;
sem_t semProgExit;
sem_t semProgramaFinaliza;
t_config* config;
t_EstructuraInicial *estructuraInicial;
t_list *listaDispositivosIO, *listaSemaforos, *listaVariablesCompartidas;
t_list* colaNew;
t_list* listaIndCodigo;
t_list* listaSocketsConsola;
t_list* listaSocketsCPUs;
t_list* pcbStack;
//Archivo de Log
t_log* ptrLog;
t_pcb* crearPCB(char* programa,int socket);
t_queue* colaBloqueados;
t_queue* colaExit;
t_queue* colaImprimibles;
t_queue* colaReady;
uint32_t handshakeumv=0;
uint32_t pcbAFinalizar;
uint32_t pcbAFinalizar; // pcb q vamos a cerrar porque el programa cerro mal
uint32_t tamaniostack;
void envioPCBaClienteOcioso(t_clienteCpu *clienteSeleccionado);
void operacionesConSemaforos(char operacion, char* buffer, t_clienteCpu *unCliente);
//Metodos para iniciar valores de Nucleo
int crearLog() {
	ptrLog = log_create(getenv("NUCLEO_LOG"), "Nucleo", 1, 0);
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
	char ** idIO;					// Aca voy a cargar todos los IO
	char ** values_IO;				// Valores de los IO
	t_IO *entradaSalida;
	idIO = ioIds;
	values_IO = ioSleepValues;
	int ivar = 0;
	while (idIO[ivar] != NULL ) {
	entradaSalida = malloc(sizeof(t_IO));
	entradaSalida->nombre = idIO[ivar];
	entradaSalida->valor = atoi(values_IO[ivar]);
	entradaSalida->ID_IO = ivar;
	entradaSalida->colaSolicitudes = queue_create();
	sem_init(&entradaSalida->semaforo, 1, 0);
	list_add(listaDispositivosIO, entradaSalida);
	log_trace(ptrLog, "(\"%s\", valor:%d, ID:%d)", entradaSalida->nombre,entradaSalida->valor, entradaSalida->ID_IO);
	ivar++;
	}
	free(idIO);
	free(values_IO);
	log_trace(ptrLog, ":: Finalizada la carga de la lista de IO ::");
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

	config = config_create(getenv("NUCLEO_CONFIG"));
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
		if(config_has_property(config, "TAMANIO_STACK"))
		{
			tamaniostack = config_get_int_value(config, "TAMANIO_STACK");

		}
		else{
			log_info(ptrLog, "el archivo de configuracion no tiene la clave stack");
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

int enviarMensajeACpu(socket)
{

	char *buffer;
	uint32_t id = 3;
	uint32_t operacion = 1;
	buffer = "Estoy recibiendo una nueva consola/0";
	log_info(ptrLog, buffer);
	int longitud = strlen(buffer);
	int bytesEnviados = enviarDatos(socket,buffer, longitud ,operacion,id);
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

int enviarMensajeAUMC(char* msj) {
	uint32_t id = 3;
	uint32_t operacion = 1;
	log_info(ptrLog, *msj);
	int longitud = strlen(msj);
	int bytesEnviados = enviarDatos(socket,msj, longitud ,operacion,id);
	if(bytesEnviados < 0) {
		log_info(ptrLog, "Ocurrio un error al enviar mensajes a UMC");
		return -1;
	}else if(bytesEnviados == 0){
		log_info(ptrLog, "No se envio ningun byte a UMC");
	}else{
		log_info(ptrLog, "Mensaje a UMC enviado: %s", msj);
	}
	return 1;
}

int enviarMensajeAConsola(int socket) {
	char* buffer = "Consola aca terminas papa";
	log_info(ptrLog, buffer);
	int longitud = strlen(buffer);
	uint32_t operacion = 1;
	uint32_t id = 3;
	int bytesEnviados = enviarDatos(socket,buffer, longitud ,operacion,id);
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
	char *buffer;
	uint32_t id;
	uint32_t operacion;
	int bytesRecibidos = recibirDatos(nuevoSocketConexion, &buffer, &operacion, &id);

	if(bytesRecibidos < 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos en un Socket CPU");
	} else if(bytesRecibidos == 0) {
		log_info(ptrLog, "No se recibieron datos en el Socket CPU");
	} else {
		log_info(ptrLog, "Bytes recibidos desde una CPU: %s", buffer);
		char *mensajeParaCPU = "Este es un mensaje para vos, CPU\0";
		int bytesEnviados = enviarDatos(nuevoSocketConexion,buffer, (uint32_t)strlen(mensajeParaCPU) ,operacion,id);

		if(bytesEnviados < 0)
		{
			log_error(ptrLog, "Error al enviar datos a cpu");
		}
	}
}

int datosEnSocketUMC() {
	char *buffer;
	uint32_t id;
	uint32_t operacion;
	int bytesRecibidos = recibirDatos(socketUMC,  &buffer, &operacion,&id);

	if(bytesRecibidos < 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos en Socket UMC");
	} else if(bytesRecibidos == 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos en Socket UMC");
		finalizarConexion(socketUMC);
		return -1;
	} else {
		log_info(ptrLog, "Bytes recibidos desde UMC: %s", buffer);
	}
	if(handshakeumv == 0)
	{
		tamanioMarcos = (int)*buffer;
		handshakeumv++;
	}
	return 0;
}

void operacionesConVariablesCompartidas(char operacion, char *buffer,
		uint32_t socketCliente) {
	log_debug(ptrLog, "Se ingresa a operacionVariablesCompartidas");
	t_op_varCompartida * varCompartida;
	t_variable_compartida * varCompartidaEnLaLista;
	char* valorEnBuffer, *bufferLeerVarCompartida;
	_Bool _obtenerVariable(t_variable_compartida * unaVariable) {
		return (strcmp(unaVariable->nombre, varCompartida->nombre) == 0);
	}
	_Bool _obtenerVariable2(t_variable_compartida * unaVariable) {
		return (strcmp(unaVariable->nombre, bufferLeerVarCompartida) == 0);
	}

	// Dependiendo de la operacion..
	switch (operacion) {
	case ASIG_VAR_COMPARTIDA:
		log_debug(ptrLog,
				"Cliente quiere asignar valor a variable compartida");
		//Asigno el valor a la variable
		varCompartida = deserializar_opVarCompartida(*buffer);
		varCompartidaEnLaLista = list_find(listaVariablesCompartidas,
				(void*) _obtenerVariable);
		varCompartidaEnLaLista->valor = varCompartida->valor;
		free(varCompartida);
		break;
	case LEER_VAR_COMPARTIDA:
		log_debug(ptrLog, "Cliente quiere leer variable compartida");
		//Leo el valor de la variable y lo envio
		valorEnBuffer = malloc(sizeof(uint32_t));
		//	bufferLeerVarCompartida=malloc(strlen(buffer));
		bufferLeerVarCompartida = buffer;
		varCompartidaEnLaLista = list_find(listaVariablesCompartidas,
				(void*) _obtenerVariable2);
		memcpy(valorEnBuffer, &(varCompartidaEnLaLista->valor),
				sizeof(uint32_t));
		enviarDatos(socketCliente, &valorEnBuffer, sizeof(uint32_t), NOTHING);
		free(valorEnBuffer);
		free(bufferLeerVarCompartida);
		break;
	}
}

void escucharPuertos() {

	int socketMaximo = obtenerSocketMaximoInicial(); //descriptor mas grande
	FD_SET(socketReceptorCPU, &sockets);
	FD_SET(socketReceptorConsola, &sockets);
	FD_SET(socketUMC, &sockets);

	while(1)
	{
		FD_ZERO(&tempSockets);
		tempSockets = sockets;
		log_info(ptrLog, "Esperando conexiones");

		int resultadoSelect = select(socketMaximo+1, &tempSockets, NULL, NULL, NULL);
		if (resultadoSelect == 0) {
			log_info(ptrLog, "Time out. Volviendo a esperar conexiones");
		} else if (resultadoSelect < 0) {
			log_info(ptrLog, "Ocurrio un error en el select de nucleo");
			break;
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
				t_clienteCpu *cliente;
				cliente = malloc(sizeof(t_clienteCpu));
				cliente->socket= nuevoSocketConexion;
				cliente->id = (uint32_t)list_size(listaSocketsCPUs);
				cliente->fueAsignado = false;
				cliente->programaCerrado = false;
				list_add(listaSocketsCPUs, cliente);
				estructuraInicial = malloc(sizeof(t_EstructuraInicial));
				estructuraInicial->Quantum = quantum;
				estructuraInicial->RetardoQuantum = quantumSleep;
				char* envio = malloc(sizeof(t_EstructuraInicial));
				envio = serializar_EstructuraInicial(estructuraInicial);
				enviarDatos(cliente->socket, &envio, sizeof(t_EstructuraInicial), NOTHING, NUCLEO);
				sem_post(&semCpuOciosa);
				log_debug(ptrLog, "Cliente aceptado y quantum enviado");
				free(estructuraInicial);
				free(envio);
				FD_CLR(socketReceptorCPU, &tempSockets);
				//aca deberia recibir alguna respuesta de cpu????
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

				uint32_t id;
				t_pcb *unPcb;
				char *buffer;
				uint32_t operacion;
				int bytesRecibidos = recibirDatos(nuevoSocketConexion,  &buffer, &operacion, &id);
				if(bytesRecibidos < 0) {
					log_info(ptrLog, "Ocurrio un error al recibir datos en un Socket Consola");
				} else if(bytesRecibidos == 0) {
					log_info(ptrLog, "No se recibieron datos en el Socket Consola");
				} else {
					log_info(ptrLog, "Recibi lo siguiente de consola: %s", buffer);
					t_pcb *unPcb;
					unPcb = crearPCB(buffer, socketUMC);

					buffer = "Este es un mensaje para vos, Consola\0";
					int bytesEnviados = enviarDatos(nuevoSocketConexion, buffer, (uint32_t)strlen(buffer), operacion, id);
					if(bytesEnviados<0)
					{
						log_error(ptrLog, "Error al enviar los datos");
					}
				}
				t_socket_pid* unCliente = malloc(sizeof(t_socket_pid));/*Definimos la estructura del cliente con su socket y pid asociado*/
				unCliente->pid = nroProg;
				unCliente->socket = i;
				unCliente->terminado = false;
				list_add(listaSocketsConsola, unCliente);
				FD_CLR(socketReceptorConsola, &tempSockets);
				if (unPcb == NULL && buffer[0] == -1) {
					log_error(ptrLog,
					"Hubo un fallo en la creacion del PCB - Causa: Memory Overload");
					t_imprimibles *unMensaje = malloc(
					sizeof(t_imprimibles));
					unCliente->terminado = true;
					char* texto ="Error al crear el pcb correspondiente a tu programa - Causa: Memory Overload";
					unMensaje->PCB_ID = nroProg;
					unMensaje->valor = texto;
					queue_push(colaImprimibles, unMensaje);
					sem_post(&semMensajeImpresion);
					} else if (unPcb == NULL && buffer[0] == -2) {
					log_error(ptrLog,
						"Hubo un fallo en la creacion del PCB - Causa: Segmentation Fault");
					t_imprimibles *unMensaje = malloc(
					sizeof(t_imprimibles));
					unCliente->terminado = true;
					char* texto =
							"Error al crear el pcb correspondiente a tu programa - Causa: Segmentation Fault";
					unMensaje->PCB_ID = nroProg;
					unMensaje->valor = texto;
					queue_push(colaImprimibles, unMensaje);
					sem_post(&semMensajeImpresion);
					} else {
					log_debug(ptrLog,
							"Todo reservado, se procede a ponerlo en la lista New");
							list_add(colaNew, unPcb);
						log_info(ptrLog,
							"===== Lista de Procesos en cola NEW =====");
						void _imprimirProcesoNew(t_pcb* pcb) {
						log_debug(ptrLog, "Proceso %d en estado NEW \n",
						pcb->pcb_id);
					}
					list_iterate(colaNew, (void*) _imprimirProcesoNew);
					sem_post(&semNuevoProg);

					}


			} else if(FD_ISSET(socketUMC, &tempSockets)) {
				//Ver como es aca porque no estamos aceptando una conexion cliente, sino recibiendo algo de UMC
				int returnDeUMC = datosEnSocketUMC();
				if(returnDeUMC == -1) {
					//Aca matamos Socket UMC
					FD_CLR(socketUMC, &sockets);

				}

			} else
			{

				//Ver que hacer aca, se esta recibiendo algo de un socket en particular
				char* buffer;
				int bytesRecibidos;
				uint32_t id;
				uint32_t operacion;
				int socketFor;
				for (socketFor = 0; socketFor < (socketMaximo + 1); socketFor++)
				{
					if (FD_ISSET(socketFor, &tempSockets))
					{
						bytesRecibidos = recibirDatos(socketFor, &buffer, &operacion, &id);
						if (bytesRecibidos > 0)
						{
							if(id == CPU){
								//entonces recibio de cpu!!!
								log_info(ptrLog, "Mensaje recibido: %s", *buffer);
								void _comprueboReciboMensajes(t_clienteCpu *unCliente)
								{
									unCliente->socket = socketFor;
									if (unCliente->programaCerrado && (operacion == QUANTUM || operacion == EXIT || operacion == IO)){
										unCliente->fueAsignado = false;
										unCliente->programaCerrado = false;
										sem_post(&semCpuOciosa);
										return;
									}
									if (unCliente->programaCerrado && (operacion == IMPRIMIR_VALOR || operacion == IMPRIMIR_TEXTO || operacion == ASIG_VAR_COMPARTIDA || operacion == SIGNAL))
									return;
									t_pcb* unPCB;
									switch (operacion) {
									case QUANTUM:
											log_debug(ptrLog, "Se ingresa a operacionQuantum");
											log_debug(ptrLog, "Cliente %d envía unPCB", unCliente->id);
											unPCB = deserializar_pcb((char **) &buffer);// aca deberia deserializar el pcb mediante lo que lei desde leer.
											pthread_mutex_lock(&mutex);
											queue_push(colaReady, (void*) unPCB);
											pthread_mutex_unlock(&mutex);
											log_debug(ptrLog, "Agregado a la colaReady el PCB del pid: %d",
													unPCB->pcb_id);
											if (cpuAcerrar!=unCliente->socket) {
												unCliente->fueAsignado = false;
												sem_post(&semCpuOciosa);
											}
											sem_post(&semNuevoPcbColaReady);
											free(buffer);
										break;
									case IO:
											log_debug(ptrLog, "Se ingresa a operacionIO");

											t_solicitudes *solicitud = malloc(sizeof(t_solicitudes));
											t_dispositivo_io opIO;
											log_debug(ptrLog, "Cliente %d envía unPCB", unCliente->id);
											//opIO = deserializar_opIO(&buffer); me falta crear que va a tener la operacion entrada salida

											unPCB = deserializar_pcb((char **) &buffer);
											solicitud->pcb = unPCB;
											solicitud->valor = opIO.tiempo;

											bool _BuscarIO(t_dispositivo_io* element) {
												return (strcmp(opIO.nombre, element->nombre) == 0);
											}
											t_IO * entradaSalida;
											entradaSalida = list_find(listaDispositivosIO, (void*) _BuscarIO);
											if (entradaSalida == NULL ) {
												log_error(ptrLog,
														"Dispositivo IO no encontrado en archivo de configuracion");
												log_debug(ptrLog, "Agregado el PCB_ID:%d a la cola Ready",
														unPCB->pcb_id);
												queue_push(colaReady, unPCB);
												sem_post(&semNuevoPcbColaReady);
											} else {
												queue_push(entradaSalida->colaSolicitudes, solicitud);
												sem_post(&entradaSalida->semaforo);
											}
											if (cpuAcerrar!=unCliente->socket) {
													unCliente->fueAsignado = false;
													sem_post(&semCpuOciosa);
												}
											free(buffer);
										break;
									case EXIT:
										log_debug(ptrLog, "Se ingresa a operacionExit");

											log_debug(ptrLog, "CPU %d envía unPCB para desalojar", unCliente->id);
											unPCB = deserializar_pcb((char **) &buffer); //debo deserializar el pcb que me envio la cpu
											char* texto = "Se ha finalizado el programa correctamente";
											t_imprimibles *imp = malloc(sizeof(t_imprimibles));
											imp->PCB_ID = unCliente->pcbAsignado->pcb_id;
											imp->tipoDeValor = EXIT;
											imp->valor = buffer;
											queue_push(colaImprimibles, imp);
											sem_post(&semMensajeImpresion);
											log_debug(ptrLog, "Agregado a la colaExit el PCB del pid: %d",unPCB->pcb_id);
											queue_push(colaExit, (void*) unPCB);
											sem_post(&semProgExit);
											pthread_mutex_unlock(&mutex_exit);
											if (cpuAcerrar!=unCliente->socket) {
												unCliente->fueAsignado = false;
												sem_post(&semCpuOciosa);
											}
											free(buffer);
										break;
									case IMPRIMIR_VALOR:
									{
										t_imprimibles* imprimi = malloc(sizeof(t_imprimibles));
										imprimi->tipoDeValor = IMPRIMIR_VALOR;
										imprimi->valor = buffer;
										imprimi->PCB_ID = unCliente->pcbAsignado->pcb_id;
										queue_push(colaImprimibles, imprimi);
										sem_post(&semMensajeImpresion);
										break;
									}
									case IMPRIMIR_TEXTO:
									{
										t_imprimibles* imprimir = malloc(sizeof(t_imprimibles));
										imprimir->tipoDeValor = IMPRIMIR_TEXTO;
										imprimir->valor = buffer;
										imprimir->PCB_ID = unCliente->pcbAsignado->pcb_id;
										queue_push(colaImprimibles, imprimir);
										sem_post(&semMensajeImpresion);
										break;
									}
									case LEER_VAR_COMPARTIDA:
										operacionesConVariablesCompartidas(LEER_VAR_COMPARTIDA,buffer, unCliente->socket);
										break;
									case ASIG_VAR_COMPARTIDA:
										operacionesConVariablesCompartidas(ASIG_VAR_COMPARTIDA,buffer, unCliente->socket);
										break;
									case WAIT:
										operacionesConSemaforos(WAIT, buffer, unCliente);
										break;
									case SIGNAL:
										operacionesConSemaforos(SIGNAL, buffer, unCliente);
										break;
									case SIGUSR:
										cpuAcerrar=unCliente->socket;
										free(buffer);
										break;
									}

								}
							}
							else if(id == UMC)
							{

								//entonces recibio de umc
							}
							else if(id == CONSOLA)
							{

							}
						}
						else if (bytesRecibidos == 0)
						{
							finalizarConexion(socketFor);
							FD_CLR(socketFor, &tempSockets);
							FD_CLR(socketFor, &sockets);
							log_info(ptrLog, "No se recibio ningun byte de un socket que solicito conexion.");
						}
						else if (bytesRecibidos < 0)
						{
							finalizarConexion(socketFor);
							FD_CLR(socketFor, &sockets);
							log_info(ptrLog, "Ocurrio un error al recibir los bytes de un socket");
						}
					}
				}
			}
		}
	}
}

void operacionesConSemaforos(char operacion, char* buffer, t_clienteCpu *unCliente) {
	log_debug(ptrLog, "Se ingresa a operacionConSemaforos");
	_Bool _obtenerSemaforo(t_semaforo *unSemaforo) {
		return (strcmp(unSemaforo->nombre, buffer) == 0);
	}

	t_semaforo * semaforo;
	t_pcbBlockedSemaforo * unPcbBlocked;
	char* bufferPCB;
	semaforo = list_find(listaSemaforos, (void*) _obtenerSemaforo);
	switch (operacion) {
	case WAIT:
		if (unCliente->programaCerrado)
		{
			enviarDatos(unCliente->socket, &buffer, sizeof(buffer), ERROR);
			break;
		}

		if (semaforo->valor > 0) {
			//No se bloquea
			enviarDatos(unCliente->socket, &buffer, sizeof(buffer), NOTHING);
		} else {
			//Se bloquea
			enviarDatos(unCliente->socket, &buffer, sizeof(buffer), WAIT);
			unPcbBlocked = malloc(sizeof(t_pcbBlockedSemaforo));
			recibirDatos(unCliente->socket, &bufferPCB, NULL );
			unPcbBlocked->nombreSemaforo = semaforo->nombre;
			unPcbBlocked->pcb = deserializar_pcb((char **) &bufferPCB);
			free(bufferPCB);
			list_add(colaBloqueados, unPcbBlocked);
			log_debug(ptrLog,
					"Agregado a la colaBlockeadoPorSemaforo el PCB del pid: %d",
					unPcbBlocked->pcb->pcb_id);
			unCliente->fueAsignado = false;
			sem_post(&semCpuOciosa);
		}
		semaforo->valor--;
		free(buffer);
		break;
	case SIGNAL:

		//comprobar si podria despertar a alguien
		if (semaforo->valor < 0) {

			_Bool _obtenerPcbBlockeado(t_pcbBlockedSemaforo *unPcbBlockeado) {
				return (strcmp(unPcbBlockeado->nombreSemaforo, semaforo->nombre)
						== 0);
			}
			t_pcbBlockedSemaforo* pcbBlockeado;
			pcbBlockeado = list_find(colaBloqueados,
					(void*) _obtenerPcbBlockeado);
			//Compruebo si hay algun pcb blocked por este semaforo
			if (pcbBlockeado != NULL ) {
				//lo desbloqueo, lo mando a la colaReady

				_Bool _quitarPcbBlockeado(t_pcbBlockedSemaforo *unPcbBlockeado) {
					return (unPcbBlockeado->pcb->pcb_id
							== pcbBlockeado->pcb->pcb_id);
				}
				pthread_mutex_lock(&mutex);
				list_remove_by_condition(colaBloqueados,
						(void*) _quitarPcbBlockeado);
				queue_push(colaReady, pcbBlockeado->pcb);
				sem_post(&semNuevoPcbColaReady);
				pthread_mutex_unlock(&mutex);
			}
		}
		semaforo->valor++;
		break;
	}
}

void cerrarConexionCliente(t_clienteCpu *unCliente) {
	log_debug(ptrLog, "CPU %d ha cerrado la conexión", unCliente->id);
	_Bool _sacarCliente(t_clienteCpu * elCliente) {
		return unCliente->socket == elCliente->socket;
	}
	// Si tenia un pcb asignado, lo mando a la cola d exit para liberar recursos..
	if (unCliente->fueAsignado == true && cpuAcerrar!=unCliente->socket) {
		log_debug(ptrLog, "Agregado a la colaExit el PCB del pid: %d",
				unCliente->pcbAsignado->pcb_id);
		queue_push(colaExit, unCliente->pcbAsignado);
		sem_post(&semProgExit);
		pthread_mutex_unlock(&mutex_exit);
	}
	list_remove_by_condition(listaSocketsCPUs,(void*)_sacarCliente);
	if (cpuAcerrar==unCliente->socket) { cpuAcerrar=0;	}
	else {sem_wait(&semCpuOciosa);}
}

char* enviarOperacion(uint32_t operacion, void* estructuraDeOperacion, int serverSocket) {
	int packageSize;
	char *paqueteSerializado;
	char* respuestaOperacion;

	switch (operacion) {
	case LEER:
		//info del segundo paquete
		packageSize = sizeof(t_solicitarBytes) + sizeof(uint32_t);
		paqueteSerializado = serializarSolicitarBytes(estructuraDeOperacion, &operacion);

		if ((enviarDatos(serverSocket, &paqueteSerializado, packageSize, NOTHING)) < 0) {
			free(paqueteSerializado);
			return NULL;
		}

		//####		Recibo buffer pedidos		####
		if (recibirDatos(serverSocket, &respuestaOperacion, NULL ) < 0) {
			free(paqueteSerializado);
			return NULL;
		}

		break;
	case ESCRIBIR:
		packageSize = sizeof(t_enviarBytes) + sizeof(uint32_t);
		t_enviarBytes* aux = (t_enviarBytes*)estructuraDeOperacion;
		packageSize += aux->tamanio;
		paqueteSerializado = serializarEnviarBytes(estructuraDeOperacion, &operacion);

		//info del primer paquete
		if ((enviarDatos(serverSocket, &paqueteSerializado, packageSize, NOTHING)) < 0) {
			free(paqueteSerializado);
			return NULL;
		}
		//Recibo la respuesta si fue exitosa
		if ((recibirDatos(serverSocket, &respuestaOperacion, NULL )) < 0) {
			free(paqueteSerializado);
			return NULL;
		}

		break;

	case CAMBIOPROCESOACTIVO:
		//info del segundo paquete
		packageSize = sizeof(t_cambio_proc_activo) + sizeof(uint32_t);
		paqueteSerializado = serializarCambioProcActivo(estructuraDeOperacion, &operacion);

		//Envio paquete
		if ((enviarDatos(serverSocket, &paqueteSerializado, packageSize, NOTHING)) < 0) {
			free(paqueteSerializado);
			return NULL;
		}

		break;
	case NUEVOPROGRAMA:
		//info del segundo paquete
		packageSize = sizeof(t_iniciar_programa) + sizeof(uint32_t);
		paqueteSerializado = serializarCrearSegmento(estructuraDeOperacion,	&operacion);

		//Envio paquete
		if ((enviarDatos(serverSocket, &paqueteSerializado, packageSize, NOTHING)) < 0) {
			free(paqueteSerializado);
			return NULL;
		}
		//Recibo el valor respuesta de la operación
		respuestaOperacion = malloc(sizeof(int));
		if (recibirDatos(serverSocket, &respuestaOperacion, NULL ) < 0) {
			respuestaOperacion[0] = -1;
			respuestaOperacion[1] = '\0';
			free(paqueteSerializado);
			return respuestaOperacion;
		}

		break;
	case FINALIZARPROGRAMA:
		//info del segundo paquete
		packageSize = sizeof(t_finalizar_programa) + sizeof(uint32_t);
		paqueteSerializado = serializarDestruirSegmento(estructuraDeOperacion, &operacion);

		//envio paquete
		if ((enviarDatos(serverSocket, &paqueteSerializado, packageSize, NOTHING)) < 0) {
			free(paqueteSerializado);
			return NULL;
		}

		//Recibo respuesta
		if (recibirDatos(serverSocket, &respuestaOperacion, NULL ) < 0) {
			free(paqueteSerializado);
			return NULL;
		}

		break;
	default:
		printf("Operacion no admitida");
		break;
	}

	free(paqueteSerializado);
	return respuestaOperacion;
}

t_pcb* crearPCB(char* programa,int socket){
	log_debug(ptrLog,"Procedo a crear el pcb del programa recibido");
	t_pcb* pcb;
	pcb=malloc(sizeof(t_pcb));
	t_metadata_program* datos;
	uint32_t pagina=0;
	int rtaOp=0;
	char* rtaEnvio;
	char* basePagCod;
	char* basePagStack;
	log_debug(ptrLog,"Obtengo la metadata utilizando el preprocesador del parser");
	datos=metadata_desde_literal(programa);
	pcb->pcb_id=++nroProg;//nroProg variable global definida en el Kernel, aumenta en 1 su valor cada vez que entra un programa
	log_info(ptrLog,"Enviamos el 1er pedido a la UMV, para crear el espacio de codigo literal");
	t_iniciar_programa iniciarProg;
	iniciarProg.programID=pcb->pcb_id;
	iniciarProg.tamanio=(strlen(programa+1))/tamanioMarcos;
	basePagCod=malloc(sizeof(uint32_t));
	basePagCod=enviarOperacion(NUEVOPROGRAMA,&iniciarProg,socket);
	memcpy(&rtaOp,basePagCod,sizeof(int));
	if(rtaOp < 0){
		/*Suponemos que en el caso de que no pueda crear un segmento, va a devolver -1*/
		log_error(ptrLog,"No se pudo crear el espacio de codigo literal para el Programa %i",pcb->pcb_id);
		programa[0]=-1;// Indicamos con -1 para saber que el error fue por Memory Overload
		free(pcb);
		free(basePagCod);
		free(datos);
		return NULL;
	}
	pcb->posicionPrimerPaginaCodigo = (uint32_t)basePagCod; //la funcion enviaroperacion me deberia devolver la primer pagina en la que se almacena el codigo
	log_debug(ptrLog,"espacio de codigo literal creado!");
	log_info(ptrLog,"En este punto ya creamos las distintos paginas requeridos para el programa, procedemos a"
			" escribir sobre ellos");
	log_debug(ptrLog,"Enviamos a la UMC el aviso de cambio de Proceso");
	t_cambio_proc_activo procActivo;
	procActivo.programID=pcb->pcb_id;
	enviarOperacion(CAMBIOPROCESOACTIVO,&procActivo,socket);/*Avisamos a la UMC que proceso es el que se va a escribir*/
	t_enviarBytes envioBytes;
	log_debug(ptrLog,"Escribimos sobre las paginas de codigo literal");
	memcpy(&pagina,basePagCod,sizeof(uint32_t));
	envioBytes.pagina=pagina;
	envioBytes.offset=0;
	envioBytes.tamanio=(strlen(programa+1))/tamanioMarcos;
	envioBytes.buffer=malloc(strlen(programa)+1);
	strcpy(envioBytes.buffer,programa);
	rtaEnvio=enviarOperacion(ESCRIBIR,&envioBytes,socket);
	memcpy(&rtaOp,rtaEnvio,sizeof(int));
	if(rtaOp < 0){
		log_error(ptrLog,"Error al tratar de escribir sobre las paginas de codigo");
		programa[0]=-2;
		free(basePagCod);
		free(datos);
		free(envioBytes.buffer);
		free(rtaEnvio);
		queue_push(colaExit,pcb);
		sem_post(&semProgExit);
		return NULL;
	}
	log_debug(ptrLog,"Se ha escrito correctamente sobre las paginas de codigo");
	free(envioBytes.buffer);
	free(rtaEnvio);

	log_debug(ptrLog,"Rellenamos la estructura del pcb con las direcciones devueltas por la UMC y completamos"
	" la creacion del PCB %d",pcb->pcb_id);
	pcb->PC=datos->instruccion_inicio;
	pcb->codigo=datos->instrucciones_size;

	pcbStack= list_create();
	pcb->ind_stack = (t_list *)pcbStack;

    listaIndCodigo= list_create();
    listaIndCodigo=llenarLista(listaIndCodigo, datos->instrucciones_serializado, datos->instrucciones_size);
    pcb->ind_codigo = listaIndCodigo;
	/*La funcion toma el mapa de instrucciones, lo serializa y lo copia en el buffer*/

    if(datos->cantidad_de_etiquetas > 0 || datos->cantidad_de_funciones > 0){
	char* indiceEtiquetas =malloc(datos->etiquetas_size);
	indiceEtiquetas = datos->etiquetas;
	pcb->ind_etiq=indiceEtiquetas;
		}else{
			pcb->ind_etiq=0;
		}
	free(basePagCod);
	free(basePagStack);
	free(datos);
	return pcb;
}

void *hiloClienteOcioso() {

	while (1) {
		// Tengo la colaReady con algun PCB, compruebo si tengo un cliente ocioso..

		sem_wait(&semNuevoPcbColaReady);
		log_debug(ptrLog, "\nPase semaforo semNuevoPcbColaReady\n");
		sem_wait(&semCpuOciosa);
		log_debug(ptrLog, "\nPase semaforo semClienteOcioso\n");
		bool _sinPCB(t_clienteCpu * unCliente) {
			return unCliente->fueAsignado == false;
		}

		t_clienteCpu *clienteSeleccionado = list_get(list_filter(listaSocketsCPUs, (void*) _sinPCB), 0);
		envioPCBaClienteOcioso(clienteSeleccionado);
	}
	return 0;
}

void imprimoInformacionGeneral() {

	log_info(ptrLog, "#### #### #### #### #### ####");
	log_info(ptrLog, "#### #### #### #### #### ####");
	log_info(ptrLog, "#### Cant clientesCPU:%d ####",
			list_size(listaSocketsCPUs));
	//imprimoListaClientesCPU();
	log_info(ptrLog, "#### #### #### #### #### ####");

	log_info(ptrLog, "#### elementos en colaReady:%d ####",
			queue_size(colaReady));
	//recorrerUnaColaDePcbs(colaReady);
	log_info(ptrLog, "#### #### #### #### #### ####");

	//log_info(ptrLog, "#### elementos en colaExit:%d ####",
		//	queue_size(colaExit));
	//recorrerUnaColaDePcbs(colaExit);
	log_info(ptrLog, "#### #### #### #### #### ####");

	//log_info(ptrLog, "#### elementos en lista Bloqueados:%d ####",
		//	list_size(listaBloqueados));
	//imprimoListaBloqueados();

	//log_info(ptrLog, "#### CPU a finalizar=%d , PCB a finalizar=%d####",cpuAcerrar, pcbAFinalizar);
	log_info(ptrLog, "#### Valores semaforos ####");
	log_info(ptrLog, "Semaforo clienteOcioso:%d",semCpuOciosa.__align);
	log_info(ptrLog, "Semaforo NuevoPcbColaReady:%d",semNuevoPcbColaReady.__align);
	log_info(ptrLog, "Semaforo ProgExit:%d",semProgExit.__align);
	log_info(ptrLog, "Semaforo ProgramaFinaliza:%d",semProgramaFinaliza.__align);
	log_info(ptrLog, "Semaforo MensajeImpresio:%d",semMensajeImpresion.__align);
	log_info(ptrLog, "Semaforo NuevoProg:%d",semNuevoProg.__align);

	log_info(ptrLog, "#### #### #### #### #### ####");
}

void envioPCBaClienteOcioso(t_clienteCpu *clienteSeleccionado) {
	imprimoInformacionGeneral();
	log_debug(ptrLog,
			"Obtengo un cliente ocioso que este esperando por un PCB... cliente:%d",
			clienteSeleccionado->id);
	pthread_mutex_lock(&mutex);
	t_pcb *unPCB = queue_pop(colaReady);
	pthread_mutex_unlock(&mutex);
	log_debug(ptrLog, "Obtengo PCB_ID %d de la colaReady", unPCB->pcb_id);
	char *pcbSerializado;
	log_debug(ptrLog, "Serializo el PCB");
	pcbSerializado = serializar_pcb(unPCB);
	enviarDatos(clienteSeleccionado->socket, &pcbSerializado, sizeof(t_pcb),NOTHING);
	log_debug(ptrLog, "PCB enviado al CPU %d", clienteSeleccionado->id);
	clienteSeleccionado->pcbAsignado = unPCB;
	clienteSeleccionado->fueAsignado = true;
	imprimoInformacionGeneral();
}

void* mensajesPrograma(void){

	while(1){
		sem_wait(&semMensajeImpresion);
		t_imprimibles* msjCliente;
		msjCliente=queue_pop(colaImprimibles);
		t_socket_pid *aux;
		log_info(ptrLog,"Ingreso un nuevo mensaje que sera impreso por el Programa %u",msjCliente->PCB_ID);

		bool _encuentraCliente(t_socket_pid* socket_pid){
			return socket_pid->pid==msjCliente->PCB_ID;
		}

		aux=list_find(listaSocketsConsola,(void*) _encuentraCliente);
		if (aux==NULL) {
			free(msjCliente);
			log_error(ptrLog,"No se llego a imprimir porque el programa habia sido cerrado");
			return 0;
		}
		switch(msjCliente->tipoDeValor){
			case IMPRIMIR_TEXTO:
				if((enviarDatos(aux->socket,&msjCliente->valor,strlen(msjCliente->valor)+1,IMPRIMIR_TEXTO)) < 0){
					log_error(ptrLog,"Error al enviar un mensaje a imprimir:TEXTO por el Programa: %u",aux->pid);
				}else{
					log_debug(ptrLog,"Mensaje de tipo:TEXTO enviado al Programa: %u con éxito!",aux->pid);
				}
				break;
			case IMPRIMIR_VALOR:
				if((enviarDatos(aux->socket,&msjCliente->valor,sizeof(uint32_t),IMPRIMIR_VALOR)) < 0){
					log_error(ptrLog,"Error al enviar un mensaje a imprimir de tipo:VALOR por el Programa: %u",aux->pid);
				}else{
					log_debug(ptrLog,"Mensaje de tipo:VALOR enviado al Programa: %u con éxito!",aux->pid);
				}
				break;
			case EXIT:
				aux->terminado=true;
				if((enviarDatos(aux->socket,&msjCliente->valor,strlen(msjCliente->valor)+1,EXIT)) < 0){
					log_error(ptrLog,"Error al enviar un mensaje a imprimir de tipo:EXIT por el Programa: %u",aux->pid);
				}else{
					log_debug(ptrLog,"Mensaje de tipo:EXIT enviado al Programa: %u con éxito!",aux->pid);
				}

				pthread_mutex_unlock(&mutex_exit);
				break;
			case ERROR:
				aux->terminado=true;
				if((enviarDatos(aux->socket,&msjCliente->valor,strlen(msjCliente->valor)+1,ERROR)) < 0){
					log_error(ptrLog,"Error al enviar un mensaje a imprimir de tipo:ERROR por el Programa: %u",aux->pid);
				}else{
					log_debug(ptrLog,"Mensaje de tipo:ERROR enviado al Programa: %u con éxito!",aux->pid);
				}
				pthread_mutex_unlock(&mutex_exit);
				break;
			default:
				log_error(ptrLog,"El tipo de mensaje ingresado no es soportado por la interface");
				break;
		}
		free(msjCliente);
	}

	return 0;
}

int main() {
	colaNew=list_create();					// Lista para procesos en estado NEW
	colaReady=queue_create();				// Cola para procesos en estado READY
	colaExit=queue_create();				// Cola para procesos en estado EXIT
	colaImprimibles=queue_create();
	listaSocketsCPUs = list_create();
	listaSocketsConsola = list_create();
	colaBloqueados= queue_create();// Cola para valores a imprimir

	sem_init(&semNuevoProg,1,0);		// Para sacar de la cola New solo cuando se manda señal de que se creó uno
	sem_init(&semProgExit,1,0);		// Misma funcion que semNuevoProg pero para los prog en Exit
	sem_init(&semCpuOciosa, 1, 0);		// Semaforo para clientes ociosos
	sem_init(&semNuevoPcbColaReady,1,0);// Semaforo que avisa cuando la cola de ready le agregan un PCB
	sem_init(&semMensajeImpresion,1,0);// Semaforo para revisar los mensajes a mandar a los programas para imprimir
	sem_init(&semProgramaFinaliza,1,0);
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

		returnInt = enviarMensajeAUMC("Hola, soy el Nucleo/0");
		if(pthread_create(&threadSocket, NULL, (void*)escucharPuertos, NULL)!= 0)
		{
			perror("could not create thread");
		}
		else
		{
			log_debug(ptrLog, "Hilo para escuchar puertos creado");
		}
		if (pthread_create(&hiloCpuOciosa, NULL, (void*) hiloClienteOcioso,
					NULL ) != 0) {
				perror("could not create thread");
			} else {
				log_debug(ptrLog, "Hilo cliente ocioso creado");
		}
		if (pthread_create(&threadPlanificador, NULL, (void*) mensajesPrograma,	NULL ) != 0) {
			perror("could not create thread");
		} else{
			log_debug(ptrLog, "Hilo para enviar mensajes a clientes creado");
		}
		while (1) {
				sem_wait(&semNuevoProg);/*Este 1er semaforo espera que se cree un nuevo pcb y que mande signal para recien
				 poder enviarlo a la cola de Ready*/
				log_debug(ptrLog,
						"Se procede a pasar un pcb en estado New a la cola Ready");
				t_pcb *aux = list_remove(colaNew, 0);
				log_debug(ptrLog, "Proceso %u pasa a la cola READY", aux->pcb_id);
				queue_push(colaReady, aux);
				sem_post(&semNuevoPcbColaReady);
		}
		pthread_join(threadSocket, NULL);
		pthread_join(hiloCpuOciosa, NULL);
		pthread_join(threadPlanificador, NULL);

	} else {
		log_info(ptrLog, "El Nucleo no pudo inicializarse correctamente");
		return -1;
	}

	finalizarConexion(socketUMC);
	finalizarConexion(socketReceptorConsola);
	finalizarConexion(socketReceptorCPU);

	return returnInt;

}
