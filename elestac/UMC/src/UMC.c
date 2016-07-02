/*
 * UMC.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <commons/log.h>
#include <netinet/in.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include <sockets/ServidorFunciones.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>
#include <sockets/StructsUtiles.h>
#include <pthread.h>
#include <semaphore.h>
#include "UMC.h"

//Socket que recibe conexiones de Nucleo y CPU
int socketReceptorNucleo;
int socketReceptorCPU;
int socketSwap;
int socketClienteNucleo;
//Archivo de Log
t_log* ptrLog;

//Variables de configuracion
int puertoTCPRecibirConexionesCPU;
int puertoTCPRecibirConexionesNucleo;
int puertoReceptorSwap;
uint32_t marcos, marcosSize, marcoXProc, entradasTLB, retardo;
char *algoritmoReemplazo;
char *ipSwap;
int i = 0;
t_config* config;

//Variables frames, tlb, tablas
t_list * listaCpus;
t_list * frames;
t_list * tablaProcesosPaginas;
t_list * TLB;

//Variables Hilos
pthread_t hiloConexiones;
pthread_t hiloCpu;
pthread_t hiloNucleo;
pthread_t hiloConsola;
//variables semaforos

int main() {
	if (init()) {
		tablaProcesosPaginas = list_create();
		frames = list_create();

		iniciarTLB();

		reservarMemoria(marcos, marcosSize);

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
			printf("No se pudo abrir el Socket Servidor Nucleo de UMC\n");
			return -1;
		}
		log_info(ptrLog, "Se abrio el socket Servidor Nucleo de UMC");
		printf("Se abrio el socket Servidor Nucleo de UMC\n");

		socketReceptorCPU = AbrirSocketServidor(puertoTCPRecibirConexionesCPU);
		if (socketReceptorCPU < 0) {
			log_info(ptrLog,
					"No se pudo abrir el Socket Servidor Nucleo de CPU");
			printf("No se pudo abrir el Socket Servidor Nucleo de CPU\n");
			return -1;
		}
		log_info(ptrLog, "Se abrio el socket Servidor Nucleo de CPU");
		pthread_create(&hiloConexiones, NULL,
				(void *) manejarConexionesRecibidas, NULL);

		crearConsola();

	} else {
		log_info(ptrLog, "La UMC no pudo inicializarse correctamente");
		printf("La UMC no pudo inicializarse correctamente\n");
		return -1;
	}
	log_info(ptrLog, "Proceso UMC finalizado");
	printf("Proceso UMC finalizado\n");

	free(config);
	finalizarTLB();
	finalizarConexion(socketSwap);
	finalizarConexion(socketReceptorNucleo);
	finalizarConexion(socketReceptorCPU);
	return EXIT_SUCCESS;
}

////FUNCIONES UMC////

char * enviarYRecibirMensajeSwap(t_buffer_tamanio * bufferTamanio,
		uint32_t operacion) {
	pthread_mutex_lock(&comunicacionConSwap);
	char * mensajeDeSwap;

	int bytesEnviados = enviarDatos(socketSwap, bufferTamanio->buffer,
			bufferTamanio->tamanioBuffer, operacion, UMC);
	if (bytesEnviados <= 0) {
		log_error(ptrLog, "Ocurrio un error al enviarle un mensaje a Swap.");
		printf("Ocurrio un error al enviarle un mensaje a Swap.\n");
		mensajeDeSwap = NULL;
	} else {
		uint32_t operacion, id;
		mensajeDeSwap = recibirDatos(socketSwap, &operacion, &id);
	}
	pthread_mutex_unlock(&comunicacionConSwap);
	return mensajeDeSwap;
}

void reservarMemoria(int cantidadMarcos, int tamanioMarco) {
	int i;
	for (i = 0; i < cantidadMarcos; i++) {
		t_frame * frame = malloc((sizeof(uint32_t) * 3) + tamanioMarco);
		frame->disponible = 1;
		frame->numeroFrame = i;
		frame->contenido = malloc(tamanioMarco);

		list_add(frames, frame);
	}
}

void enviarMensajeASwap(char *mensajeSwap, int tamanioMensaje, int operacion) {
	pthread_mutex_lock(&comunicacionConSwap);
	enviarDatos(socketSwap, mensajeSwap, tamanioMensaje, operacion, UMC);
	pthread_mutex_unlock(&comunicacionConSwap);
}

//Metodos para Iniciar valores de la UMC
int crearLog() {
	ptrLog = log_create(getenv("UMC_LOG"), "UMC", 0, 0);
	if (ptrLog) {
		return 1;
	} else {
		return 0;
	}
}

int iniciarUMC(t_config* config) {

	if (config_has_property(config, "MARCOS")) {
		marcos = config_get_int_value(config, "MARCOS");
	} else {
		log_info(ptrLog,
				"El archivo de configuracion no contiene la clave MARCOS");
		return 0;
	}

	if (config_has_property(config, "MARCO_SIZE")) {
		marcosSize = config_get_int_value(config, "MARCO_SIZE");
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

	if (config_has_property(config, "ENTRADAS_TLB")) { // =0 -> deshabilitada
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

	if (config_has_property(config, "ALGORITMO")) {
		algoritmoReemplazo = config_get_string_value(config, "ALGORITMO");
	} else {
		log_info(ptrLog,
				"El archivo de configuracion no contiene la clave ALGORITMO");
		return 0;
	}

	return 1;
}

int cargarValoresDeConfig() {
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

void manejarConexionesRecibidas() {
	listaCpus = list_create();
	socketClienteNucleo = AceptarConexionCliente(socketReceptorNucleo);
	if (socketClienteNucleo < 0) {
		log_info(ptrLog,
				"Ocurrio un error al intentar aceptar una conexion de Nucleo");
		printf("Ocurrio un error al intentar aceptar una conexion de Nucleo\n");
	} else {
		log_info(ptrLog, "Se conecto Nucleo");
		printf("Se conecto Nucleo\n");
		enviarTamanioPaginaANUCLEO();
	}
	pthread_create(&hiloNucleo, NULL, (void *) recibirPeticionesNucleo, NULL);
	//recibirPeticionesNucleo();
	log_info(ptrLog, "Esperando conexiones CPU");
//	printf("Esperando conexiones CPU\n");
	pthread_create(&hiloCpu, NULL, (void *) aceptarConexionCpu, NULL);
	//aceptarConexionCpu();

	pthread_join(hiloNucleo, NULL);
	pthread_join(hiloCpu, NULL);
}

void recibirPeticionesNucleo() {
	uint32_t operacion;
	uint32_t id;

	while (1) {

		log_info(ptrLog, "Esperando Peticion de CPU");
//		printf("Esperando conexiones CPU\n");
		char* mensajeRecibido = recibirDatos(socketClienteNucleo, &operacion,
				&id);

		if (strcmp(mensajeRecibido, "ERROR") == 0) {
			log_error(ptrLog,
					"No se recibio nada de Nucleo, se cierra la conexion");
			printf("No se recibio nada de Nucleo, se cierra la conexion\n");
			finalizarConexion(socketClienteNucleo);
			return;
		} else {
			if (operacion == NUEVOPROGRAMA) {
				t_iniciar_programa *iniciarProg = deserializarIniciarPrograma(
						mensajeRecibido);

				log_info(ptrLog,
						"Nucleo quiere iniciar Proceso %d. Chequeamos espacio disponible en Swap.",
						iniciarProg->programID);
				printf(
						"Nucleo quiere iniciar Proceso %d. Chequeamos espacio disponible en Swap.\n",
						iniciarProg->programID);

				uint32_t pudoSwap = checkDisponibilidadPaginas(iniciarProg); //pregunto a swap si tiene paginas

				if (pudoSwap == SUCCESS) {
					log_info(ptrLog, "Proceso %d almacenado.",
							iniciarProg->programID);
					printf("Proceso %d almacenado.\n", iniciarProg->programID);
					log_info(ptrLog, "Cantidad de Procesos Actuales: %d",
							list_size(tablaProcesosPaginas) + 1);
					printf("Cantidad de Procesos Actuales: %d\n",
							list_size(tablaProcesosPaginas) + 1);
					t_nuevo_prog_en_umc * iniciarPrograma = inicializarProceso(
							iniciarProg);
					notificarProcesoIniciadoANucleo(iniciarPrograma);
				} else {
					operacion = ERROR;
					log_info(ptrLog,
							"No hay espacio, no inicializa el Proceso con PID: %d",
							iniciarProg->programID);
					printf(
							"No hay espacio, no inicializa el Proceso con PID: %d\n",
							iniciarProg->programID);
					notificarProcesoNoIniciadoANucleo();
				}

				free(iniciarProg->codigoAnsisop);
				free(iniciarProg);
			} else if (operacion == FINALIZARPROGRAMA) {
				t_finalizar_programa *finalizar = deserializarFinalizarPrograma(
						mensajeRecibido); //deserializar finalizar
				uint32_t PID = finalizar->programID;
				log_info(ptrLog,
						"Nucleo ordena finalizar el Proceso con PID: %i", PID);
				printf("Nucleo ordena finalizar el Proceso con PID: %i\n", PID);
				finalizarPrograma(PID);

			} else {
				operacion = ERROR;
				log_error(ptrLog, "No se recibio nada de Nucleo");
			}
			free(mensajeRecibido);
		}
	}
}

void notificarProcesoIniciadoANucleo(t_nuevo_prog_en_umc * nuevoPrograma) {
	t_buffer_tamanio * mensaje = serializarNuevoProgEnUMC(nuevoPrograma);
	int enviarRespuesta = enviarDatos(socketClienteNucleo, mensaje->buffer,
			mensaje->tamanioBuffer, NOTHING, UMC);
	free(mensaje->buffer);
	free(mensaje);
	free(nuevoPrograma);
	if (enviarRespuesta <= 0) {
		log_error(ptrLog, "Error al notificar a Nucleo de Proceso Iniciado");
		printf("Error al notificar a Nucleo de Proceso Iniciado\n");
	}
}

void notificarProcesoNoIniciadoANucleo() {
	t_nuevo_prog_en_umc * nuevoPrograma = malloc(sizeof(t_nuevo_prog_en_umc));
	nuevoPrograma->primerPaginaDeProc = -1;
	nuevoPrograma->primerPaginaStack = -1;

	t_buffer_tamanio * mensaje = serializarNuevoProgEnUMC(nuevoPrograma);
	int enviarRespuesta = enviarDatos(socketClienteNucleo, mensaje->buffer,
			mensaje->tamanioBuffer, NOTHING, UMC);
	free(mensaje->buffer);
	free(mensaje);
	if (enviarRespuesta <= 0) {
		log_error(ptrLog, "Error al notificar a Nucleo de Proceso No Iniciado");
		printf("Error al notificar a Nucleo de Proceso No Iniciado\n");
	}
}

t_nuevo_prog_en_umc * inicializarProceso(t_iniciar_programa *iniciarProg) {
	t_list * tablaDePaginas = list_create();

	int i;
	for (i = 0; i < iniciarProg->tamanio; i++) {
		t_registro_tabla_de_paginas * registro = crearRegistroPag(i, -1, 0, 0);
		list_add(tablaDePaginas, registro);
	}

	t_tabla_de_paginas * tablaDeUnProceso = malloc(
			(sizeof(uint32_t) * marcoXProc) + (sizeof(uint32_t) * 2)
					+ (sizeof(t_registro_tabla_de_paginas)
							* list_size(tablaDePaginas)));
	tablaDeUnProceso->pID = iniciarProg->programID;
	tablaDeUnProceso->posibleProximaVictima = 0;
	tablaDeUnProceso->tablaDePaginas = tablaDePaginas;
	tablaDeUnProceso->paginasEnUMC = list_create();
//	for (i = 0; i < marcoXProc; i++) {
//		list_add(tablaDeUnProceso->paginasEnUMC, -1);
//	}

	list_add(tablaProcesosPaginas, tablaDeUnProceso);

	t_nuevo_prog_en_umc * nuevoPrograma = malloc(sizeof(t_nuevo_prog_en_umc));
	nuevoPrograma->primerPaginaDeProc = 0;
	nuevoPrograma->primerPaginaStack = iniciarProg->tamanio - 2;

	enviarPaginasASwap(iniciarProg);

	return nuevoPrograma;
}

void enviarPaginasASwap(t_iniciar_programa * iniciarProg) {
	int i, offset = 0;
	int hastaAcaCodigo;
	uint32_t longitudCodigo = strlen(iniciarProg->codigoAnsisop);
	for (i = 0; i < iniciarProg->tamanio; i++) {
		t_escribir_en_swap * escribirEnSwap = malloc(
				(sizeof(uint32_t) * 2) + marcosSize);
		escribirEnSwap->paginaProceso = i;
		escribirEnSwap->pid = iniciarProg->programID;

		if (offset + marcosSize < longitudCodigo) {
			escribirEnSwap->contenido = malloc(marcosSize);
			memcpy(escribirEnSwap->contenido,
					(iniciarProg->codigoAnsisop) + offset, marcosSize);
			offset += marcosSize;
		} else {
			uint32_t longitudACopiar = longitudCodigo - offset;
			escribirEnSwap->contenido = malloc(longitudACopiar);
			memcpy(escribirEnSwap->contenido,
					(iniciarProg->codigoAnsisop) + offset, longitudACopiar);
			hastaAcaCodigo = i;
			i = iniciarProg->tamanio;
		}

		t_buffer_tamanio * buffer_tamanio = serializarEscribirEnSwap(
				escribirEnSwap, marcosSize);
		char * resultadoSwap = enviarYRecibirMensajeSwap(buffer_tamanio,
				ESCRIBIR);
		usleep(10);
		free(resultadoSwap);
		free(escribirEnSwap->contenido);
		free(escribirEnSwap);
		free(buffer_tamanio->buffer);
		free(buffer_tamanio);
	}
	int a = 0;
	for (a = hastaAcaCodigo + 1; a < iniciarProg->tamanio; a++) {
		t_escribir_en_swap * escribirEnSwap = malloc(
				(sizeof(uint32_t) * 2) + marcosSize);
		escribirEnSwap->paginaProceso = a;
		escribirEnSwap->pid = iniciarProg->programID;
		escribirEnSwap->contenido = malloc(marcosSize);
		t_buffer_tamanio * buffer_tamanio = serializarEscribirEnSwap(
				escribirEnSwap, marcosSize);
		char * resultadoSwap = enviarYRecibirMensajeSwap(buffer_tamanio,
				ESCRIBIR);
		free(resultadoSwap);
		free(escribirEnSwap->contenido);
		free(escribirEnSwap);
		free(buffer_tamanio->buffer);
		free(buffer_tamanio);
	}
}

t_registro_tabla_de_paginas * crearRegistroPag(int pagina, int marco,
		int presencia, int modificado) {
	t_registro_tabla_de_paginas * regPagina = malloc(
			sizeof(t_registro_tabla_de_paginas));

	regPagina->paginaProceso = pagina;
	regPagina->frame = marco;
	regPagina->modificado = modificado;
	regPagina->estaEnUMC = presencia;
	regPagina->bitDeReferencia = 0;
	regPagina->esStack = 0;

	return regPagina;
}

void aceptarConexionCpu() {
	while (1) {
		pthread_t hiloEscuchaCpu;

		int socketCpu = AceptarConexionCliente(socketReceptorCPU);
		if (socketCpu < 0) {
			log_info(ptrLog,
					"Ocurrio un error al intentar aceptar una conexion de CPU");
			printf(
					"Ocurrio un error al intentar aceptar una conexion de CPU\n");
		} else {
			log_info(ptrLog, "Nueva conexion de CPU");
			printf("Nueva conexion de CPU!!\n");
			enviarTamanioPaginaACPU(socketCpu);

			t_cpu* cpu = malloc(sizeof(t_cpu));
			uint32_t num = list_size(listaCpus);
			cpu->socket = socketCpu;
			cpu->numCpu = num + 1;
			cpu->procesoActivo = -1;
			pthread_create(&hiloEscuchaCpu, NULL, (void*) recibirPeticionesCpu,
					cpu);
			cpu->hiloCpu = hiloEscuchaCpu; // MATAR ESTE HILO ptheradExit y free(cpu) borrar cpu lista.
			list_add(listaCpus, cpu);
		}
	}
}

void borrarCpuMuerta(t_cpu* cpu) {
	int i, indiceDeCpu;

	for (i = 0; i < list_size(listaCpus); i++) {
		t_cpu* cpuDeLista = list_get(listaCpus, i);

		if (cpu->numCpu == cpuDeLista->numCpu) {
			indiceDeCpu = i;
		}
	}
	list_remove(listaCpus, indiceDeCpu);
	free(cpu);
}

void recibirPeticionesCpu(t_cpu * cpuEnAccion) {
	uint32_t operacion;
	uint32_t id;
	uint32_t socketCpu = cpuEnAccion->socket;

	while (1) {
		log_debug(ptrLog, "Esperando solicitud de CPU %d", cpuEnAccion->numCpu);
//		printf("Esperando solicitud de CPU %d\n", cpuEnAccion->numCpu);
		char* mensajeRecibido = recibirDatos(socketCpu, &operacion, &id);

		if (strcmp(mensajeRecibido, "ERROR") == 0) {
			log_error(ptrLog,
					"No se recibio ningun dato de CPU %d. Cierro conexion",
					cpuEnAccion->numCpu);
			printf("No se recibio ningun dato de CPU %d. Cierro conexion\n",
					cpuEnAccion->numCpu);
			finalizarConexion(socketCpu);
			pthread_join(cpuEnAccion->hiloCpu, NULL);
			borrarCpuMuerta(cpuEnAccion);
			break;
		} else {
			if (operacion == LEER || operacion == LEER_VALOR_VARIABLE) {
				t_solicitarBytes *leer = deserializarSolicitarBytes(
						mensajeRecibido);

				uint32_t pagina = leer->pagina;
				uint32_t start = leer->start;
				uint32_t offset = leer->offset;
				log_debug(ptrLog,
						"CPU %d solicita lectura -> Proceso Activo Actual %d - Pagina %d - Start %d - Offset %d",
						cpuEnAccion->numCpu, cpuEnAccion->procesoActivo, pagina,
						start, offset);
//				printf(
//						"CPU %d solicita lectura -> Proceso Activo Actual %d - Pagina %d - Start %d - Offset %d\n",
//						cpuEnsssssss
				enviarDatoACPU(cpuEnAccion, pagina, start, offset, operacion);

				free(leer);
			} else if (operacion == ESCRIBIR) {
				t_enviarBytes *escribir = deserializarEnviarBytes(
						mensajeRecibido);

				uint32_t pagina = escribir->pagina;
				uint32_t tamanio = escribir->tamanio;
				uint32_t offset = escribir->offset;
				char* buffer = escribir->buffer;
				log_debug(ptrLog,
						"CPU %d solicita escritura -> Proceso Activo Actual %d - Pagina %d - Tamanio %d - Offset %d - Buffer: %s",
						cpuEnAccion->numCpu, cpuEnAccion->procesoActivo, pagina,
						tamanio, offset, buffer);
//				printf(
//						"CPU %d solicita escritura -> Proceso Activo Actual %d - Pagina %d - Tamanio %d - Offset %d - Buffer: %s\n",
//						cpuEnAccion->numCpu, cpuEnAccion->procesoActivo, pagina,
//						tamanio, offset, buffer);

				escribirDatoDeCPU(cpuEnAccion, pagina, offset, tamanio, buffer);

				free(escribir->buffer);
				free(escribir);
			} else if (operacion == CAMBIOPROCESOACTIVO) {

				if (cpuEnAccion->procesoActivo != -1) {
					tlbFlushDeUnPID(cpuEnAccion->procesoActivo);
				}

				t_cambio_proc_activo *procesoActivo =
						deserializarCambioProcesoActivo(mensajeRecibido);

				uint32_t PID_Activo = procesoActivo->programID;
				log_info(ptrLog,
						"CPU %d notifica Cambio de Proceso Activo. Proceso %d en ejecucion",
						cpuEnAccion->numCpu, PID_Activo);
				printf(
						"CPU %d notifica Cambio de Proceso Activo. Proceso %d en ejecucion\n",
						cpuEnAccion->numCpu, PID_Activo);
				cpuEnAccion->procesoActivo = PID_Activo;

				free(procesoActivo);
			} else {
				operacion = ERROR;
				log_info(ptrLog, "Operacion no reconocida.");
				//nucleo reciba error

				//enviarMensajeACpu("Error, operacion no reconocida", operacion, socketCpu);
				break;
			}

			free(mensajeRecibido);
		}
	}
}

void enviarACPUQueDebeFinalizarProceso(t_cpu * cpu,
		t_buffer_tamanio * bufferTamanio) {
	enviarDatos(cpu->socket, bufferTamanio->buffer,
			bufferTamanio->tamanioBuffer, NOTHING, UMC);
}

void escribirDatoDeCPU(t_cpu * cpu, uint32_t pagina, uint32_t offset,
		uint32_t tamanio, char * buffer) {
	t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(
			cpu->procesoActivo);
	if (tablaDeProceso != NULL) {
		t_registro_tabla_de_paginas * registro = buscarPaginaEnTabla(
				tablaDeProceso, pagina);
		if (registro != NULL) {
			int entradaTLB = pagEstaEnTLB(cpu->procesoActivo,
					registro->paginaProceso);
			if (entradasTLB > 0 && entradaTLB != -1) {
				log_info(ptrLog,
						"La Pagina %d del Proceso %d esta en la TLB (TLB HIT)",
						registro->paginaProceso, cpu->procesoActivo);

				t_tlb * registroTLB = obtenerYActualizarRegistroTLB(entradaTLB);
				t_frame * frame = list_get(frames, registroTLB->numFrame);

				int bufferAsInt = atoi(buffer);
				memcpy(frame->contenido + offset, &bufferAsInt, tamanio);

				registro->modificado = 1;
				registro->esStack = 1;
			} else {
				usleep(retardo * 1000);
				if (registro->estaEnUMC == 1) {
					if (entradaTLB == -1) {
						log_info(ptrLog,
								"La Pag %d del PID %d no esta en la TLB, busco en la TP (TLB MISS)",
								registro->paginaProceso, cpu->procesoActivo);
					}

					log_info(ptrLog, "La pagina %d del Proceso %d esta en UMC",
							registro->paginaProceso, cpu->procesoActivo);
					t_frame * frame = list_get(frames, registro->frame);

					int bufferAsInt = atoi(buffer);
					memcpy(frame->contenido + offset, &bufferAsInt, tamanio);

					agregarATLB(cpu->procesoActivo, registro->paginaProceso,
							frame->numeroFrame, frame->contenido);

					registro->estaEnUMC = 1;
					registro->frame = frame->numeroFrame;
					registro->modificado = 1;
					registro->bitDeReferencia = 1;
					registro->esStack = 1;
				} else {

					if (entradaTLB == -1) {
						log_info(ptrLog,
								"La Pag %d del PID %d no esta en la TLB, busco en la TP (TLB MISS)",
								registro->paginaProceso, cpu->procesoActivo);
					}
					log_info(ptrLog,
							"La pag %d del PID %d no esta en UMC, se la pido a Swap (PAGE FAULT)",
							registro->paginaProceso, cpu->procesoActivo);
					t_frame * frameSolicitado = solicitarPaginaASwap(cpu,
							pagina);

					if (frameSolicitado != NULL) {
						int bufferAsInt = atoi(buffer);
						memcpy(frameSolicitado->contenido + offset,
								&bufferAsInt, tamanio);

						agregarATLB(cpu->procesoActivo, registro->paginaProceso,
								frameSolicitado->numeroFrame,
								frameSolicitado->contenido);

						registro->estaEnUMC = 1;
						registro->frame = frameSolicitado->numeroFrame;
						registro->modificado = 1;
						registro->bitDeReferencia = 1;
						registro->esStack = 1;
					} else {
						uint32_t successInt = ERROR;
						t_buffer_tamanio * buffer_tamanio = serializarUint32(
								successInt);
						enviarACPUQueDebeFinalizarProceso(cpu, buffer_tamanio);
						return;
					}
				}
			}

			uint32_t successInt = SUCCESS;
			t_buffer_tamanio * buffer_tamanio = serializarUint32(successInt);

			int enviarBytes = enviarDatos(cpu->socket, buffer_tamanio->buffer,
					buffer_tamanio->tamanioBuffer, NOTHING, UMC);

			free(buffer_tamanio->buffer);
			free(buffer_tamanio);
		} else {
			log_error(ptrLog, "El Proceso %d no tiene pagina nro %d",
					cpu->procesoActivo, pagina);
			uint32_t successInt = ERROR;
			t_buffer_tamanio * buffer_tamanio = serializarUint32(successInt);
			enviarACPUQueDebeFinalizarProceso(cpu, buffer_tamanio);
			return;
		}
	} else {
		log_error(ptrLog, "El Proceso %d no tiene Tabla de Paginas",
				cpu->procesoActivo);
		uint32_t successInt = ERROR;
		t_buffer_tamanio * buffer_tamanio = serializarUint32(successInt);
		enviarACPUQueDebeFinalizarProceso(cpu, buffer_tamanio);
		return;
	}
}

int estaElProcesoEnUMC(uint32_t pid) {
	return buscarTablaDelProceso(pid) != NULL;
}

void limpiarInstruccion(char * instruccion) {
	char *p2 = instruccion;
	while (*instruccion != '\0') {
		if (*instruccion != '~') {
			*p2++ = *instruccion++;
		} else {
			++instruccion;
		}
	}
	*p2 = '\0';
}

void enviarDatoACPU(t_cpu * cpu, uint32_t pagina, uint32_t start,
		uint32_t offset, uint32_t operacion) {
	uint32_t offsetMemcpy = 0;
	t_list * datosParaCPU = list_create();
	t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(
			cpu->procesoActivo);
	if (tablaDeProceso != NULL) {
		t_list * listaRegistros = registrosABuscarParaPeticion(tablaDeProceso,
				pagina, start, offset);
		if (listaRegistros != NULL) {
			int i;
			for (i = 0; i < list_size(listaRegistros); i++) {
				t_auxiliar_registro * auxiliar = list_get(listaRegistros, i);
				t_registro_tabla_de_paginas * registro = auxiliar->registro;
				int entradaTLB = pagEstaEnTLB(cpu->procesoActivo,
						registro->paginaProceso);
				if (entradasTLB > 0 && entradaTLB != -1) {
					log_info(ptrLog,
							"La Pagina %d del Proceso %d esta en la TLB (TLB HIT)",
							registro->paginaProceso, cpu->procesoActivo);
					t_tlb * registroTLB = obtenerYActualizarRegistroTLB(
							entradaTLB);
					t_frame * frame = list_get(frames, registroTLB->numFrame);
					char * bufferAux = calloc(1, auxiliar->offset);
					if (operacion == LEER) {
						memcpy(bufferAux,
								(frame->contenido) + (auxiliar->start),
								auxiliar->offset);
					} else {
						int bufferInt;
						memcpy(&bufferInt,
								(frame->contenido) + (auxiliar->start),
								auxiliar->offset);
						sprintf(bufferAux, "%d", bufferInt);
					}
					list_add(datosParaCPU, bufferAux);
					offsetMemcpy += auxiliar->offset;
					registro->bitDeReferencia = 1;
				} else {
					usleep(retardo * 1000);
					if (registro->estaEnUMC == 1) {

						if (entradaTLB == -1) {
							log_info(ptrLog,
									"La Pag %d del PID %d no esta en la TLB, busco en la TP (TLB MISS)",
									registro->paginaProceso,
									cpu->procesoActivo);
						}

						log_info(ptrLog,
								"La pagina %d del Proceso %d esta en UMC",
								registro->paginaProceso, cpu->procesoActivo);
						t_frame * frame = list_get(frames, registro->frame);
						char * bufferAux = calloc(1, auxiliar->offset);
						if (operacion == LEER) {
							memcpy(bufferAux,
									(frame->contenido) + (auxiliar->start),
									auxiliar->offset);
						} else {
							int bufferInt;
							memcpy(&bufferInt,
									(frame->contenido) + (auxiliar->start),
									auxiliar->offset);
							sprintf(bufferAux, "%d", bufferInt);
						}
						list_add(datosParaCPU, bufferAux);
						offsetMemcpy += auxiliar->offset;
						agregarATLB(cpu->procesoActivo, registro->paginaProceso,
								frame->numeroFrame, frame->contenido);
						registro->estaEnUMC = 1;
						registro->frame = frame->numeroFrame;
						registro->bitDeReferencia = 1;
					} else {
						if (entradaTLB == -1) {
							log_info(ptrLog,
									"La Pag %d del PID %d no esta en la TLB, busco en la TP (TLB MISS)",
									registro->paginaProceso,
									cpu->procesoActivo);
						}

						log_info(ptrLog,
								"La pagina %d del Proceso %d no esta en UMC, se la pido a Swap (PAGE FAULT)",
								registro->paginaProceso, cpu->procesoActivo);
						t_frame * frameSolicitado = solicitarPaginaASwap(cpu,
								registro->paginaProceso);

						if (frameSolicitado != NULL) {
							char * bufferAux = calloc(1, auxiliar->offset);
							if (operacion == LEER) {
								memcpy(bufferAux,
										(frameSolicitado->contenido)
												+ (auxiliar->start),
										auxiliar->offset);
							} else {
								int bufferInt;
								memcpy(&bufferInt,
										(frameSolicitado->contenido)
												+ (auxiliar->start),
										auxiliar->offset);
								sprintf(bufferAux, "%d", bufferInt);
							}
							list_add(datosParaCPU, bufferAux);
							offsetMemcpy += auxiliar->offset;
							agregarATLB(cpu->procesoActivo,
									registro->paginaProceso,
									frameSolicitado->numeroFrame,
									frameSolicitado->contenido);
							registro->estaEnUMC = 1;
							registro->frame = frameSolicitado->numeroFrame;
							registro->bitDeReferencia = 1;
						} else {
							char * instrFinalizar = "FINALIZAR";
							t_instruccion * instruccion = malloc(
									strlen(instrFinalizar) + 1);
							instruccion->instruccion = instrFinalizar;
							t_buffer_tamanio * buffer_tamanio =
									serializarInstruccion(instruccion,
											strlen(instrFinalizar) + 1);
							enviarACPUQueDebeFinalizarProceso(cpu,
									buffer_tamanio);
							return;
						}
					}
				}
			}
			for (i = 0; i < list_size(listaRegistros); i++) {
				t_auxiliar_registro * auxiliar = list_get(listaRegistros, i);
				free(auxiliar);
			}
			free(listaRegistros);
		} else {
			char * instrFinalizar = "FINALIZAR";
			t_instruccion * instruccion = malloc(strlen(instrFinalizar) + 1);
			instruccion->instruccion = instrFinalizar;
			t_buffer_tamanio * buffer_tamanio = serializarInstruccion(
					instruccion, strlen(instrFinalizar) + 1);
			enviarACPUQueDebeFinalizarProceso(cpu, buffer_tamanio);
			return;
		}
	} else {
		char * instrFinalizar = "FINALIZAR";
		t_instruccion * instruccion = malloc(strlen(instrFinalizar) + 1);
		instruccion->instruccion = instrFinalizar;
		t_buffer_tamanio * buffer_tamanio = serializarInstruccion(instruccion,
				strlen(instrFinalizar) + 1);
		enviarACPUQueDebeFinalizarProceso(cpu, buffer_tamanio);
		return;
	}
	char *instruccionPosta = calloc(list_size(datosParaCPU), 50);
	for (i = 0; i < list_size(datosParaCPU); i++) {
		char * aux = list_get(datosParaCPU, i);
		strcat(instruccionPosta, aux);
		free(aux);
	}
	log_info(ptrLog, "CPU %d pidio la instruccion: %s", cpu->numCpu,
			instruccionPosta);
	t_instruccion * instruccion = malloc(strlen(instruccionPosta) + 1);
	instruccion->instruccion = instruccionPosta;
	t_buffer_tamanio * buffer_tamanio;
	buffer_tamanio = serializarInstruccion(instruccion,
			strlen(instruccionPosta) + 1);
	enviarDatos(cpu->socket, buffer_tamanio->buffer,
			buffer_tamanio->tamanioBuffer, NOTHING, UMC);
	free(instruccion);
}

t_frame * solicitarPaginaASwap(t_cpu * cpu, uint32_t pagina) {
	t_solicitud_pagina * solicitudPagina = malloc(sizeof(t_solicitud_pagina));
	solicitudPagina->pid = cpu->procesoActivo;
	solicitudPagina->paginaProceso = pagina;
	if(estaElProcesoEnUMC(cpu->procesoActivo)) {
		t_buffer_tamanio * buffer_tamanio = serializarSolicitudPagina(solicitudPagina);
		char * mensajeDeSwap = enviarYRecibirMensajeSwap(buffer_tamanio, LEER);
		free(solicitudPagina);
		free(buffer_tamanio->buffer);
		free(buffer_tamanio);
		if (strcmp(mensajeDeSwap, "ERROR") == 0) {
			log_error(ptrLog, "Ocurrio un error al Solicitar una Pagina a Swap");
			printf("Ocurrio un error al Solicitar una Pagina a Swap\n");
			return NULL;
		} else {
			t_pagina_de_swap * paginaSwap = deserializarPaginaDeSwap(mensajeDeSwap);
			free(mensajeDeSwap);
			log_info(ptrLog, "Swap envia la Pagina %d del Proceso %d.", pagina,
					cpu->procesoActivo);
			t_frame * frame = agregarPaginaAUMC(paginaSwap, cpu->procesoActivo,
					pagina);

			if (frame != NULL) {
				log_info(ptrLog, "Se agrego la pagina %i a umc", pagina);
				free(paginaSwap);
				return frame;
			} else {
				log_info(ptrLog,
						"No hay lugar para guardar una pagina del Proceso %d. Se finaliza el Proceso.",
						cpu->procesoActivo);
				printf(
						"No hay lugar para guardar una pagina del Proceso %d. Se finaliza el Proceso.\n",
						cpu->procesoActivo);
				return NULL;
			}
		}
	}
	return NULL;
}

t_list * registrosABuscarParaPeticion(t_tabla_de_paginas * tablaDeProceso,
		uint32_t pagina, uint32_t start, uint32_t offset) {
	t_list * registros = list_create();

	//Armo el primer request
	t_registro_tabla_de_paginas * registro = buscarPaginaEnTabla(tablaDeProceso,
			pagina);

	if (registro != NULL) {
		t_auxiliar_registro * auxiliar = malloc(
				sizeof(t_registro_tabla_de_paginas) + (sizeof(uint32_t) * 2));
		auxiliar->registro = registro;

		if ((start + offset) <= marcosSize) {
			//Es solo 1 pagina la que hay que agarrar
			auxiliar->start = start;
			auxiliar->offset = offset;
			start = -1;
			offset = -1;
			list_add(registros, auxiliar);
		} else {
			auxiliar->start = start;
			auxiliar->offset = (marcosSize - start);
			offset = offset - (marcosSize - start);
			start = 0;
			list_add(registros, auxiliar);

			while ((start + offset) > marcosSize) {
				pagina++;
				t_registro_tabla_de_paginas * registroAdicional =
						buscarPaginaEnTabla(tablaDeProceso, pagina);
				if (registroAdicional != NULL) {
					t_auxiliar_registro * auxiliarAdicional = malloc(
							sizeof(t_registro_tabla_de_paginas)
									+ (sizeof(uint32_t) * 2));
					auxiliarAdicional->registro = registroAdicional;
					auxiliarAdicional->start = start;
					auxiliarAdicional->offset = marcosSize;
					offset = offset - marcosSize;
					list_add(registros, auxiliarAdicional);
				} else {
					return NULL;
				}
			}

			if (offset >= 0 && (start + offset) < marcosSize) {
				pagina++;
				t_registro_tabla_de_paginas * registroAdicional2 =
						buscarPaginaEnTabla(tablaDeProceso, pagina);
				if (registroAdicional2 != NULL) {
					t_auxiliar_registro * auxiliarAdicional2 = malloc(
							sizeof(t_registro_tabla_de_paginas)
									+ (sizeof(uint32_t) * 2));
					auxiliarAdicional2->registro = registroAdicional2;
					auxiliarAdicional2->start = 0;
					//if(offset == 0) {
					auxiliarAdicional2->offset = offset;
					/*}else{
					 auxiliarAdicional2->offset = offset;
					 }*/
					list_add(registros, auxiliarAdicional2);
				} else {
					return NULL;
				}
			}
		}

		return registros;
	} else {
		return NULL;
	}

}

