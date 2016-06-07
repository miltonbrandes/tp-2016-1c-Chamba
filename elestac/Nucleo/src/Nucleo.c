/*

 * nucleo.c
 * */
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/string.h>
#include <sockets/ServidorFunciones.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>
#include <sockets/StructsUtiles.h>
#include <sockets/OpsUtiles.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/collections/queue.h>
#include <parser/metadata_program.h>
#include "Nucleo.h"

t_log* ptrLog;

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
	while (idIO[ivar] != NULL) {
		entradaSalida = malloc(sizeof(t_IO));
		entradaSalida->nombre = idIO[ivar];
		entradaSalida->valor = atoi(values_IO[ivar]);
		entradaSalida->ID_IO = ivar;
		entradaSalida->colaSolicitudes = queue_create();
		sem_init(&entradaSalida->semaforo, 1, 0);
		list_add(listaDispositivosIO, entradaSalida);
		log_trace(ptrLog, "(\"%s\", valor:%d, ID:%d)", entradaSalida->nombre,
				entradaSalida->valor, entradaSalida->ID_IO);
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
		if (config_has_property(config, "TAMANIO_STACK")) {
			tamanioStack = config_get_int_value(config, "TAMANIO_STACK");

		} else {
			log_info(ptrLog,
					"el archivo de configuracion no tiene la clave stack");
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

int obtenerSocketMaximoInicial() {
	int socketMaximoInicial = 0;

	if (socketReceptorCPU > socketReceptorConsola) {
		if (socketReceptorCPU > socketUMC) {
			socketMaximoInicial = socketReceptorCPU;
		} else if (socketUMC > socketReceptorCPU) {
			socketMaximoInicial = socketUMC;
		}
	} else if (socketReceptorConsola > socketUMC) {
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

	buffer = recibirDatos(socket, &operacion, &id);
	int bytesRecibidos = strlen(buffer);

	if (bytesRecibidos < 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos en un Socket CPU");
	} else if (bytesRecibidos == 0) {
		log_info(ptrLog, "No se recibieron datos en el Socket CPU");
	} else {
		if (strcmp("ERROR", buffer) == 0) {

		} else {
			log_info(ptrLog, "Bytes recibidos desde una CPU: %s", buffer);
			char *mensajeParaCPU = "Este es un mensaje para vos, CPU\0";
			int bytesEnviados = enviarDatos(nuevoSocketConexion, buffer,
					(uint32_t) strlen(mensajeParaCPU), operacion, id);

			if (bytesEnviados < 0) {
				log_error(ptrLog, "Error al enviar datos a cpu");
			}
		}
	}
}

int datosEnSocketUMC() {
	uint32_t id;
	uint32_t operacion;

	char * buffer = recibirDatos(socketUMC, &operacion, &id);

	if (strcmp("ERROR", buffer) == 0) {
		finalizarConexion(socketUMC);
		return -1;
	} else {
		tamanioMarcos = deserializarUint32(buffer);
		log_info(ptrLog, "Tamanio Pagina: %d\n", tamanioMarcos);
	}

	free(buffer);
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
		log_debug(ptrLog, "Cliente quiere asignar valor a variable compartida");
		//Asigno el valor a la variable
		varCompartida = deserializar_opVarCompartida(buffer);
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
		enviarDatos(socketCliente, valorEnBuffer, sizeof(uint32_t), NOTHING, NUCLEO);
		free(valorEnBuffer);
		free(bufferLeerVarCompartida);
		break;
	}
	pthread_mutex_unlock(&mutex_cpu);
}

void imprimirProcesoNew(t_pcb* pcb) {
	log_debug(ptrLog, "Proceso %d en estado NEW \n",pcb->pcb_id);
}

void cerrarConexionCliente(t_clienteCpu *unCliente) {
	log_debug(ptrLog, "CPU %d ha cerrado la conexión", unCliente->id);
	_Bool _sacarCliente(t_clienteCpu * elCliente) {
		return unCliente->socket == elCliente->socket;
	}
	// Si tenia un pcb asignado, lo mando a la cola d exit para liberar recursos..
	if (unCliente->fueAsignado == true && cpuAcerrar == unCliente->socket) {
		log_debug(ptrLog, "Agregado a la colaExit el PCB del pid: %d", unCliente->pcbAsignado->pcb_id);
		queue_push(colaExit, unCliente->pcbAsignado);
		sem_post(&semProgExit);
		pthread_mutex_unlock(&mutex_exit);
	}
	list_remove_by_condition(listaSocketsCPUs, (void*) _sacarCliente);
	if (cpuAcerrar == unCliente->socket) {
		cpuAcerrar = 0;
	} else {
		sem_wait(&semCpuOciosa);
	}
}

void aceptarConexionEnSocketReceptorCPU(int nuevoSocketConexion) {
	t_clienteCpu *cliente = malloc(sizeof(t_clienteCpu));
	cliente->socket = nuevoSocketConexion;
	cliente->id = (uint32_t) list_size(listaSocketsCPUs);
	cliente->fueAsignado = false;
	cliente->programaCerrado = false;
	list_add(listaSocketsCPUs, cliente);
	estructuraInicial = malloc(sizeof(t_EstructuraInicial));
	estructuraInicial->Quantum = quantum;
	estructuraInicial->RetardoQuantum = quantumSleep;
	t_buffer_tamanio * buffer_tamanio = serializar_EstructuraInicial(estructuraInicial);
	enviarDatos(cliente->socket, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, QUANTUM_PARA_CPU, NUCLEO);
	sem_post(&semCpuOciosa);
	log_debug(ptrLog, "Cliente aceptado y quantum enviado");
	free(estructuraInicial);
	free(buffer_tamanio);
}

void aceptarConexionEnSocketReceptorConsola(int socketConexion) {
	uint32_t id;
	t_pcb *unPcb;
	uint32_t operacion;
	char* buffer = recibirDatos(socketConexion, &operacion, &id);
	if (strcmp("ERROR", buffer) == 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos en un Socket Consola");
	} else {
		log_info(ptrLog, "Recibi lo siguiente de consola: %s", buffer);

		unPcb = crearPCB(buffer, socketUMC);
		if(unPcb == NULL){
			int bytesEnviados = enviarDatos(socketConexion, buffer,(uint32_t) strlen(buffer), ERROR, id);
			if (bytesEnviados < 0) {
				log_error(ptrLog, "Error al enviar los datos");
			}
			return;
		}

		/*Definimos la estructura del cliente con su socket y pid asociado*/
		t_socket_pid* unCliente = malloc(sizeof(t_socket_pid));
		unCliente->pid = unPcb->pcb_id;
		unCliente->socket = socketConexion;
		unCliente->terminado = false;
		list_add(listaSocketsConsola, unCliente);

		if (buffer[0] == -1) {
			log_error(ptrLog, "Hubo un fallo en la creacion del PCB - Causa: Memory Overload");
			char* texto = "Error al crear el pcb correspondiente a tu programa - Causa: Memory Overload";
			t_imprimibles *unMensaje = malloc((sizeof(uint32_t) * 2) + strlen(texto) + 1);
			unCliente->terminado = true;
			unMensaje->PCB_ID = nroProg;
			unMensaje->valor = texto;
			queue_push(colaImprimibles, unMensaje);
			sem_post(&semMensajeImpresion);
		} else if (buffer[0] == -2) {
			log_error(ptrLog, "Hubo un fallo en la creacion del PCB - Causa: Segmentation Fault");
			char* texto = "Error al crear el pcb correspondiente a tu programa - Causa: Segmentation Fault";
			t_imprimibles *unMensaje = malloc((sizeof(uint32_t) * 2) + strlen(texto) + 1);
			unCliente->terminado = true;
			unMensaje->PCB_ID = nroProg;
			unMensaje->valor = texto;
			queue_push(colaImprimibles, unMensaje);
			sem_post(&semMensajeImpresion);
		} else {
			log_debug(ptrLog, "Todo reservado, se procede a ponerlo en la lista New");
			list_add(colaNew, unPcb);
			log_info(ptrLog, "===== Lista de Procesos en cola NEW =====");
			imprimirProcesoNew(unPcb);
			sem_post(&semNuevoProg);
		}
	}
}

void finalizarConexionDeUnSocketEnParticular(int socketFor) {
	finalizarConexion(socketFor);

	bool buscarCpuPorSocket(t_clienteCpu * unCliente) {
		return unCliente->socket == socketFor;
	}
	bool buscarConsolaPorSocket(t_socket_pid* unCliente) {
		return unCliente->socket == socketFor;
	}

	t_clienteCpu* cpu = list_get(list_filter(listaSocketsCPUs, (void *)buscarCpuPorSocket), 0);

	if(cpu != NULL) {
		cerrarConexionCliente(cpu);
		finalizarConexion(socketFor);
		free(cpu);
	}else{
		t_socket_pid *consola = list_remove_by_condition(listaSocketsConsola, (void*)buscarConsolaPorSocket);
		if(consola != NULL) {
			if (consola->terminado == false) {
				log_error(ptrLog, "El programa:%d se ha desconectado", consola->pid);
				pcbAFinalizar = consola->pid;
				sem_post(&semProgramaFinaliza);
			}
			free(consola);
		}
	}
}

void recibirDatosDeSocketCPU(char * buffer, int socketFor, uint32_t operacion) {
	bool buscarCpuPorSocket(t_clienteCpu * unCliente) {
		return unCliente->socket == socketFor;
	}
	t_clienteCpu* unCliente = list_get(list_filter(listaSocketsCPUs, (void *)buscarCpuPorSocket), 0);
	log_info(ptrLog, "Mensaje recibido de la cpu: %i porque: %i", unCliente->id, operacion);
	pthread_mutex_lock(&mutex_cpu);
	comprobarMensajesDeClientes(unCliente, socketFor, operacion, buffer);
	//pthread_mutex_unlock(&mutex_cpu);
}

void escucharPuertos() {

	int socketMaximo = obtenerSocketMaximoInicial(); //descriptor mas grande
	FD_SET(socketReceptorCPU, &sockets);
	FD_SET(socketReceptorConsola, &sockets);
	FD_SET(socketUMC, &sockets);

	while (1) {
		FD_ZERO(&tempSockets);
		tempSockets = sockets;
		log_info(ptrLog, "Esperando conexiones");

		int resultadoSelect = select(socketMaximo + 1, &tempSockets, NULL, NULL, NULL);
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

					aceptarConexionEnSocketReceptorCPU(nuevoSocketConexion);
				}
			} else if (FD_ISSET(socketReceptorConsola, &tempSockets)) {

				int nuevoSocketConexion = AceptarConexionCliente(socketReceptorConsola);
				if (nuevoSocketConexion < 0) {
					log_info(ptrLog, "Ocurrio un error al intentar aceptar una conexion de Consola");
				} else {
					FD_SET(nuevoSocketConexion, &sockets);
					FD_SET(nuevoSocketConexion, &tempSockets);
					socketMaximo = (socketMaximo < nuevoSocketConexion) ? nuevoSocketConexion : socketMaximo;

					aceptarConexionEnSocketReceptorConsola(nuevoSocketConexion);
				}
			} else if (FD_ISSET(socketUMC, &tempSockets)) {
				int returnDeUMC = datosEnSocketUMC();
				if (returnDeUMC == -1) {
					//Aca matamos Socket UMC
					FD_CLR(socketUMC, &sockets);
				}
			} else {
				uint32_t id;
				uint32_t operacion;
				int socketFor;
				for (socketFor = 0; socketFor < (socketMaximo + 1); socketFor++) {
					if (FD_ISSET(socketFor, &tempSockets)) {


						char * buffer = recibirDatos(socketFor, &operacion, &id);
						if (strcmp("ERROR", buffer) == 0) {
							FD_CLR(socketFor, &tempSockets);
							FD_CLR(socketFor, &sockets);
							finalizarConexionDeUnSocketEnParticular(socketFor);
							log_info(ptrLog, "No se recibio ningun byte de un socket que solicito conexion.");
						} else {
							if (id == CPU) {
								recibirDatosDeSocketCPU(buffer, socketFor, operacion);
							}
						}
					}
				}
			}
		}
	}
}