t_registro_tabla_de_paginas * buscarPaginaEnTabla(t_tabla_de_paginas * tabla,
		uint32_t pagina) {
	int i;
	for (i = 0; i < list_size(tabla->tablaDePaginas); i++) {
		if (tabla != NULL) {
			t_registro_tabla_de_paginas * registro = list_get(
					tabla->tablaDePaginas, i);
			if (registro != NULL) {
				if (registro->paginaProceso == pagina) {
					return registro;
				}
			} else {
				return NULL;
			}
		} else {
			return NULL;
		}
	}
	return NULL;
}

t_tabla_de_paginas * buscarTablaDelProceso(uint32_t procesoId) {
	int i;
	for (i = 0; i < list_size(tablaProcesosPaginas); i++) {
		t_tabla_de_paginas * tabla = list_get(tablaProcesosPaginas, i);
		if (tabla->pID == procesoId) {
			return tabla;
		}
	}
	return NULL;
}

int indiceDeTablaDelProceso(uint32_t procesoId) {
	int i;
	for (i = 0; i < list_size(tablaProcesosPaginas); i++) {
		t_tabla_de_paginas * tabla = list_get(tablaProcesosPaginas, i);
		if (tabla->pID == procesoId) {
			return i;
		}
	}
	return i;
}

void enviarTamanioPaginaACPU(int socketCPU) {
	t_buffer_tamanio * buffer_tamanio = serializarUint32(marcosSize);
	int bytesEnviados = enviarDatos(socketCPU, buffer_tamanio->buffer,
			buffer_tamanio->tamanioBuffer, ENVIAR_TAMANIO_PAGINA_A_CPU, UMC);
	free(buffer_tamanio->buffer);
	free(buffer_tamanio);
	if (bytesEnviados <= 0) {
		log_info(ptrLog, "No se pudo enviar Tamanio de Pagina a CPU");
		printf("No se pudo enviar Tamanio de Pagina a CPU\n");
	}
}

void enviarTamanioPaginaANUCLEO() {
	t_buffer_tamanio * buffer_tamanio = serializarUint32(marcosSize);
	int bytesEnviados = enviarDatos(socketClienteNucleo, buffer_tamanio->buffer,
			buffer_tamanio->tamanioBuffer, TAMANIO_PAGINAS_NUCLEO, UMC);
	free(buffer_tamanio->buffer);
	free(buffer_tamanio);
	if (bytesEnviados <= 0) {
		log_info(ptrLog, "No se pudo enviar Tamanio de Pagina a Nucleo");
		printf("No se pudo enviar Tamanio de Pagina a Nucleo\n");
	}
}

void finalizarPrograma(uint32_t PID) {
	t_finalizar_programa * finalizarProg = malloc(sizeof(t_finalizar_programa));
	finalizarProg->programID = PID;

	t_buffer_tamanio * buffer_tamanio = serializarFinalizarPrograma(
			finalizarProg);

	char * respuestaSwap = enviarYRecibirMensajeSwap(buffer_tamanio,
			FINALIZARPROGRAMA);
	free(buffer_tamanio->buffer);
	free(buffer_tamanio);

	if (strcmp(respuestaSwap, "ERROR") == 0) {
		log_error(ptrLog, "No se recibio nada de Swap");
		printf("No se recibio nada de Swap\n");
		return;
	} else {
		uint32_t respuesta = deserializarUint32(respuestaSwap);

		if (respuesta == SUCCESS) {
			borrarEstructurasDeProceso(PID);
			log_info(ptrLog,
					"Se borraron las estructuras relacionadas al Proceso %d",
					PID);
			printf("Se borraron las estructuras relacionadas al Proceso %d\n",
					PID);
		} else {
			log_info(ptrLog,
					"Ocurrio un error al borrar las estructuras relacionadas al Proceso %d",
					PID);
		}

		free(finalizarProg);
		free(respuestaSwap);
	}
}