void comprobarMensajesDeClientes(t_clienteCpu *unCliente, int socketFor, uint32_t operacion, char * buffer) {
	unCliente->socket = socketFor;
	if (unCliente->programaCerrado
			&& (operacion == QUANTUM || operacion == EXIT || operacion == IO)) {
		unCliente->fueAsignado = false;
		unCliente->programaCerrado = false;
		sem_post(&semCpuOciosa);
		return;
	}
	if (unCliente->programaCerrado
			&& (operacion == IMPRIMIR_VALOR || operacion == IMPRIMIR_TEXTO
					|| operacion == ASIG_VAR_COMPARTIDA || operacion == SIGNAL))
		return;
	t_pcb* unPCB;
	switch (operacion) {
	case QUANTUM:
		log_debug(ptrLog, "Se ingresa a operacionQuantum");
		log_debug(ptrLog, "Cliente %d envía unPCB", unCliente->id);
		unPCB = deserializar_pcb(buffer); // aca deberia deserializar el pcb mediante lo que lei desde leer.
		pthread_mutex_lock(&mutex);
		queue_push(colaReady, (void*) unPCB);
		pthread_mutex_unlock(&mutex);
		log_debug(ptrLog, "Agregado a la colaReady el PCB del pid: %d",
				unPCB->pcb_id);
		if (cpuAcerrar != unCliente->socket) {
			unCliente->fueAsignado = false;
			sem_post(&semCpuOciosa);
			sem_post(&semNuevoPcbColaReady);
		}else{
			sem_post(&semNuevoPcbColaReady);
			pthread_mutex_unlock(&mutex_cpu);
		}
		free(buffer);
		break;
	case IO:
		log_debug(ptrLog, "Se ingresa a operacionIO");
		t_solicitudes *solicitud = malloc(sizeof(t_solicitudes));
		t_dispositivo_io* opIO;
		log_debug(ptrLog, "Cliente %d envía unPCB", unCliente->id);
		opIO = deserializar_opIO(buffer);

		unPCB = deserializar_pcb((char *) &buffer);
		solicitud->pcb = unPCB;
		solicitud->valor = opIO->tiempo;

		bool _BuscarIO(t_dispositivo_io* element) {
			return (strcmp(opIO->nombre, element->nombre) == 0);
		}
		t_IO * entradaSalida;
		entradaSalida = list_find(listaDispositivosIO, (void*) _BuscarIO);
		if (entradaSalida == NULL) {
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
		if (cpuAcerrar != unCliente->socket) {
			unCliente->fueAsignado = false;
			sem_post(&semCpuOciosa);
		}
		free(buffer);
		break;
	case EXIT:
		log_debug(ptrLog, "Se ingresa a operacionExit");
		log_debug(ptrLog, "CPU %d envía unPCB para desalojar", unCliente->id);
		unPCB = deserializar_pcb(buffer); //debo deserializar el pcb que me envio la cpu
		char* texto = "Se ha finalizado el programa correctamente";
		t_imprimibles *imp = malloc(sizeof(uint32_t)*2+43);
		imp->PCB_ID = unCliente->pcbAsignado->pcb_id;
		imp->tipoDeValor = EXIT;
		imp->valor = texto;
		queue_push(colaImprimibles, imp);
		sem_post(&semMensajeImpresion);
		sem_wait(&semFinalizoDeImprimir);
		log_debug(ptrLog, "Agregado a la colaExit el PCB del pid: %i", unPCB->pcb_id);
		queue_push(colaExit, (void*) unPCB);
		sem_post(&semProgExit);
		pthread_mutex_unlock(&mutex_exit);
		if (cpuAcerrar != unCliente->socket) {
			unCliente->fueAsignado = false;
			sem_post(&semCpuOciosa);
		}
		free(buffer);
		break;
	case IMPRIMIR_VALOR: {
		t_imprimibles* imprimi = malloc(sizeof(t_imprimibles));
		imprimi->tipoDeValor = IMPRIMIR_VALOR;
		imprimi->valor = buffer;
		imprimi->PCB_ID = unCliente->pcbAsignado->pcb_id;
		queue_push(colaImprimibles, imprimi);
		sem_post(&semMensajeImpresion);
		sem_wait(&semFinalizoDeImprimir);
		pthread_mutex_unlock(&mutex_cpu);
		break;
	}
	case IMPRIMIR_TEXTO: {
		t_imprimibles* imprimir = malloc(sizeof(t_imprimibles));
		imprimir->tipoDeValor = IMPRIMIR_TEXTO;
		imprimir->valor = buffer;
		imprimir->PCB_ID = unCliente->pcbAsignado->pcb_id;
		queue_push(colaImprimibles, imprimir);
		sem_post(&semMensajeImpresion);
		sem_wait(&semFinalizoDeImprimir);
		pthread_mutex_unlock(&mutex_cpu);
		break;
	}
	case LEER_VAR_COMPARTIDA:
		operacionesConVariablesCompartidas(LEER_VAR_COMPARTIDA, buffer,
				unCliente->socket);
		break;
	case ASIG_VAR_COMPARTIDA:
		operacionesConVariablesCompartidas(ASIG_VAR_COMPARTIDA, buffer,
				unCliente->socket);
		break;
	case WAIT:
		operacionesConSemaforos(WAIT, buffer, unCliente);
		break;
	case SIGNAL:
		operacionesConSemaforos(SIGNAL, buffer, unCliente);
		break;
	case SIGUSR:
		cpuAcerrar = unCliente->socket;
		free(buffer);
		pthread_mutex_unlock(&mutex_cpu);
		break;
	}
}

void operacionesConSemaforos(uint32_t operacion, char* buffer,
		t_clienteCpu *unCliente) {
	log_debug(ptrLog, "Se ingresa a operacionConSemaforos");
	_Bool _obtenerSemaforo(t_semaforo *unSemaforo) {
		return (strcmp(unSemaforo->nombre, buffer) == 0);
	}
	uint32_t id;
	t_semaforo * semaforo;
	t_pcbBlockedSemaforo * unPcbBlocked;
	char* bufferPCB;
	semaforo = list_find(listaSemaforos, (void*) _obtenerSemaforo);
	switch (operacion) {
	case WAIT:
		if (unCliente->programaCerrado) {
			enviarDatos(unCliente->socket, buffer, sizeof(buffer), ERROR, NUCLEO);
			break;
		}

		if (semaforo->valor > 0) {
			//No se bloquea
			enviarDatos(unCliente->socket, buffer, sizeof(buffer), NOTHING, NUCLEO);
		} else {
			//Se bloquea
			enviarDatos(unCliente->socket, buffer, sizeof(buffer), WAIT, NUCLEO);
			unPcbBlocked = malloc(sizeof(t_pcbBlockedSemaforo));
			bufferPCB = recibirDatos(unCliente->socket, &operacion, &id);
			unPcbBlocked->nombreSemaforo = semaforo->nombre;
			unPcbBlocked->pcb = deserializar_pcb((char *) &bufferPCB);
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
			if (pcbBlockeado != NULL) {
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

t_pcb* crearPCB(char* programa, int socket) {
	log_debug(ptrLog, "Procedo a crear el pcb del programa recibido");
	// lo que esta comentado en esta funcion hay que descomentarlo cuando este hecho el leer y escribir
	t_metadata_program* datos;

	//Obtengo la metadata utilizando el preprocesador del parser
	datos = metadata_desde_literal(programa);

	uint32_t tamanioPCB = 9 * sizeof(uint32_t);
	tamanioPCB += datos->instrucciones_size * (sizeof(t_puntero_instruccion) + sizeof(size_t));
	tamanioPCB += tamanioStack * tamanioMarcos;
	if(datos->cantidad_de_etiquetas == 0 && datos->cantidad_de_funciones == 0){
		//No se suma nada
	}else{
		tamanioPCB += datos->etiquetas_size;
	}
	t_pcb* pcb = malloc(tamanioPCB);

	pthread_mutex_lock(&mutex_pid_counter);
	pcb->pcb_id = nroProg++;
	pthread_mutex_unlock(&mutex_pid_counter);

	log_info(ptrLog, "Solicitamos espacio a UMC para nuevo Proceso AnSISOP");
	t_iniciar_programa * iniciarProg = malloc((sizeof(uint32_t) * 2) + strlen(programa));
	iniciarProg->programID = pcb->pcb_id;
	if(strlen(programa)%tamanioMarcos != 0){
		iniciarProg->tamanio = ((strlen(programa)) / tamanioMarcos) + 1 + tamanioStack;
	}
	else{
		iniciarProg->tamanio = ((strlen(programa) / tamanioMarcos) + tamanioStack);
	}
	iniciarProg->codigoAnsisop = malloc(strlen(programa));
	strcpy(iniciarProg->codigoAnsisop, programa);

	log_debug(ptrLog, "Enviamos a la UMC el codigo del Programa");
	char * rtaEnvio = enviarOperacion(NUEVOPROGRAMA, iniciarProg, socket);
	t_nuevo_prog_en_umc* nuevoProgEnUMC = deserializarNuevoProgEnUMC(rtaEnvio);

	if ((nuevoProgEnUMC->primerPaginaDeProc) > -1) {
		log_error(ptrLog, "Error al tratar de escribir sobre las paginas de codigo");
		programa[0] = -2;
		free(datos);
		free(iniciarProg);
		free(nuevoProgEnUMC);
		queue_push(colaExit, pcb);
		sem_post(&semProgExit);
		return NULL;
	} else {
		log_debug(ptrLog, "Se escribieron las paginas del Proceso AnSISOP %i en UMC y Swap", pcb->pcb_id);

		pcb->paginaCodigoActual = 0;
		pcb->paginaStackActual = iniciarProg->tamanio - tamanioStack;
		pcb->primerPaginaStack = pcb->paginaStackActual;
		pcb->stackPointer = 0;
		pcb->PC = datos->instruccion_inicio;
		pcb->codigo = datos->instrucciones_size;
		t_list * pcbStack = list_create();
		pcb->ind_stack = pcbStack;
		pcb->numeroContextoEjecucionActualStack = 0;
		pcb->tamanioEtiquetas = datos->etiquetas_size;
		//Cargo Indice de Codigo
		t_list * listaIndCodigo = llenarLista(datos->instrucciones_serializado, datos->instrucciones_size);
		pcb->ind_codigo = listaIndCodigo;
		if (datos->cantidad_de_etiquetas > 0 || datos->cantidad_de_funciones > 0) {
			char* indiceEtiquetas = malloc(datos->etiquetas_size);
			indiceEtiquetas = datos->etiquetas;
			pcb->ind_etiq = indiceEtiquetas;
		} else {
			//Harcodeo
			pcb->ind_etiq = NULL;
		}
		free(datos);
		free(iniciarProg);
		free(nuevoProgEnUMC);

		return pcb;
	}
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
		t_clienteCpu *clienteSeleccionado = list_get(
				list_filter(listaSocketsCPUs, (void*) _sinPCB), 0);
		envioPCBaClienteOcioso(clienteSeleccionado);

	}
	return 0;
}

void envioPCBaClienteOcioso(t_clienteCpu *clienteSeleccionado) {
	log_debug(ptrLog, "Obtengo un cliente ocioso que este esperando por un PCB... cliente:%d", clienteSeleccionado->id);
	pthread_mutex_lock(&mutex);
	t_pcb *unPCB = queue_pop(colaReady);
	pthread_mutex_unlock(&mutex);
	log_debug(ptrLog, "Obtengo PCB_ID %d de la colaReady", unPCB->pcb_id);
	t_buffer_tamanio* pcbSer = serializar_pcb(unPCB);

	log_debug(ptrLog, "Serializo el PCB");
	enviarDatos(clienteSeleccionado->socket, pcbSer->buffer, pcbSer->tamanioBuffer, EXECUTE_PCB, NUCLEO);
	log_debug(ptrLog, "PCB enviado al CPU %d", clienteSeleccionado->id);
	clienteSeleccionado->pcbAsignado = unPCB;
	clienteSeleccionado->fueAsignado = true;
	pthread_mutex_unlock(&mutex_cpu);
	free(pcbSer);
}

t_socket_pid * buscarConsolaPorProceso(uint32_t pid) {
	int i;
	for(i = 0; i < list_size(listaSocketsConsola); i++) {
		t_socket_pid * socketPid = list_get(listaSocketsConsola, i);
		if(pid == socketPid->pid) {
			return socketPid;
		}
	}
	return NULL;
}

int indiceConsolaEnLista(uint32_t pid) {
	int i;
	for(i = 0; i < list_size(listaSocketsConsola); i++) {
		t_socket_pid * socketPid = list_get(listaSocketsConsola, i);
		if(pid == socketPid->pid) {
			return i;
		}
	}
	return -1;
}

void* mensajesPrograma(void) {
	while (1) {
		sem_wait(&semMensajeImpresion);
		t_imprimibles* msjCliente; //= malloc(3*sizeof(uint32_t));
		msjCliente = queue_pop(colaImprimibles);
		t_socket_pid *aux;
		log_info(ptrLog, "Ingreso un nuevo mensaje que sera impreso por el Programa %u", msjCliente->PCB_ID);
		aux = buscarConsolaPorProceso(msjCliente->PCB_ID);
		if (aux == NULL) {
			free(msjCliente);
			log_error(ptrLog, "No se llego a imprimir porque el programa habia sido cerrado");
			return 0;
		}
		switch (msjCliente->tipoDeValor) {
		case IMPRIMIR_TEXTO:
			if ((enviarDatos(aux->socket, msjCliente->valor,
					strlen(msjCliente->valor) + 1, IMPRIMIR_TEXTO, NUCLEO)) < 0) {
				log_error(ptrLog,
						"Error al enviar un mensaje a imprimir:TEXTO por el Programa: %u",
						aux->pid);
			} else {
				log_debug(ptrLog,
						"Mensaje de tipo:TEXTO enviado al Programa: %u con éxito!",
						aux->pid);
			}
			free(msjCliente);
			sem_post(&semFinalizoDeImprimir);
			break;
		case IMPRIMIR_VALOR:
			if ((enviarDatos(aux->socket, msjCliente->valor, sizeof(uint32_t),
					IMPRIMIR_VALOR, NUCLEO)) < 0) {
				log_error(ptrLog,
						"Error al enviar un mensaje a imprimir de tipo:VALOR por el Programa: %u",
						aux->pid);
			} else {
				log_debug(ptrLog,
						"Mensaje de tipo:VALOR enviado al Programa: %u con éxito!",
						aux->pid);
			}
			free(msjCliente);
			sem_post(&semFinalizoDeImprimir);
			break;
		case EXIT:
			aux->terminado = true;
			if ((enviarDatos(aux->socket, msjCliente->valor, strlen(msjCliente->valor) + 1, EXIT, NUCLEO)) < 0) {
				log_error(ptrLog, "Error al enviar un mensaje a imprimir de tipo: EXIT por el Programa: %d", aux->pid);
			} else {
				log_debug(ptrLog, "Mensaje de tipo: EXIT del Programa %d enviado con éxito!", aux->pid);
			}
			FD_CLR(aux->socket, &tempSockets);
			FD_CLR(aux->socket, &sockets);
			finalizarConexion(aux->socket);
			list_remove(listaSocketsConsola, indiceConsolaEnLista(aux->pid));
			free(msjCliente);
			sem_post(&semFinalizoDeImprimir);
			break;
		case ERROR:
			aux->terminado = true;
			if ((enviarDatos(aux->socket, msjCliente->valor,
					strlen(msjCliente->valor) + 1, ERROR, NUCLEO)) < 0) {
				log_error(ptrLog,
						"Error al enviar un mensaje a imprimir de tipo:ERROR por el Programa: %u",
						aux->pid);
			} else {
				log_debug(ptrLog,
						"Mensaje de tipo:ERROR enviado al Programa: %u con éxito!",
						aux->pid);
			}
			free(aux);
			free(msjCliente);
			pthread_mutex_unlock(&mutex_exit);
			sem_post(&semFinalizoDeImprimir);
			break;
		default:
			log_error(ptrLog,
					"El tipo de mensaje ingresado no es soportado por la interface");
			free(aux);
			free(msjCliente);
			break;

		}
	}
	return 0;
}

void* vaciarColaExit(){
	while(1){
		sem_wait(&semProgExit);/*Esperamos a que se envie algun PCB a la cola exit*/
		pthread_mutex_lock(&mutex_exit);
		t_pcb *aux=queue_pop(colaExit);
		pthread_mutex_unlock(&mutex_exit);
		t_finalizar_programa * finalizarProg = malloc(sizeof(t_finalizar_programa));
		finalizarProg->programID = aux->pcb_id;
		if((enviarOperacion(FINALIZARPROGRAMA, finalizarProg, socketUMC)) < 0){
			log_error(ptrLog,"Error al borrar las paginas del pcb con pid: %d",aux->pcb_id);
		}else{
			log_info(ptrLog,"Borrados las paginas del programa:%d",aux->pcb_id);
		}
		free(aux);
		pthread_mutex_unlock(&mutex_cpu);
		pthread_mutex_unlock(&mutex_exit);
	}
	return 0;
}

void *hiloPorIO(void* es) {
	t_IO *entradaSalida = (t_IO*) es;
	t_solicitudes *solicitud;
	log_debug(ptrLog, "%s: Hilo creado, (ID:%d, colaSize:%d)",
			entradaSalida->nombre, entradaSalida->ID_IO,
			queue_size(entradaSalida->colaSolicitudes));
	while (1) {
		//aca el if estaria de mas, despues sacarlo
		if (queue_is_empty(entradaSalida->colaSolicitudes)) {
			log_debug(ptrLog, "%s: Me quedo a la espera de solicitudes",
					entradaSalida->nombre);
			sem_wait(&entradaSalida->semaforo);
			log_debug(ptrLog,
					"\nPase semaforo entradaSalidaSemaforo ID:%s\n",
					entradaSalida->nombre);

		} else {
			solicitud = queue_pop(entradaSalida->colaSolicitudes);
			log_trace(ptrLog, "%s: Solicitud recibida, valor:%d",
					entradaSalida->nombre, solicitud->valor);
			log_trace(ptrLog, "%s: Hago retardo de :%d milisegundos",
					entradaSalida->nombre,
					solicitud->valor * entradaSalida->valor);
			usleep(solicitud->valor * entradaSalida->valor / 1000);
			queue_push(colaReady, solicitud->pcb);
			sem_post(&semNuevoPcbColaReady);
			log_debug(ptrLog, "%s: Envio PCB ID:%d a colaReady",
					entradaSalida->nombre, solicitud->pcb->pcb_id);
			free(solicitud);
		}
		pthread_mutex_unlock(&mutex_cpu);
	}
	return 0;
}

void hilosIO(void* entradaSalida)
{
	if(pthread_create(&hiloIO, NULL, hiloPorIO, (void *)entradaSalida) != 0){
		perror("no se puede crear los threads de entrada salida");
	}
}

void* hiloPCBaFinalizar() {
	while (1) {
		sem_wait(&semProgramaFinaliza);
		log_debug(ptrLog, "Debido a que se detuvo el programa %d, se envia el pcb a la cola de Exit", pcbAFinalizar);

		bool obtenerDeBloqueados(t_pcbBlockedSemaforo* pcbBlocked) {
			return pcbBlocked->pcb->pcb_id == pcbAFinalizar;
		}

		bool obtenerDeReady(t_pcb* pcbReady) {
			return pcbReady->pcb_id == pcbAFinalizar;
		}

		bool obtenerDeClientes(t_clienteCpu* cliente) {
			return cliente->pcbAsignado->pcb_id == pcbAFinalizar;
		}

		while (1) {

			t_pcbBlockedSemaforo* pcbBlock = list_remove_by_condition(colaBloqueados, (void*) obtenerDeBloqueados);
			if (pcbBlock != NULL ) {
				queue_push(colaExit, pcbBlock->pcb);
				sem_post(&semProgExit);
				pthread_mutex_unlock(&mutex_exit);
				log_debug(ptrLog, "El pcb se encontraba en la cola de bloqueados");
				break;
			}

			t_pcb* pcb = list_remove_by_condition(colaReady->elements, (void*) obtenerDeReady);
			if (pcb != NULL ) {
				queue_push(colaExit, pcb);
				sem_post(&semProgExit);
				pthread_mutex_unlock(&mutex_exit);
				log_debug(ptrLog, "El pcb se encontraba en la cola de ready");
				break;
			}

			t_clienteCpu* cliente = list_find(listaSocketsCPUs, (void*) obtenerDeClientes);
			if (cliente != NULL ) {
				cliente->programaCerrado = true;
				cliente->fueAsignado=false;
				queue_push(colaExit, cliente->pcbAsignado);
				sem_post(&semProgExit);
				pthread_mutex_unlock(&mutex_exit);
				log_debug(ptrLog, "El programa se encontraba en ejecucion");
				break;
			}

			usleep(2);
		}
	}
	return 0;
}

int abrirSocketsImportantes() {
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

	return 1;
}

int crearThreadsUtiles() {
	if (pthread_create(&threadSocket, NULL, (void*) escucharPuertos, NULL) != 0) {
		perror("no se pudo crear el thread para escuchar las nuevas conexiones y las respuestas de cpu");
		return -1;
	} else {
		log_debug(ptrLog, "Hilo para escuchar puertos creado");
	}
	if (pthread_create(&hiloCpuOciosa, NULL, (void*) hiloClienteOcioso, NULL) != 0) {
		perror("no se pudo crear el thread para planificar a que cpu mandar un nuevo prog");
		return -1;
	} else {
		log_debug(ptrLog, "Hilo cliente ocioso creado");
	}
	if (pthread_create(&threadPlanificador, NULL, (void*) mensajesPrograma, NULL) != 0) {
		perror("no se pudo crear el thread para enviar mensajes a consoals");
		return -1;
	} else {
		log_debug(ptrLog, "Hilo para enviar mensajes a clientes creado");
	}
	if (pthread_create(&threadExit, NULL, vaciarColaExit, NULL)!= 0) {
		perror("No se pudo crear el thread para ir vaciando la cola de programas a finalizar");
		return -1;
	}else{
		log_debug(ptrLog, "Hilo para vaciar los programas finalizados creado");
	}
	//hilo creado para programas aque se cierren inesperadamente.
	if (pthread_create(&hiloPcbFinalizarError, NULL, (void*) hiloPCBaFinalizar, NULL ) != 0) {
		perror("no se pudo crear el thread para finalizar programas inesperadamente");
		return -1;
	}else{
		log_debug(ptrLog, "Hilo para finalizar los programas que tuvieron un fin inesperado");
	}

	return 1;
}

int main() {

	colaNew = list_create();				// Lista para procesos en estado NEW
	colaReady = queue_create();			// Cola para procesos en estado READY
	colaExit = queue_create();				// Cola para procesos en estado EXIT
	colaImprimibles = queue_create();		// Cola para valores a imprimir
	listaSocketsCPUs = list_create();		//lista de cpus conectadas
	listaSocketsConsola = list_create();	//lista de consolas conectadas
	colaBloqueados = list_create();			//lista de procesos bloqueados

	sem_init(&semNuevoProg, 1, 0);// Para sacar de la cola New solo cuando se manda señal de que se creó uno
	sem_init(&semProgExit, 1, 0);// Misma funcion que semNuevoProg pero para los prog en Exit
	sem_init(&semCpuOciosa, 1, 0);		// Semaforo para clientes ociosos
	sem_init(&semFinalizoDeImprimir, 1, 0); //semaforo para ver si termino de imprimir
	sem_init(&semNuevoPcbColaReady, 1, 0);// Semaforo que avisa cuando la cola de ready le agregan un PCB
	sem_init(&semMensajeImpresion, 1, 0);// Semaforo para revisar los mensajes a mandar a los programas para imprimir
	sem_init(&semProgramaFinaliza, 1, 0);	//semaforo que avisa cuando hay un programa que se finalizo inesperadamente
	int returnInt = EXIT_SUCCESS;
	if (init()) {
		list_iterate(listaDispositivosIO, (void*) hilosIO);

		if (abrirSocketsImportantes()) {
			if(crearThreadsUtiles()) {
				while (1) {
					sem_wait(&semNuevoProg);/*Este 1er semaforo espera que se cree un nuevo pcb y que mande signal para recien
					 poder enviarlo a la cola de Ready*/
					log_debug(ptrLog, "Se procede a pasar un pcb en estado New a la cola Ready");
					t_pcb * aux = list_get(colaNew, 0);
					list_remove(colaNew, 0);
					log_debug(ptrLog, "Proceso %u pasa a la cola READY", aux->pcb_id);

					pthread_mutex_lock(&mutex);
					queue_push(colaReady, aux);
					pthread_mutex_unlock(&mutex);

					sem_post(&semNuevoPcbColaReady);
				}
			}else{
				returnInt = EXIT_FAILURE;
			}

			pthread_join(threadSocket, NULL);
			pthread_join(hiloCpuOciosa, NULL);
			pthread_join(threadPlanificador, NULL);
			pthread_join(hiloPcbFinalizarError, NULL);
			pthread_join(threadExit,NULL);
		}else{
			returnInt = EXIT_FAILURE;
		}

	} else {
		log_info(ptrLog, "El Nucleo no pudo inicializarse correctamente");
		returnInt = EXIT_FAILURE;
	}

	finalizarConexion(socketUMC);
	finalizarConexion(socketReceptorConsola);
	finalizarConexion(socketReceptorCPU);

	return returnInt;

}