void borrarEstructurasDeProceso(uint32_t pid) {
	tlbFlushDeUnPID(pid);
	t_tabla_de_paginas * tablaAEliminar = buscarTablaDelProceso(pid);
	if (tablaAEliminar != NULL) {
		int i;
		for (i = 0; i < list_size(tablaAEliminar->tablaDePaginas); i++) {
			t_registro_tabla_de_paginas * registro = list_get(
					tablaAEliminar->tablaDePaginas, i);
			if (registro != NULL && registro->estaEnUMC == 1) {
				pthread_mutex_lock(&accesoAFrames);
				t_frame * frame = list_get(frames, registro->frame);
				frame->disponible = 1;
				frame->numeroFrame = i;
				pthread_mutex_unlock(&accesoAFrames);
			}
			free(registro);
		}

		int indexDeTablaAEliminar = indiceDeTablaDelProceso(pid);
		list_remove(tablaProcesosPaginas, indexDeTablaAEliminar);
		free(tablaAEliminar);
	}
}

uint32_t checkDisponibilidadPaginas(t_iniciar_programa * iniciarProg) {
	t_check_espacio * check = malloc(sizeof(t_check_espacio));
	check->pid = iniciarProg->programID;
	check->cantidadDePaginas = iniciarProg->tamanio;

	t_buffer_tamanio * iniciarProgSerializado = serializarCheckEspacio(check);
	free(check);

	log_info(ptrLog, "Envio a Swap Cantidad de Paginas requeridas y PID: %d",
			iniciarProg->programID);
	printf("Envio a Swap Cantidad de Paginas requeridas y PID: %d\n",
			iniciarProg->programID);
	//enviarMensajeASwap(iniciarProgSerializado->buffer, iniciarProgSerializado->tamanioBuffer, NUEVOPROGRAMA);
	//;
	uint32_t operacion;
	uint32_t id;
	log_info(ptrLog,
			"Espero que Swap me diga si puede o no alojar el Proceso con PID: %d.",
			iniciarProg->programID);
	printf(
			"Espero que Swap me diga si puede o no alojar el Proceso con PID: %d.\n",
			iniciarProg->programID);

	pthread_mutex_lock(&comunicacionConSwap);
	enviarDatos(socketSwap, iniciarProgSerializado->buffer,
			iniciarProgSerializado->tamanioBuffer, NUEVOPROGRAMA, UMC);
	free(iniciarProgSerializado->buffer);
	free(iniciarProgSerializado);
	char* hayEspacio = recibirDatos(socketSwap, &operacion, &id);
	usleep(30);
	pthread_mutex_unlock(&comunicacionConSwap);

	uint32_t pudoSwap = deserializarUint32(hayEspacio);
	free(hayEspacio);
	return pudoSwap;
}

void liberarMemoria(char * memoriaALiberar) {
	free(memoriaALiberar);
	log_info(ptrLog, "Memoria Liberada");
}

t_cambio_proc_activo* deserializarCambioProcesoActivo(char * mensaje) {
	t_cambio_proc_activo *respuesta = malloc(sizeof(t_cambio_proc_activo));
	int offset = 0, tmp_size = sizeof(uint32_t);
	memcpy(&(respuesta->programID), mensaje + offset, tmp_size);
	return respuesta;
}

t_tlb * obtenerYActualizarRegistroTLB(int entradaTLB) {
	pthread_mutex_lock(&accesoATLB);
	t_tlb * tlbRegistro = list_get(TLB, entradaTLB);

	t_tlb * nuevoRegistroTLB = malloc((sizeof(int) * 4) + marcosSize);
	nuevoRegistroTLB->indice = tlbRegistro->indice;
	nuevoRegistroTLB->numFrame = tlbRegistro->numFrame;
	nuevoRegistroTLB->numPag = tlbRegistro->numPag;
	nuevoRegistroTLB->pid = tlbRegistro->pid;

	list_remove(TLB, entradaTLB);
	list_add(TLB, nuevoRegistroTLB);
	pthread_mutex_unlock(&accesoATLB);

	free(tlbRegistro);

	return nuevoRegistroTLB;
}

int pagEstaEnTLB(int pid, int numPag) {
	int i, j = -1;
	pthread_mutex_lock(&accesoATLB);
	if (entradasTLB > 0 && TLB != NULL && list_size(TLB) > 0) {
		for (i = 0; i < list_size(TLB); i++) {
			t_tlb * registro = list_get(TLB, i);
			if (registro->pid == pid && registro->numPag == numPag) {
				j = i;
			}
		}
	}
	pthread_mutex_unlock(&accesoATLB);
	return j;
}

void iniciarTLB() {
	int i = 0;
	if (entradasTLB != 0) {
		TLB = list_create();

		for (i = 0; i <= (entradasTLB - 1); i++) {
			t_tlb * registro = malloc((sizeof(int) * 4));
			registro->pid = -1;
			registro->indice = i;
			registro->numPag = -1;
			registro->numFrame = -1;

			list_add(TLB, registro);
		}
	} else {
		TLB = NULL;
	}
}

int entradaTLBAReemplazarPorLRU() {
	//Retorna -1 si no hay libres. Sino, retorna indice de Libre.
	int i = 0;
	int indiceLibre = -1;
	pthread_mutex_lock(&accesoATLB);
	if (TLB != NULL) {
		for (i = 0; i <= list_size(TLB) - 1; i++) {
			t_tlb * registro = list_get(TLB, i);
			if (registro->pid == -1) {
				indiceLibre = registro->indice;
				break;
			}
		}
	}
	pthread_mutex_unlock(&accesoATLB);
	return indiceLibre;
}

void agregarATLB(int pid, int pagina, int frame, char * contenidoFrame) {
	if (entradasTLB > 0) {
		int indiceLibre = entradaTLBAReemplazarPorLRU();

		//t_tlb * aInsertar = malloc(sizeof(t_tlb));
		t_tlb* aInsertar = malloc(4 * sizeof(int));
		aInsertar->numPag = pagina;
		aInsertar->numFrame = frame;
		aInsertar->pid = pid;

		if (indiceLibre > -1) {
			aInsertar->indice = indiceLibre;
			pthread_mutex_lock(&accesoATLB);
			list_remove(TLB, indiceLibre);
			list_add_in_index(TLB, indiceLibre, aInsertar);
			pthread_mutex_unlock(&accesoATLB);
			log_info(ptrLog, "La Pagina %d del Proceso %d se agrego en la TLB",
					pagina, pid);
		} else {
			aInsertar->indice = list_size(TLB) - 1;
			//Actualizo el Frame en Swap antes de eliminar
			t_tlb * tlbAEliminar = list_get(TLB, 0);
			//Elimino la primera entrada => La mas vieja
			log_info(ptrLog,
					"Se elimina entrada de la TLB: Proceso %i - Pagina %i",
					tlbAEliminar->pid, tlbAEliminar->numPag);
			pthread_mutex_lock(&accesoATLB);
			list_remove(TLB, 0);
			//aca falta que todos los anteriores cambien su indice hecho
			int a = 0;
			for (a = 0; a < (list_size(TLB)); a++) {
				t_tlb * tlbCambiar = list_get(TLB, a);
				tlbCambiar->indice = tlbCambiar->indice - 1;
			}
			list_add(TLB, aInsertar);
			pthread_mutex_unlock(&accesoATLB);
			log_info(ptrLog, "La Pagina %d del Proceso %d se agrego en la TLB",
					pagina, pid);
		}
	}
}

void tlbFlush() {
	if (entradasTLB == 0) {
		log_info(ptrLog, "TLB deshabilitada, no se realiza flush");
		printf("TLB deshabilitada, no se realiza flush\n");
	} else {
		int i = 0;
		if ((entradasTLB) != 0) {
			pthread_mutex_lock(&accesoATLB);
			for (i = 0; i < list_size(TLB); i++) {
				t_tlb * registro = list_get(TLB, i);
				registro->pid = -1;
				registro->indice = i;
				registro->numPag = 0;
				registro->numFrame = -1;
			}
			pthread_mutex_unlock(&accesoATLB);
		}
		log_info(ptrLog, "Flush en TLB realizado.");
		printf("Flush en TLB realizado.\n");
	}
}

void tlbFlushDeUnPID(int PID) {
	int i = 0;
	if ((entradasTLB) != 0 && TLB != NULL) {
		pthread_mutex_lock(&accesoATLB);
		for (i = 0; i < list_size(TLB); i++) {
			t_tlb * registro = list_get(TLB, i);
			if (registro->pid == PID) {
				registro->pid = -1;
				registro->indice = i;
				registro->numPag = -1;
				registro->numFrame = -1;
			}
		}
		pthread_mutex_unlock(&accesoATLB);
		log_info(ptrLog, "Flush en TLB de Proceso con PID %d realizado", PID);
	}
}

void finalizarTLB() {
	if (TLB != NULL) {
		pthread_mutex_lock(&accesoATLB);
		for (i = 0; i < list_size(TLB); i++) {
			t_tlb * registro = malloc(sizeof(t_tlb));
			free(registro);
		}
		pthread_mutex_unlock(&accesoATLB);
		free(TLB);
	}
}

t_frame * agregarPaginaAUMC(t_pagina_de_swap * paginaSwap, uint32_t pid,
		uint32_t pagina) {
	if (!strcmp(algoritmoReemplazo, "CLOCK")) {
		return actualizarFramesConClock(paginaSwap, pid, pagina);
	}
	if (!strcmp(algoritmoReemplazo, "CLOCK-M")) {
		return actualizarFramesConClockModificado(paginaSwap, pid, pagina);
	} else {
		log_info(ptrLog, "Algoritmo de reemplazo no reconocido");
		printf("Algoritmo de reemplazo no reconocido\n");
		return NULL;
	}
}

int procesoPuedeGuardarFrameSinDesalojar(uint32_t pid) {
	t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(pid);
	if (tablaDeProceso != NULL) {
		if (list_size(tablaDeProceso->paginasEnUMC) < marcoXProc) {
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

int procesoTieneAlgunaPaginaEnUMC(uint32_t pid) {
	t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(pid);
	if (tablaDeProceso != NULL) {
		return list_size(tablaDeProceso->paginasEnUMC) > 0;
	} else {
		return 0;
	}
}

t_frame * actualizarFramesConClock(t_pagina_de_swap * paginaSwap, uint32_t pid,
		uint32_t pagina) {
	if (procesoPuedeGuardarFrameSinDesalojar(pid)) {
		int indiceFrameLibre = buscarFrameLibre();
		if (indiceFrameLibre > -1) {
			t_frame * frame = malloc((sizeof(uint32_t) * 3) + marcosSize);
			frame->contenido = paginaSwap->paginaSolicitada;
			frame->disponible = 0;
			frame->numeroFrame = indiceFrameLibre;
			pthread_mutex_lock(&accesoAFrames);
			list_remove(frames, indiceFrameLibre);
			list_add_in_index(frames, indiceFrameLibre, frame);
			//Actualizo Tabla de Paginas del Proceso - Actualizar puntero
			t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(pid);
			list_add(tablaDeProceso->paginasEnUMC, pagina);
			tablaDeProceso->posibleProximaVictima++;
			t_registro_tabla_de_paginas * registroTabla = buscarPaginaEnTabla(
					tablaDeProceso, pagina);
			registroTabla->estaEnUMC = 1;
			registroTabla->frame = frame->numeroFrame;
			registroTabla->bitDeReferencia = 1;
			pthread_mutex_unlock(&accesoAFrames);
			return frame;
		} else {
			if (procesoTieneAlgunaPaginaEnUMC(pid)) {
				t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(
						pid);
				if (tablaDeProceso != NULL) {
					return desalojarFrameConClock(paginaSwap, pid, pagina,
							tablaDeProceso);
				} else {
					return NULL;
				}
			} else {
				return NULL;
			}
		}
	} else {
		t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(pid);
		if (tablaDeProceso != NULL) {
			return desalojarFrameConClock(paginaSwap, pid, pagina,
					tablaDeProceso);
		} else {
			return NULL;
		}
	}
}

t_registro_tabla_de_paginas * obtenerPaginaDeProceso(uint32_t pag,
		t_tabla_de_paginas * tabla) {
	int i;
	for (i = 0; i < list_size(tabla->tablaDePaginas); i++) {
		t_registro_tabla_de_paginas * registro = list_get(tabla->tablaDePaginas,
				i);
		if (registro->paginaProceso == pag) {
			return registro;
		}
	}
	return NULL;
}

t_frame * desalojarFrameConClock(t_pagina_de_swap * paginaSwap, uint32_t pid,
		uint32_t pagina, t_tabla_de_paginas * tablaDeProceso) {
	if (tablaDeProceso->posibleProximaVictima >= marcoXProc) {
		tablaDeProceso->posibleProximaVictima = 0;
	}
	pthread_mutex_lock(&accesoAFrames);
	uint32_t pag = list_get(tablaDeProceso->paginasEnUMC,
			tablaDeProceso->posibleProximaVictima);
	t_registro_tabla_de_paginas * registroCandidato = obtenerPaginaDeProceso(
			pag, tablaDeProceso);
	pthread_mutex_unlock(&accesoAFrames);
	if (registroCandidato->estaEnUMC == 1) {
		if (registroCandidato->bitDeReferencia == 1) {
			registroCandidato->bitDeReferencia = 0;
			tablaDeProceso->posibleProximaVictima++;
			return desalojarFrameConClock(paginaSwap, pid, pagina,
					tablaDeProceso);
		} else {
			t_frame * frame = list_get(frames, registroCandidato->frame);
			frame->disponible = 0;
			int posEnTlb = pagEstaEnTLB(pid, registroCandidato->paginaProceso);
			if (posEnTlb > -1 && posEnTlb < list_size(TLB)) {
				//la tengo que eliminar
				pthread_mutex_lock(&accesoATLB);
				t_tlb * aEliminar = list_get(TLB, posEnTlb);
				aEliminar->pid = -1;
				aEliminar->numPag = 0;
				aEliminar->numFrame = -1;
				pthread_mutex_unlock(&accesoATLB);
			}
			chequearSiHayQueEscribirEnSwapLaPagina(pid, registroCandidato);
			pthread_mutex_lock(&accesoAFrames);
			frame->contenido = paginaSwap->paginaSolicitada;
			list_remove(tablaDeProceso->paginasEnUMC,
					tablaDeProceso->posibleProximaVictima);
			list_add_in_index(tablaDeProceso->paginasEnUMC,
					tablaDeProceso->posibleProximaVictima, pagina);
			tablaDeProceso->posibleProximaVictima++;
			t_registro_tabla_de_paginas * registroTabla = buscarPaginaEnTabla(
					tablaDeProceso, pagina);
			registroTabla->estaEnUMC = 1;
			registroTabla->frame = frame->numeroFrame;
			registroTabla->bitDeReferencia = 1;
			pthread_mutex_unlock(&accesoAFrames);
			return frame;
		}
	} else {
		tablaDeProceso->posibleProximaVictima++;
		return desalojarFrameConClock(paginaSwap, pid, pagina, tablaDeProceso);
	}
}

t_frame * actualizarFramesConClockModificado(t_pagina_de_swap * paginaSwap,
		uint32_t pid, uint32_t pagina) {
	if (procesoPuedeGuardarFrameSinDesalojar(pid)) {
		int indiceFrameLibre = buscarFrameLibre();
		if (indiceFrameLibre > -1) {
			t_frame * frame = malloc((sizeof(uint32_t) * 3) + marcosSize);
			frame->contenido = paginaSwap->paginaSolicitada;
			frame->disponible = 0;
			frame->numeroFrame = indiceFrameLibre;
			pthread_mutex_lock(&accesoAFrames);
			list_remove(frames, indiceFrameLibre);
			list_add_in_index(frames, indiceFrameLibre, frame);
			//Actualizo Tabla de Paginas del Proceso - Actualizar puntero
			t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(pid);
			list_add(tablaDeProceso->paginasEnUMC, pagina);
			tablaDeProceso->posibleProximaVictima++;
			t_registro_tabla_de_paginas * registroTabla = buscarPaginaEnTabla(
					tablaDeProceso, pagina);
			registroTabla->estaEnUMC = 1;
			registroTabla->frame = frame->numeroFrame;
			registroTabla->bitDeReferencia = 1;
			pthread_mutex_unlock(&accesoAFrames);
			return frame;
		} else {
			if (procesoTieneAlgunaPaginaEnUMC(pid)) {
				t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(
						pid);
				if (tablaDeProceso != NULL) {
					return desalojarFrameConClockModificado(paginaSwap, pid,
							pagina, tablaDeProceso);
				} else {
					return NULL;
				}
			} else {
				return NULL;
			}
		}
	} else {
		t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(pid);
		if (tablaDeProceso != NULL) {
			return desalojarFrameConClockModificado(paginaSwap, pid, pagina,
					tablaDeProceso);
		} else {
			return NULL;
		}
	}
}

t_frame * desalojarFrameConClockModificado(t_pagina_de_swap * paginaSwap,
		uint32_t pid, uint32_t pagina, t_tabla_de_paginas * tablaDeProceso) {
	if (tablaDeProceso->posibleProximaVictima >= marcoXProc) {
		tablaDeProceso->posibleProximaVictima = 0;
	}

	int i;
	for (i = 0; i < list_size(tablaDeProceso->tablaDePaginas); i++) {
		pthread_mutex_lock(&accesoAFrames);
		uint32_t pag = list_get(tablaDeProceso->paginasEnUMC,
				tablaDeProceso->posibleProximaVictima);
		t_registro_tabla_de_paginas * registroCandidato =
				obtenerPaginaDeProceso(pag, tablaDeProceso);
		pthread_mutex_unlock(&accesoAFrames);

		if (registroCandidato->estaEnUMC == 1
				&& registroCandidato->modificado == 0
				&& registroCandidato->bitDeReferencia == 0) {
			t_frame * frame = list_get(frames, registroCandidato->frame);
			frame->disponible = 0;
			int posEnTlb = pagEstaEnTLB(pid, registroCandidato->paginaProceso);
			if (posEnTlb > -1 && posEnTlb < list_size(TLB)) {
				//la tengo que eliminar
				pthread_mutex_lock(&accesoATLB);
				t_tlb * aEliminar = list_get(TLB, posEnTlb);
				aEliminar->pid = -1;
				aEliminar->numPag = 0;
				aEliminar->numFrame = -1;
				pthread_mutex_unlock(&accesoATLB);
			}
			chequearSiHayQueEscribirEnSwapLaPagina(pid, registroCandidato);
			pthread_mutex_lock(&accesoAFrames);
			frame->contenido = paginaSwap->paginaSolicitada;
			list_remove(tablaDeProceso->paginasEnUMC,
					tablaDeProceso->posibleProximaVictima);
			list_add_in_index(tablaDeProceso->paginasEnUMC,
					tablaDeProceso->posibleProximaVictima, pagina);
			tablaDeProceso->posibleProximaVictima++;
			t_registro_tabla_de_paginas * registroTabla = buscarPaginaEnTabla(
					tablaDeProceso, pagina);
			registroTabla->estaEnUMC = 1;
			registroTabla->frame = frame->numeroFrame;
			registroTabla->bitDeReferencia = 1;
			pthread_mutex_unlock(&accesoAFrames);
			return frame;
		} else {
			tablaDeProceso->posibleProximaVictima++;
			if (tablaDeProceso->posibleProximaVictima >= marcoXProc) {
				tablaDeProceso->posibleProximaVictima = 0;
			}
		}
	}

	for (i = 0; i < list_size(tablaDeProceso->tablaDePaginas); i++) {
		pthread_mutex_lock(&accesoAFrames);
		uint32_t pag = list_get(tablaDeProceso->paginasEnUMC,
				tablaDeProceso->posibleProximaVictima);
		t_registro_tabla_de_paginas * registroCandidato =
				obtenerPaginaDeProceso(pag, tablaDeProceso);
		pthread_mutex_unlock(&accesoAFrames);

		if (registroCandidato->estaEnUMC == 1
				&& registroCandidato->modificado == 1
				&& registroCandidato->bitDeReferencia == 0) {
			t_frame * frame = list_get(frames, registroCandidato->frame);
			frame->disponible = 0;
			int posEnTlb = pagEstaEnTLB(pid, registroCandidato->paginaProceso);
			if (posEnTlb > -1 && posEnTlb < list_size(TLB)) {
				//la tengo que eliminar
				pthread_mutex_lock(&accesoATLB);
				t_tlb * aEliminar = list_get(TLB, posEnTlb);
				aEliminar->pid = -1;
				aEliminar->numPag = 0;
				aEliminar->numFrame = -1;
				pthread_mutex_unlock(&accesoATLB);
			}
			chequearSiHayQueEscribirEnSwapLaPagina(pid, registroCandidato);
			pthread_mutex_lock(&accesoAFrames);
			frame->contenido = paginaSwap->paginaSolicitada;
			list_remove(tablaDeProceso->paginasEnUMC,
					tablaDeProceso->posibleProximaVictima);
			list_add_in_index(tablaDeProceso->paginasEnUMC,
					tablaDeProceso->posibleProximaVictima, pagina);
			tablaDeProceso->posibleProximaVictima++;
			t_registro_tabla_de_paginas * registroTabla = buscarPaginaEnTabla(
					tablaDeProceso, pagina);
			registroTabla->estaEnUMC = 1;
			registroTabla->frame = frame->numeroFrame;
			registroTabla->bitDeReferencia = 1;
			pthread_mutex_unlock(&accesoAFrames);
			return frame;
		} else {
			registroCandidato->bitDeReferencia = 0;
			tablaDeProceso->posibleProximaVictima++;
			if (tablaDeProceso->posibleProximaVictima >= marcoXProc) {
				tablaDeProceso->posibleProximaVictima = 0;
			}
		}
	}

	return desalojarFrameConClockModificado(paginaSwap, pid, pagina,
			tablaDeProceso);
}

void chequearSiHayQueEscribirEnSwapLaPagina(uint32_t pid,
		t_registro_tabla_de_paginas * registro) {
	if (registro->modificado == 1) {
		escribirFrameEnSwap(registro->frame, pid, registro->paginaProceso);
	}
	registro->estaEnUMC = 0;
	registro->frame = -1;
	registro->modificado = 0;
	registro->bitDeReferencia = 0;
}

void escribirFrameEnSwap(int nroFrame, uint32_t pid, uint32_t pagina) {
	pthread_mutex_lock(&accesoAFrames);
	t_frame * frame = list_get(frames, nroFrame);

	t_escribir_en_swap * escribirEnSwap = malloc(
			(sizeof(uint32_t) * 2) + marcosSize);
	escribirEnSwap->paginaProceso = pagina;
	escribirEnSwap->pid = pid;
	escribirEnSwap->contenido = malloc(marcosSize);
	memcpy(escribirEnSwap->contenido, frame->contenido, marcosSize);
	pthread_mutex_unlock(&accesoAFrames);

	t_buffer_tamanio * buffer_tamanio = serializarEscribirEnSwap(escribirEnSwap,
			marcosSize);
	enviarYRecibirMensajeSwap(buffer_tamanio, ESCRIBIR);

	free(buffer_tamanio->buffer);
	free(buffer_tamanio);
	free(escribirEnSwap);
}

int buscarFrameLibre() {
	pthread_mutex_lock(&accesoAFrames);
	int i, j = -1;
	for (i = 0; i <= (list_size(frames) - 1); i++) {
		t_frame * frame = list_get(frames, i);
		if (frame->disponible == 1) {
			j = i;
			break;
		}
	}
	pthread_mutex_unlock(&accesoAFrames);
	return j;
}

void retardar(int retardoNuevo) {
	if (retardoNuevo >= 0) {
		retardo = retardoNuevo;
		printf("Se modifico el retardo a: %d\n", retardoNuevo);
	} else {
		printf("El nuevo valor de retardo debe ser un numero >= 0!\n");
	}
}

void flushMemory(uint32_t pid) {
	t_tabla_de_paginas * tablaAModificar = buscarTablaDelProceso(pid);
	if (tablaAModificar != NULL) {
		int i;
		pthread_mutex_lock(&accesoAFrames);
		for (i = 0; i < list_size(tablaAModificar->tablaDePaginas); i++) {
			t_registro_tabla_de_paginas * registro = list_get(
					tablaAModificar->tablaDePaginas, i);
			registro->modificado = 1;
		}
		pthread_mutex_unlock(&accesoAFrames);
		printf(
				"Se realizo flush a la Tabla de Paginas del Proceso con PID: %d\n",
				pid);
	} else {
		printf("No existe Tabla de Paginas del Proceso con PID: %d\n", pid);
	}
}

void dumpDeUnPID(uint32_t pid) {
	//traigo la tabla del proceso
	pthread_mutex_lock(&accesoAFrames);
	t_tabla_de_paginas * tablaAMostrar = buscarTablaDelProceso(pid);
	int i;
	if (tablaAMostrar != NULL) {
		//pthread_mutex_lock(&accesoAFrames);
		int noHayAlgo = 1;
		for (i = 0; i < list_size(tablaAMostrar->tablaDePaginas); i++) {
			t_registro_tabla_de_paginas * registro = list_get(
					tablaAMostrar->tablaDePaginas, i);
			if (registro->estaEnUMC == 1) {
				noHayAlgo = 0;
			}
		}
		if (noHayAlgo) {
			log_info(ptrLog, "No hay nada en memoria del PID: %d", pid);
			printf("No hay nada en memoria del PID: %d\n", pid);
		} else {
			for (i = 0; i < list_size(tablaAMostrar->tablaDePaginas); i++) {
				//Traigo cada tabla de paginas
				t_registro_tabla_de_paginas * registro = list_get(
						tablaAMostrar->tablaDePaginas, i);
				if (registro->estaEnUMC == 1) {
					t_frame * frame = list_get(frames, registro->frame);

					if (registro->esStack > 0) {
						int numerosPosiblesDentroDeStack = marcosSize / 4;
						int j, offset = 0;
						char * contenidoStack = malloc(
								numerosPosiblesDentroDeStack * 45);
						for (j = 0; j < numerosPosiblesDentroDeStack; j++) {
							int numAux;
							memcpy(&numAux, frame->contenido + offset,
									sizeof(int));
							offset += 4;
							char * auxChar = calloc(1, sizeof(int) + 1);
							sprintf(auxChar, "%d", numAux);
							strcat(contenidoStack, auxChar);
							strcat(contenidoStack, " - \0");
						}
						strcat(contenidoStack, "\0");
						log_info(ptrLog,
								"PID: %d | Pag: %d | Marco: %d | P: %d | M: %d | Contenido: \"%s\"\n",
								pid, registro->paginaProceso, registro->frame,
								registro->estaEnUMC, registro->modificado,
								contenidoStack);
						log_info(ptrLog,
								"__________________________________________________________________\n");
						printf(
								"PID: %d | Pag: %d | Marco: %d | P: %d | M: %d | Contenido: \"%s\"\n",
								pid, registro->paginaProceso, registro->frame,
								registro->estaEnUMC, registro->modificado,
								contenidoStack);
						printf(
								"__________________________________________________________________\n");
						escribirEnArchivo(pid, registro->paginaProceso,
								registro->frame, registro->estaEnUMC,
								registro->modificado, contenidoStack);

						free(contenidoStack);
					} else {
						char * contenido;
						contenido = malloc((marcosSize * sizeof(char)) + 1);

						memcpy(contenido, frame->contenido, marcosSize);
						contenido[marcosSize] = '\0';

						log_info(ptrLog,
								"PID: %d | Pag: %d | Marco: %d | P: %d | M: %d | Contenido: \"%s\"\n",
								pid, registro->paginaProceso, registro->frame,
								registro->estaEnUMC, registro->modificado,
								contenido);
						log_info(ptrLog,
								"__________________________________________________________________\n");
						printf(
								"PID: %d | Pag: %d | Marco: %d | P: %d | M: %d | Contenido: \"%s\"\n",
								pid, registro->paginaProceso, registro->frame,
								registro->estaEnUMC, registro->modificado,
								contenido);
						printf(
								"__________________________________________________________________\n");
						escribirEnArchivo(pid, registro->paginaProceso,
								registro->frame, registro->estaEnUMC,
								registro->modificado, contenido);

						free(contenido);
					}

				} else {
					log_info(ptrLog,
							"PID: %d | Pag: %d | Marco: %d | P: %d | M: %d | Contenido: \n",
							pid, registro->paginaProceso, registro->frame,
							registro->estaEnUMC, registro->modificado);
					log_info(ptrLog,
							"__________________________________________________________________\n");
					printf(
							"__________________________________________________________________\n");
					printf(
							"PID: %d | Pag: %d | Marco: %d | P: %d | M: %d | Contenido: \n",
							pid, registro->paginaProceso, registro->frame,
							registro->estaEnUMC, registro->modificado);
					escribirEnArchivo(pid, registro->paginaProceso,
							registro->frame, registro->estaEnUMC,
							registro->modificado, NULL);
				}
			}
		}
		pthread_mutex_unlock(&accesoAFrames);
	} else {
		log_info(ptrLog,
				"No se encontro Tabla de Paginas del Proceso con PID: %d", pid);
		printf("No se encontro Tabla de Paginas del Proceso con PID: %d\n",
				pid);
	}
}

void dump(uint32_t pid) {
//Me fijo si hay algo cargado en la TP
	if (list_size(tablaProcesosPaginas) != 0) {
		//caso que sea dump de 1 proceso especifico
		if (pid != NULL) {
			dumpDeUnPID(pid);
		} else {
			//caso que sea dump for all
			for (i = 0; i < list_size(tablaProcesosPaginas); i++) {
				t_tabla_de_paginas * tabla = list_get(tablaProcesosPaginas, i);
				dumpDeUnPID(tabla->pID);
			}
		}
	} else {
		log_info(ptrLog, "La memoria esta vacia");
		printf("La memoria esta vacia\n");
	}
}

int punteroCharEsInt(char* cadena) {
	int i;
	int len = strlen(cadena);

	for (i = 0; i < len; i++) {
		if (!isdigit(cadena[i]))
			break;
	}
	return len == i ? atoi(cadena) : -1;
}

void crearConsola() {
	char* comando = malloc(30);
	char* parametro = malloc(30);
	while (1) {
		printf("Ingrese un comando: \n");
		scanf("%s %s", comando, parametro);
		if (strcmp("retardo", comando) == 0) {
			retardar(punteroCharEsInt(parametro));
			printf("\n");
		} else if (strcmp("dump", comando) == 0) {
			if (strcmp("all", parametro) == 0) {
				dump(NULL);
			} else {
				dump(atoi(parametro));
			}
			printf("\n");
		} else if (strcmp("flush_memory", comando) == 0) {
			flushMemory(atoi(parametro));
			printf("\n");
		} else if (strcmp("flushtlb", strcat(comando, parametro)) == 0) {
			tlbFlush();
			printf("\n");
		} else {
			printf("\nComando no reconocido.\n\n");
		}
	}
	free(comando);
	free(parametro);
}

void escribirEnArchivo(uint32_t pid, uint32_t paginaProceso, uint32_t frame,
		uint32_t presencia, uint32_t modificado, char* contenido) {

	FILE*archivoDump = fopen("dump.txt", "a");

	if (contenido != NULL) {
		fprintf(archivoDump,
				"PID: %d \t| Pagina: %d \t| Marco: %d \t| P: %d \t| M: %d \t| Contenido: \"%s\" \n",
				pid, paginaProceso, frame, presencia, modificado, contenido);

		fprintf(archivoDump,
				"__________________________________________________________________\n");
	} else {
		fprintf(archivoDump,
				"PID: %d \t| Pagina: %d \t| Marco: %d \t| P: %d \t| M: %d \t| Contenido: \n",
				pid, paginaProceso, frame, presencia, modificado);
		fprintf(archivoDump,
				"__________________________________________________________________\n");
	}
	fclose(archivoDump);
}
