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
int datoGuardado = 0;
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

int posibleProximaVictimaClock;

//Variables Hilos
pthread_t hiloConexiones;
pthread_t hiloCpu;
pthread_t hiloNucleo;
pthread_t hiloConsola;
//variables semaforos
sem_t semEmpezarAceptarCpu;

int main() {
	if (init()) {
		tablaProcesosPaginas = list_create();
		frames = list_create();

		iniciarTLB();

		sem_init(&semEmpezarAceptarCpu, 1, 0);
		reservarMemoria(marcos,marcosSize);

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
		pthread_create(&hiloConexiones, NULL, (void *) manejarConexionesRecibidas, NULL);
		crearConsola();
	} else {
		log_info(ptrLog, "La UMC no pudo inicializarse correctamente");
		return -1;
	}
	log_info(ptrLog, "Proceso UMC finalizado");
//	pthread_join(hiloConexiones, NULL);
	free(config);
	finalizarTLB();
	finalizarConexion(socketSwap);
	finalizarConexion(socketReceptorNucleo);
	finalizarConexion(socketReceptorCPU);
	return EXIT_SUCCESS;
}

char * enviarYRecibirMensajeSwap(t_buffer_tamanio * bufferTamanio, uint32_t operacion) {
	pthread_mutex_lock(&comunicacionConSwap);
	char * mensajeDeSwap;

	int bytesEnviados = enviarDatos(socketSwap, bufferTamanio->buffer, bufferTamanio->tamanioBuffer, operacion, UMC);
	if(bytesEnviados <= 0) {
		log_error(ptrLog, "Ocurrio un error al enviarle un mensaje a Swap.");
		mensajeDeSwap = NULL;
	} else {
		uint32_t operacion, id;
		mensajeDeSwap = recibirDatos(socketSwap, &operacion, &id);
	}
	pthread_mutex_unlock(&comunicacionConSwap);
	return mensajeDeSwap;
}

void reservarMemoria(int cantidadMarcos, int tamanioMarco){
	int i;
	for(i = 0; i < cantidadMarcos; i++) {
		t_frame * frame = malloc((sizeof(uint32_t) * 3) + tamanioMarco);
		frame->bitDeReferencia = 0;
		frame->disponible = 1;
		frame->numeroFrame = i;
		frame->contenido = malloc(tamanioMarco);

		list_add(frames, frame);
	}

	posibleProximaVictimaClock = 0;
}

/////HAY QUE MODIFICAR COMO SE ENVIAN LOS MENSAJES
void enviarMensajeANucleo(char *mensajeNucleo, int operacion) {
 	log_info(ptrLog, "Envio mensaje a Nucleo: %s", mensajeNucleo);
 	int tamanio = strlen(mensajeNucleo);
 	enviarDatos(socketClienteNucleo,mensajeNucleo,tamanio, operacion,UMC);
 }

void enviarMensajeASwap(char *mensajeSwap, int tamanioMensaje, int operacion) {
 	enviarDatos(socketSwap,mensajeSwap, tamanioMensaje, operacion, UMC);
 }

void enviarMensajeACpu(char *mensaje, int operacion, int socketCpu) {
 	log_info(ptrLog, "Envio mensaje a Cpu: %s", mensaje);
 	int tamanio = strlen(mensaje);
 	enviarDatos(socketCpu,mensaje,tamanio, operacion, UMC);
 }
////////////////////////////////////////////////////////////////////////////

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

	if (config_has_property(config, "MARCOS")) {
		marcos = config_get_int_value(config, "MARCOS");
	} else {
		log_info(ptrLog, "El archivo de configuracion no contiene la clave MARCOS");
		return 0;
	}

	if (config_has_property(config, "MARCOS_SIZE")) {
		marcosSize = config_get_int_value(config, "MARCOS_SIZE");
	} else {
		log_info(ptrLog, "El archivo de configuracion no contiene la clave MARCOS_SIZE");
		return 0;
	}

	if (config_has_property(config, "MARCO_X_PROC")) {
		marcoXProc = config_get_int_value(config, "MARCO_X_PROC");
	} else {
		log_info(ptrLog, "El archivo de configuracion no contiene la clave MARCO_X_PROC");
		return 0;
	}

	if (config_has_property(config, "ENTRADAS_TLB")) { // =0 -> deshabilitada
		entradasTLB = config_get_int_value(config, "ENTRADAS_TLB");
	} else {
		log_info(ptrLog, "El archivo de configuracion no contiene la clave ENTRADAS_TLB");
		return 0;
	}

	if (config_has_property(config, "RETARDO")) {
		retardo = config_get_int_value(config, "RETARDO");
	} else {
		log_info(ptrLog, "El archivo de configuracion no contiene la clave RETARDO");
		return 0;
	}

	if (config_has_property(config, "ALGORITMO")) {
		algoritmoReemplazo = config_get_string_value(config, "ALGORITMO");
	} else {
		log_info(ptrLog, "El archivo de configuracion no contiene la clave ALGORITMO");
		return 0;
	}

	return 1;
}

int cargarValoresDeConfig() {
	config = config_create(getenv("UMC_CONFIG"));

	if (config) {
		if (config_has_property(config, "PUERTO_CPU")) {
			puertoTCPRecibirConexionesCPU = config_get_int_value(config, "PUERTO_CPU");
		} else {
			log_info(ptrLog, "El archivo de configuracion no contiene la clave PUERTO_CPU");
			return 0;
		}

		if (config_has_property(config, "PUERTO_NUCLE0")) {
			puertoTCPRecibirConexionesNucleo = config_get_int_value(config, "PUERTO_NUCLE0");
		} else {
			log_info(ptrLog, "El archivo de configuracion no contiene la clave PUERTO_NUCLE0");
			return 0;
		}

		if (config_has_property(config, "IP_SWAP")) {
			ipSwap = config_get_string_value(config, "IP_SWAP");
		} else {
			log_info(ptrLog, "El archivo de configuracion no contiene la clave IP_SWAP");
			return 0;
		}

		if (config_has_property(config, "PUERTO_SWAP")) {
			puertoReceptorSwap = config_get_int_value(config, "PUERTO_SWAP");
		} else {
			log_info(ptrLog, "El archivo de configuracion no contiene la clave PUERTO_SWAP");
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

void manejarConexionesRecibidas(){
	listaCpus = list_create();
	socketClienteNucleo = AceptarConexionCliente(socketReceptorNucleo);
	if (socketClienteNucleo < 0) {
		log_info(ptrLog,
				"Ocurrio un error al intentar aceptar una conexion de Nucleo");
	} else {
		log_info(ptrLog, "Se conecto Nucleo");
		enviarTamanioPaginaANUCLEO();
	}
	pthread_create(&hiloNucleo, NULL, (void *) recibirPeticionesNucleo, NULL);
	//recibirPeticionesNucleo();
	log_info(ptrLog, "Esperando conexiones CPU");
	pthread_create(&hiloCpu, NULL,(void *) aceptarConexionCpu, NULL);
	//aceptarConexionCpu();

	pthread_join(hiloNucleo, NULL);
	pthread_join(hiloCpu, NULL);
}

void recibirPeticionesNucleo(){
	uint32_t operacion;
	uint32_t id;

	while (1) {

		log_info(ptrLog, "Esperando Peticion de Nucleo");
		char* mensajeRecibido = recibirDatos(socketClienteNucleo, &operacion, &id);

		if(strcmp(mensajeRecibido, "ERROR") == 0) {
			log_error(ptrLog, "No se recibio nada de Nucleo, se cierra la conexion");
			finalizarConexion(socketClienteNucleo);
			free(mensajeRecibido);
			return;
		}else{
			if (operacion == NUEVOPROGRAMA) {
				t_iniciar_programa *iniciarProg = deserializarIniciarPrograma(mensajeRecibido);

				if(iniciarProg->tamanio > marcoXProc) {
					log_info(ptrLog, "No se puede iniciar el Proceso %d porque supera la cantidad de paginas permitidas", iniciarProg->programID);
					notificarProcesoNoIniciadoANucleo();
				}else{
					log_info(ptrLog, "Nucleo quiere iniciar Proceso %d. Vemos si Swap tiene espacio.", iniciarProg->programID);

					uint32_t pudoSwap = checkDisponibilidadPaginas(iniciarProg); //pregunto a swap si tiene paginas

					if (pudoSwap == SUCCESS) {
						log_info(ptrLog, "Proceso %d almacenado.", iniciarProg->programID);
						log_info(ptrLog, "Cantidad de Procesos Actuales: %d", list_size(tablaProcesosPaginas) + 1);
						t_nuevo_prog_en_umc * iniciarPrograma = inicializarProceso(iniciarProg);
						notificarProcesoIniciadoANucleo(iniciarPrograma);
					} else {
						operacion = ERROR;
						log_info(ptrLog, "No hay espacio, no inicializa el PID: %d", iniciarProg->programID);
						notificarProcesoNoIniciadoANucleo();
					}
				}
			}else if (operacion == FINALIZARPROGRAMA) {
				t_finalizar_programa *finalizar = deserializarFinalizarPrograma( mensajeRecibido); //deserializar finalizar
				uint32_t PID = finalizar->programID;
				log_info(ptrLog, "Se recibio orden de finalizacion del PID: %i", PID);
				finalizarPrograma(PID);

			} else {
				operacion = ERROR;
			}
			free(mensajeRecibido);
		}
	}
}

void notificarProcesoIniciadoANucleo(t_nuevo_prog_en_umc * nuevoPrograma) {
	t_buffer_tamanio * mensaje = serializarNuevoProgEnUMC(nuevoPrograma);
	int enviarRespuesta = enviarDatos(socketClienteNucleo, mensaje->buffer, mensaje->tamanioBuffer, NOTHING, UMC);
	if(enviarRespuesta <= 0) {
		log_error(ptrLog, "Error al notificar a Nucleo de Proceso Iniciado");
	}
}

void notificarProcesoNoIniciadoANucleo() {
	t_nuevo_prog_en_umc * nuevoPrograma = malloc(sizeof(t_nuevo_prog_en_umc));
	nuevoPrograma->primerPaginaDeProc = -1;
	nuevoPrograma->primerPaginaStack = -1;

	t_buffer_tamanio * mensaje = serializarNuevoProgEnUMC(nuevoPrograma);
	int enviarRespuesta = enviarDatos(socketClienteNucleo, mensaje->buffer, mensaje->tamanioBuffer, NOTHING, UMC);
	if(enviarRespuesta <= 0) {
		log_error(ptrLog, "Error al notificar a Nucleo de Proceso No Iniciado");
	}
}

t_nuevo_prog_en_umc * inicializarProceso(t_iniciar_programa *iniciarProg){
	t_list * tablaDePaginas = list_create();

	int i;
	for(i = 0; i < iniciarProg->tamanio; i++) {
		t_registro_tabla_de_paginas * registro = crearRegistroPag(i, -1, 0, 0);
		list_add(tablaDePaginas, registro);
	}

	t_tabla_de_paginas * tablaDeUnProceso = malloc(sizeof(uint32_t) + (sizeof(t_registro_tabla_de_paginas) * list_size(tablaDePaginas)));
	tablaDeUnProceso->pID = iniciarProg->programID;
	tablaDeUnProceso->tablaDePaginas = tablaDePaginas;

	list_add(tablaProcesosPaginas, tablaDeUnProceso);

	t_nuevo_prog_en_umc * nuevoPrograma = malloc(sizeof(t_nuevo_prog_en_umc));
	nuevoPrograma->primerPaginaDeProc = 0;
	nuevoPrograma->primerPaginaStack = iniciarProg->tamanio - 2;

	enviarPaginasASwap(iniciarProg);

	return nuevoPrograma;
}

void enviarPaginasASwap(t_iniciar_programa * iniciarProg) {
	int i, offset = 0;
	uint32_t longitudCodigo = strlen(iniciarProg->codigoAnsisop);
	for(i = 0; i < iniciarProg->tamanio; i++) {
		t_escribir_en_swap * escribirEnSwap = malloc((sizeof(uint32_t) * 2) + marcosSize);
		escribirEnSwap->paginaProceso = i;
		escribirEnSwap->pid = iniciarProg->programID;

		if(offset + marcosSize < longitudCodigo) {
			escribirEnSwap->contenido = malloc(marcosSize);
			memcpy(escribirEnSwap->contenido, (iniciarProg->codigoAnsisop) + offset, marcosSize);
			offset += marcosSize;
		}else{
			uint32_t longitudACopiar = longitudCodigo - offset;
			escribirEnSwap->contenido = malloc(longitudACopiar);
			memcpy(escribirEnSwap->contenido, (iniciarProg->codigoAnsisop) + offset, longitudACopiar);
			i = iniciarProg->tamanio;
		}

		t_buffer_tamanio * buffer_tamanio = serializarEscribirEnSwap(escribirEnSwap, marcosSize);
		enviarYRecibirMensajeSwap(buffer_tamanio, ESCRIBIR);

		free(escribirEnSwap);
		free(buffer_tamanio);
	}
}

t_registro_tabla_de_paginas * crearRegistroPag(int pagina, int marco, int presencia, int modificado) {
	t_registro_tabla_de_paginas * regPagina = malloc(sizeof(t_registro_tabla_de_paginas));

	regPagina->paginaProceso = pagina;
	regPagina->frame = marco;
	regPagina->modificado = modificado;
	regPagina->estaEnUMC = presencia;

	return regPagina;
}

void aceptarConexionCpu(){
	pthread_t hiloEscuchaCpu;
	sem_wait(&semEmpezarAceptarCpu);
	int socketCpu = AceptarConexionCliente(socketReceptorCPU);
	if (socketCpu < 0) {
		log_info(ptrLog, "Ocurrio un error al intentar aceptar una conexion de CPU");
	} else {
		log_info(ptrLog, "Nueva conexion de CPU");
		enviarTamanioPaginaACPU(socketCpu);

		t_cpu* cpu = malloc(sizeof(t_cpu));
		uint32_t num = list_size(listaCpus);
		cpu->socket = socketCpu;
		cpu->numCpu = num + 1;
		cpu->procesoActivo = -1;
		pthread_create(&hiloEscuchaCpu, NULL, (void*) recibirPeticionesCpu, cpu);
		cpu->hiloCpu = hiloEscuchaCpu; // MATAR ESTE HILO ptheradExit y free(cpu) borrar cpu lista.
		list_add(listaCpus, cpu);
	}
}

void recibirPeticionesCpu(t_cpu * cpuEnAccion) {
	uint32_t operacion;
	uint32_t id;
	uint32_t socketCpu = cpuEnAccion->socket;

	while(1){
		log_debug(ptrLog, "A la espera de alguna solicitud de la CPU %d", cpuEnAccion->numCpu);
		char* mensajeRecibido = recibirDatos(socketCpu, &operacion, &id);

		if(strcmp(mensajeRecibido, "ERROR") == 0) {
			free(mensajeRecibido);
			finalizarConexion(cpuEnAccion->socket);
			free(cpuEnAccion);
		}else{
			if (operacion == LEER) {
				t_solicitarBytes *leer  = deserializarSolicitarBytes(mensajeRecibido);

				uint32_t pagina = leer->pagina;
				uint32_t start = leer->start;
				uint32_t offset = leer->offset;
				log_debug(ptrLog, "Recibo una solicitud de lectura de la CPU %d -> Pagina %d - Start %d - Offset %d", cpuEnAccion->numCpu, pagina, start, offset);

				enviarDatoACPU(cpuEnAccion, pagina, start, offset);

				free(leer);
			}else if (operacion == ESCRIBIR) {
				t_enviarBytes *escribir = deserializarEnviarBytes(mensajeRecibido);

				uint32_t pagina = escribir->pagina;
				uint32_t tamanio = escribir->tamanio;
				uint32_t offset = escribir->offset;
				char* buffer = escribir->buffer;
				log_debug(ptrLog, "Recibo una solicitud de escritura de la CPU %d -> Pagina %d - Tamanio %d - Offset %d - Buffer: %s", cpuEnAccion->numCpu, pagina, tamanio, offset, buffer);

				escribirDatoDeCPU(cpuEnAccion, pagina, offset, tamanio, buffer);

				free(escribir);
			}else if (operacion == CAMBIOPROCESOACTIVO){

				if(cpuEnAccion->procesoActivo != -1){
					tlbFlushDeUnPID(cpuEnAccion->procesoActivo);
				}

				t_cambio_proc_activo *procesoActivo = deserializarCambioProcesoActivo(mensajeRecibido);

				uint32_t PID_Activo = procesoActivo->programID;
				log_info(ptrLog, "CPU %d notifica Cambio de Proceso Activo. Proceso %d en ejecucion", cpuEnAccion->numCpu, PID_Activo);
				cpuEnAccion->procesoActivo = PID_Activo;

				free(procesoActivo);
			} else {
				operacion = ERROR;
				log_info(ptrLog, "CPU no entiendo que queres, atte: UMC");
				enviarMensajeACpu("Error, operacion no reconocida", operacion, socketCpu);
				break;
			}

			free(mensajeRecibido);
		}
	}
}

void escribirDatoDeCPU(t_cpu * cpu, uint32_t pagina, uint32_t offset, uint32_t tamanio, char * buffer) {
	t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(cpu->procesoActivo);
	if(tablaDeProceso != NULL) {
		t_registro_tabla_de_paginas * registro = buscarPaginaEnTabla(tablaDeProceso, pagina);
		if(registro != NULL) {
			int entradaTLB = pagEstaEnTLB(cpu->procesoActivo, registro->paginaProceso);
			if (entradasTLB > 0 && entradaTLB != -1) {
				log_info(ptrLog, "La Pagina %d del Proceso %d esta en la TLB", pagina, cpu->procesoActivo);

				t_tlb * registroTLB = obtenerYActualizarRegistroTLB(entradaTLB);

				char *buffAux = malloc(tamanio);
				memcpy(buffAux, buffer, strlen(buffer));
				if(tamanio > strlen(buffer)) {
					int i;
					for(i = strlen(buffer); i < tamanio; i++) {
						char * auxito = "~";
						memcpy(buffAux + i, auxito, 1);
					}
				}
				memcpy(registroTLB->contenido + offset, buffAux, tamanio);

				registro->modificado = 1;
			} else {
				sleep(retardo);
				if (registro->estaEnUMC == 1) {
					t_frame * frame = list_get(frames, registro->frame);

					char *buffAux = malloc(tamanio);
					memcpy(buffAux, buffer, strlen(buffer));
					if(tamanio > strlen(buffer)) {
						int i;
						for(i = 1; i <= (tamanio-strlen(buffer)); i++) {
							char * auxito = "~";
							memcpy(buffAux + i, auxito, 1);
						}
					}
					memcpy(frame->contenido + offset, buffAux, tamanio);

					log_debug(ptrLog, "Estado del Frame luego de escritura: %s", frame->contenido);

					agregarATLB(cpu->procesoActivo, registro->paginaProceso, frame->numeroFrame, frame->contenido);

					registro->estaEnUMC = 1;
					registro->frame = frame->numeroFrame;
					registro->modificado = 1;
					frame->bitDeReferencia = 1;
				} else {
					log_info(ptrLog, "La pagina %d, no estaba en UMC, se la pido a swap", pagina);
					t_frame * frameSolicitado = solicitarPaginaASwap(cpu, pagina);

					char *buffAux = malloc(tamanio);
					memcpy(buffAux, buffer, strlen(buffer));
					if(tamanio > strlen(buffer)) {
						int i;
						for(i = 1; i <= (tamanio-strlen(buffer)); i++) {
							char * auxito = "~";
							memcpy(buffAux + i, auxito, 1);
						}
					}
					memcpy(frameSolicitado->contenido + offset, buffAux, tamanio);

					log_debug(ptrLog, "Estado del Frame luego de escritura: %s", frameSolicitado->contenido);

					agregarATLB(cpu->procesoActivo, registro->paginaProceso, frameSolicitado->numeroFrame, frameSolicitado->contenido);

					registro->estaEnUMC = 1;
					registro->frame = frameSolicitado->numeroFrame;
					registro->modificado = 1;
					frameSolicitado->bitDeReferencia = 1;
				}
			}

			uint32_t successInt = SUCCESS;
			t_buffer_tamanio * buffer_tamanio = serializarUint32(successInt);

			int enviarBytes = enviarDatos(cpu->socket, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, NOTHING, UMC);

		}else{
			log_error(ptrLog, "El Proceso %d no tiene pagina nro %d", cpu->procesoActivo, pagina);
		}
	}else{
		log_error(ptrLog, "El Proceso %d no tiene Tabla de Paginas", cpu->procesoActivo);
	}
}

void limpiarInstruccion(char * instruccion) {
     char *p2 = instruccion;
     while(*instruccion != '\0') {
     	if(*instruccion != '~') {
			*p2++ = *instruccion++;
     	}
     	else {
     		++instruccion;
     	}
     }
     *p2 = '\0';
 }

void enviarDatoACPU(t_cpu * cpu, uint32_t pagina, uint32_t start,uint32_t offset) {
	uint32_t offsetMemcpy = 0;
	t_list * datosParaCPU = list_create();

	t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(cpu->procesoActivo);
	if (tablaDeProceso != NULL) {
		t_list * listaRegistros = registrosABuscarParaPeticion(tablaDeProceso, pagina, start, offset);
		if (listaRegistros != NULL) {
			int i;
			for (i = 0; i < list_size(listaRegistros); i++) {
				t_auxiliar_registro * auxiliar = list_get(listaRegistros,i);
				t_registro_tabla_de_paginas * registro = auxiliar->registro;

				int entradaTLB = pagEstaEnTLB(cpu->procesoActivo, registro->paginaProceso);
				if (entradasTLB > 0 && entradaTLB != -1) {
					log_info(ptrLog, "La Pagina %d del Proceso %d esta en la TLB", pagina, cpu->procesoActivo);
					t_tlb * registroTLB = obtenerYActualizarRegistroTLB(entradaTLB);

					char * bufferAux = calloc(1, auxiliar->offset + 2);
					memcpy(bufferAux, (registroTLB->contenido) + (auxiliar->start) + 1, auxiliar->offset);

					int tieneCaracterEspecial = 0, i;
					for(i = 0; i < auxiliar->offset + 2; i++) {
						if(bufferAux[i] == '~') {
							tieneCaracterEspecial = 1;
						}
					}
					if(tieneCaracterEspecial == 1) {
						memcpy(bufferAux, (registroTLB->contenido) + (auxiliar->start), auxiliar->offset);
						limpiarInstruccion(bufferAux);
					}

					list_add(datosParaCPU, bufferAux);
					offsetMemcpy += auxiliar->offset + 1;

				} else {
					sleep(retardo);
					if (registro->estaEnUMC == 1) {
						t_frame * frame = list_get(frames, registro->frame);
						char * bufferAux = calloc(1, auxiliar->offset + 2);
						memcpy(bufferAux, (frame->contenido) + (auxiliar->start), auxiliar->offset);

						int tieneCaracterEspecial = 0, i;
						for(i = 0; i < auxiliar->offset + 2; i++) {
							if(bufferAux[i] == '~') {
								tieneCaracterEspecial = 1;
							}
						}
						if(tieneCaracterEspecial == 1) {
							memcpy(bufferAux, (frame->contenido) + (auxiliar->start), auxiliar->offset);
							limpiarInstruccion(bufferAux);
						}

						list_add(datosParaCPU, bufferAux);
						offsetMemcpy += auxiliar->offset + 1;

						agregarATLB(cpu->procesoActivo, registro->paginaProceso, frame->numeroFrame, frame->contenido);

						registro->estaEnUMC = 1;
						registro->frame = frame->numeroFrame;
					} else {
						log_info(ptrLog, "UMC no tiene la Pagina %d del Proceso %d. Pido a Swap", registro->paginaProceso, cpu->procesoActivo);

						t_frame * frameSolicitado = solicitarPaginaASwap( cpu, registro->paginaProceso);
						char * bufferAux = calloc(1, auxiliar->offset + 2);
						memcpy(bufferAux, (frameSolicitado->contenido) + (auxiliar->start) + 1, auxiliar->offset);

						int tieneCaracterEspecial = 0, i;
						for(i = 0; i < auxiliar->offset + 2; i++) {
							if(bufferAux[i] == '~') {
								tieneCaracterEspecial = 1;
							}
						}
						if(tieneCaracterEspecial == 1) {
							memcpy(bufferAux, (frameSolicitado->contenido) + (auxiliar->start), auxiliar->offset);
							limpiarInstruccion(bufferAux);
						}

						list_add(datosParaCPU, bufferAux);
						offsetMemcpy += auxiliar->offset + 1;

						agregarATLB(cpu->procesoActivo, registro->paginaProceso, frameSolicitado->numeroFrame, frameSolicitado->contenido);

						registro->estaEnUMC = 1;
						registro->frame = frameSolicitado->numeroFrame;
					}
				}
			}

			free(listaRegistros);
		}
	}

	char *instruccionPosta = calloc(list_size(datosParaCPU), 50);
	for(i = 0; i < list_size(datosParaCPU); i++) {
		char * aux = list_get(datosParaCPU, i);
		strcat(instruccionPosta, aux);
	}
	log_info(ptrLog, "La instruccion que pidio CPU es: %s", instruccionPosta);
	t_instruccion * instruccion = calloc(list_size(datosParaCPU), 50);
	instruccion->instruccion = instruccionPosta;
	t_buffer_tamanio * buffer_tamanio = serializarInstruccion(instruccion, offset + 5);

	int enviarBytes = enviarDatos(cpu->socket, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, NOTHING, UMC);

//	free(buffer_tamanio);
//	free(instruccion);
//	free(datosParaCPU);
//	free(instruccionPosta);
}

t_frame * solicitarPaginaASwap(t_cpu * cpu, uint32_t pagina) {
	t_solicitud_pagina * solicitudPagina = malloc(sizeof(t_solicitud_pagina));
	solicitudPagina->pid = cpu->procesoActivo;
	solicitudPagina->paginaProceso = pagina;
	t_buffer_tamanio * buffer_tamanio = serializarSolicitudPagina(solicitudPagina);
	char * mensajeDeSwap = enviarYRecibirMensajeSwap(buffer_tamanio, LEER);

	if(strcmp(mensajeDeSwap, "ERROR") == 0) {
		log_error(ptrLog, "Ocurrio un error al Solicitar una Pagina a Swap");
		return NULL;
	}else{
		t_pagina_de_swap * paginaSwap = deserializarPaginaDeSwap(mensajeDeSwap);
		log_info(ptrLog, "Swap envia la Pagina %d del Proceso %d -> %s", pagina, cpu->procesoActivo, paginaSwap->paginaSolicitada);

		t_frame * frame = agregarPaginaAUMC(paginaSwap, cpu->procesoActivo, pagina);
		return frame;
	}
	return NULL;
}

t_list * registrosABuscarParaPeticion(t_tabla_de_paginas * tablaDeProceso, uint32_t pagina, uint32_t start, uint32_t offset) {
	t_list * registros = list_create();

	//Armo el primer request
	t_registro_tabla_de_paginas * registro = buscarPaginaEnTabla(tablaDeProceso, pagina);
	t_auxiliar_registro * auxiliar = malloc(sizeof(t_registro_tabla_de_paginas) + (sizeof(uint32_t)*2));
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
			t_registro_tabla_de_paginas * registroAdicional = buscarPaginaEnTabla(tablaDeProceso, pagina);
			t_auxiliar_registro * auxiliarAdicional = malloc(sizeof(t_registro_tabla_de_paginas) + (sizeof(uint32_t)*2));
			auxiliarAdicional->registro = registroAdicional;
			auxiliarAdicional->start = start;
			auxiliarAdicional->offset = marcosSize;
			offset = offset - marcosSize;
			list_add(registros, auxiliarAdicional);
		}

		if(offset >= 0 && (start + offset) < marcosSize) {
			pagina++;
			t_registro_tabla_de_paginas * registroAdicional2 = buscarPaginaEnTabla(tablaDeProceso, pagina);
			t_auxiliar_registro * auxiliarAdicional2 = malloc(sizeof(t_registro_tabla_de_paginas) + (sizeof(uint32_t)*2));
			auxiliarAdicional2->registro = registroAdicional2;
			auxiliarAdicional2->start = -1;
			//if(offset == 0) {
				auxiliarAdicional2->offset = offset;
			/*}else{
				auxiliarAdicional2->offset = offset;
			}*/
			list_add(registros, auxiliarAdicional2);
		}
	}

	return registros;
}

t_registro_tabla_de_paginas * buscarPaginaEnTabla(t_tabla_de_paginas * tabla, uint32_t pagina) {
	int i;
	for(i = 0; i < list_size(tabla->tablaDePaginas); i++) {
		t_registro_tabla_de_paginas * registro = list_get(tabla->tablaDePaginas, i);
		if(registro->paginaProceso == pagina) {
			return registro;
		}
	}
	return NULL;
}

t_tabla_de_paginas * buscarTablaDelProceso(uint32_t procesoId) {
	int i;
	for(i = 0; i < list_size(tablaProcesosPaginas); i++) {
		t_tabla_de_paginas * tabla = list_get(tablaProcesosPaginas, i);
		if(tabla->pID == procesoId) {
			return tabla;
		}
	}
	return NULL;
}

int indiceDeTablaDelProceso(uint32_t procesoId) {
	int i;
	for(i = 0; i < list_size(tablaProcesosPaginas); i++) {
		t_tabla_de_paginas * tabla = list_get(tablaProcesosPaginas, i);
		if(tabla->pID == procesoId) {
			return i;
		}
	}
	return i;
}

void enviarTamanioPaginaACPU(int socketCPU) {
	t_buffer_tamanio * buffer_tamanio = serializarUint32(marcosSize);
	int bytesEnviados = enviarDatos(socketCPU, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, ENVIAR_TAMANIO_PAGINA_A_CPU, UMC);
	if(bytesEnviados<=0) {
		log_info(ptrLog, "No se pudo enviar Tamanio de Pagina a CPU");
	}
	free(buffer_tamanio);
}

void enviarTamanioPaginaANUCLEO(){
	t_buffer_tamanio * buffer_tamanio = serializarUint32(marcosSize);
	int bytesEnviados = enviarDatos(socketClienteNucleo, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, TAMANIO_PAGINAS_NUCLEO, UMC);
	if(bytesEnviados<=0) {
		log_info(ptrLog, "No se pudo enviar Tamanio de Pagina a CPU");
	}
	sem_post(&semEmpezarAceptarCpu);
	free(buffer_tamanio);
}

void finalizarPrograma(uint32_t PID){
	t_finalizar_programa * finalizarProg = malloc(sizeof(t_finalizar_programa));
	finalizarProg->programID = PID;

	t_buffer_tamanio * buffer_tamanio = serializarFinalizarPrograma(finalizarProg);

	char * respuestaSwap = enviarYRecibirMensajeSwap(buffer_tamanio, FINALIZARPROGRAMA);
	uint32_t respuesta = deserializarUint32(respuestaSwap);

	if(respuesta == SUCCESS){
		borrarEstructurasDeProceso(PID);
		log_info(ptrLog, "Se borraron las estructuras relacionadas al Proceso %d", PID);
	} else {
		log_info(ptrLog, "Ocurrio un error al borrar las estructuras relacionadas al Proceso %d", PID);
	}

	free(buffer_tamanio);
	free(finalizarProg);
	free(respuestaSwap);
}

void borrarEstructurasDeProceso(uint32_t pid) {
	t_tabla_de_paginas * tablaAEliminar = buscarTablaDelProceso(pid);
	int i;
	for(i = 0; i < list_size(tablaAEliminar->tablaDePaginas); i++) {
		t_registro_tabla_de_paginas * registro = list_get(tablaAEliminar->tablaDePaginas, i);
		if(registro->estaEnUMC == 1) {
			pthread_mutex_lock(&accesoAFrames);
			t_frame * frame = list_get(frames, registro->frame);
			frame->bitDeReferencia = 0;
			frame->disponible = 1;
			frame->numeroFrame = i;
			free(frame->contenido);
			frame->contenido = malloc(marcosSize);
			pthread_mutex_unlock(&accesoAFrames);
		}
		free(registro);
	}

	int indexDeTablaAEliminar = indiceDeTablaDelProceso(pid);
	list_remove(tablaProcesosPaginas, indexDeTablaAEliminar);
	free(tablaAEliminar);

	tlbFlushDeUnPID(pid);
}

uint32_t checkDisponibilidadPaginas(t_iniciar_programa * iniciarProg){
	t_check_espacio * check = malloc(sizeof(t_check_espacio));
	check->pid = iniciarProg->programID;
	check->cantidadDePaginas = iniciarProg->tamanio;

	t_buffer_tamanio * iniciarProgSerializado = serializarCheckEspacio(check);
	log_info(ptrLog, "Envio a Swap Cantidad de Paginas requeridas y PID: %d", iniciarProg->programID);
	enviarMensajeASwap(iniciarProgSerializado->buffer, iniciarProgSerializado->tamanioBuffer, NUEVOPROGRAMA); // enviar tmb el PID
	free(iniciarProgSerializado->buffer);
	free(iniciarProgSerializado);

	uint32_t operacion;
	uint32_t id;

	log_info(ptrLog, "Espero que Swap me diga si puede o no alojar el Proceso con PID: %d.", iniciarProg->programID);
	char* hayEspacio = recibirDatos(socketSwap, &operacion, &id);
	uint32_t pudoSwap = deserializarUint32(hayEspacio);

	free(hayEspacio);
	free(iniciarProgSerializado);
	return pudoSwap;
}

void liberarMemoria(char * memoriaALiberar){
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
	nuevoRegistroTLB->contenido = malloc(marcosSize);
	memcpy(nuevoRegistroTLB->contenido, tlbRegistro->contenido, marcosSize);

	list_remove(TLB, entradasTLB);
	list_add(TLB, nuevoRegistroTLB);
	pthread_mutex_unlock(&accesoATLB);

	return nuevoRegistroTLB;
}

int pagEstaEnTLB(int pid, int numPag){
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

		for (i = 0; i <= entradasTLB; i++) {
			t_tlb * registro = malloc((sizeof(int) * 4) + marcosSize);
			registro->pid = -1;
			registro->indice = i;
			registro->numPag = -1;
			registro->numFrame = -1;
			registro->contenido = malloc(marcosSize);

			list_add(TLB, registro);
		}
	} else {
		TLB = NULL;
	}
}

int entradaTLBAReemplazarPorLRU(){
	//Retorna -1 si no hay libres. Sino, retorna indice de Libre.
	int i = 0;
	int indiceLibre = -1;
	pthread_mutex_lock(&accesoATLB);
	if (TLB != NULL) {
		for (i = 0; i < list_size(TLB); i++) {
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

void agregarATLB(int pid, int pagina, int frame, char * contenidoFrame){
	if(entradasTLB > 0) {
		int indiceLibre = entradaTLBAReemplazarPorLRU();

		t_tlb * aInsertar = malloc(sizeof(t_tlb));
		aInsertar->numPag=pagina;
		aInsertar->numFrame=frame;
		aInsertar->pid=pid;
		aInsertar->contenido = malloc(marcosSize);
		memcpy(aInsertar->contenido, contenidoFrame, marcosSize);

		if(indiceLibre > -1) {
			aInsertar->indice = indiceLibre;
			pthread_mutex_lock(&accesoATLB);
			list_remove(TLB, indiceLibre);
			list_add_in_index(TLB, indiceLibre, aInsertar);
			pthread_mutex_unlock(&accesoATLB);
			log_info(ptrLog, "La Pagina %d del Proceso %d se agrego en la TLB", pagina, pid);
		}else{
			aInsertar->indice = list_size(TLB) - 1;
			//Actualizo el Frame en Swap antes de eliminar
			t_tlb * tlbAEliminar = list_get(TLB, 0);
			if(tlbAEliminar->numFrame != -1){
				pthread_mutex_lock(&accesoAFrames);
				t_frame * frameSwap = list_get(frames, tlbAEliminar->numFrame);
				frameSwap->contenido = malloc(marcosSize);
				memcpy(frameSwap->contenido, tlbAEliminar->contenido, marcosSize);
				pthread_mutex_unlock(&accesoAFrames);
			}
			//Elimino la primera entrada => La mas vieja
			log_info(ptrLog, "Se elimina la primer entrada de la TLB", pagina, pid);
			pthread_mutex_lock(&accesoATLB);
			list_remove(TLB, 0);
			list_add(TLB, aInsertar);
			pthread_mutex_unlock(&accesoATLB);
			log_info(ptrLog, "La Pagina %d del Proceso %d esta en la TLB", pagina, pid);
		}
	}
}

void tlbFlush(){
	if ((entradasTLB) != 0) {
		finalizarTLB();
		iniciarTLB();
	}
	printf("Flush en TLB realizado");
}

void tlbFlushDeUnPID(int PID) {
	int i = 0;
	if ((entradasTLB) != 0) {
		pthread_mutex_lock(&accesoATLB);
		for (i = 0; i < list_size(TLB); i++) {
			t_tlb * registro = list_get(TLB, i);
			if (registro->pid == PID) {
				pthread_mutex_lock(&accesoAFrames);
				t_frame * frameSwap = list_get(frames, registro->numFrame);
				frameSwap->contenido = malloc(marcosSize);
				memcpy(frameSwap->contenido, registro->contenido, marcosSize);
				pthread_mutex_unlock(&accesoAFrames);
				//log_info(ptrLog, "Actualize la tlb en umc porque hubo un flush");
				registro->pid = -1;
				registro->indice = -1;
				registro->numPag = 0;
				registro->numFrame = -1;
			}
		}
		pthread_mutex_unlock(&accesoATLB);
		log_info(ptrLog, "Flush en TLB de PID: %d realizado\n", PID);
	}
}

void finalizarTLB(){
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

t_frame * agregarPaginaAUMC(t_pagina_de_swap * paginaSwap, uint32_t pid, uint32_t pagina) {
	if (!strcmp(algoritmoReemplazo, "CLOCK")) {
		return actualizarFramesConClock(paginaSwap, pid, pagina);
	}
	if (!strcmp(algoritmoReemplazo, "CLOCK_MODIFICADO")) {
		return actualizarFramesConClockModificado(paginaSwap, pid, pagina);
	} else {
		log_info(ptrLog, "Algoritmo de reemplazo no reconocido");
		return NULL;
	}
}

t_frame * actualizarFramesConClock(t_pagina_de_swap * paginaSwap, uint32_t pid, uint32_t pagina) {
	int indiceFrameLibre = buscarFrameLibre();

	if(indiceFrameLibre > -1) {
		t_frame * frame = malloc((sizeof(uint32_t) * 3) + marcosSize);
		frame->contenido = paginaSwap->paginaSolicitada;
		frame->disponible = 0;
		frame->bitDeReferencia = 1;
		frame->numeroFrame = indiceFrameLibre;

		pthread_mutex_lock(&accesoAFrames);
		list_remove(frames, indiceFrameLibre);
		list_add_in_index(frames, indiceFrameLibre, frame);

		//Actualizo Tabla de Paginas del Proceso
		t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(pid);
		t_registro_tabla_de_paginas * registroTabla = buscarPaginaEnTabla(tablaDeProceso, pagina);
		registroTabla->estaEnUMC = 1;
		registroTabla->frame = frame->numeroFrame;
		pthread_mutex_unlock(&accesoAFrames);

		return frame;
	}else{
		return desalojarFrameConClock(paginaSwap, pid, pagina);
	}
}

t_frame * desalojarFrameConClock(t_pagina_de_swap * paginaSwap, uint32_t pid, uint32_t pagina) {
	if(posibleProximaVictimaClock >= marcos) {
		posibleProximaVictimaClock = 0;
	}

	pthread_mutex_lock(&accesoAFrames);
	t_frame * frameCandidato = list_get(frames, posibleProximaVictimaClock);
	pthread_mutex_unlock(&accesoAFrames);
	if(frameCandidato->bitDeReferencia == 1) {
		frameCandidato->bitDeReferencia = 0;
		posibleProximaVictimaClock++;
		return desalojarFrameConClock(paginaSwap, pid, pagina);
	}else{
		t_frame * frame = malloc((sizeof(uint32_t) * 3) + marcosSize);
		frame->contenido = paginaSwap->paginaSolicitada;
		frame->disponible = 0;
		frame->bitDeReferencia = 1;
		frame->numeroFrame = posibleProximaVictimaClock;
		chequearSiHayQueEscribirEnSwapLaPagina(posibleProximaVictimaClock);

		int posEnTlb = pagEstaEnTLB(pid, pagina);
		if(posEnTlb < list_size(TLB) && posEnTlb != -1){
			//la tengo que eliminar
			pthread_mutex_lock(&accesoATLB);
			t_tlb * aEliminar =	list_get(TLB, posEnTlb);
			list_remove(TLB, aEliminar->indice);
			pthread_mutex_unlock(&accesoATLB);
		}
		pthread_mutex_lock(&accesoAFrames);
		list_remove(frames, posibleProximaVictimaClock);
		list_add_in_index(frames, posibleProximaVictimaClock, frame);
		posibleProximaVictimaClock++;

		//Actualizo Tabla de Paginas del Proceso
		t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(pid);
		t_registro_tabla_de_paginas * registroTabla = buscarPaginaEnTabla(tablaDeProceso, pagina);
		registroTabla->estaEnUMC = 1;
		registroTabla->frame = frame->numeroFrame;
		pthread_mutex_unlock(&accesoAFrames);

		return frame;
	}
}

t_frame * actualizarFramesConClockModificado(t_pagina_de_swap * paginaSwap, uint32_t pid, uint32_t pagina) {
	//No implementado
	return NULL;
}

void chequearSiHayQueEscribirEnSwapLaPagina(int nroFrame) {
	int i, j;
	for(i = 0; i < list_size(tablaProcesosPaginas); i++) {
		t_tabla_de_paginas * tabla = list_get(tablaProcesosPaginas, i);
		for(j = 0; j < list_size(tabla->tablaDePaginas); j++) {
			t_registro_tabla_de_paginas * registroTabla = list_get(tabla->tablaDePaginas, j);
			if(registroTabla->estaEnUMC && registroTabla->frame == nroFrame) {
				if(registroTabla->modificado == 1) {
					escribirFrameEnSwap(nroFrame, tabla->pID, registroTabla->paginaProceso);
				}
				registroTabla->estaEnUMC = 0;
				registroTabla->frame = -1;
				registroTabla->modificado = 0;
				break;
			}
		}
	}
}

void escribirFrameEnSwap(int nroFrame, uint32_t pid, uint32_t pagina) {
	pthread_mutex_lock(&accesoAFrames);
	t_frame * frame = list_get(frames, nroFrame);

	t_escribir_en_swap * escribirEnSwap = malloc((sizeof(uint32_t) * 2) + marcosSize);
	escribirEnSwap->paginaProceso = pagina;
	escribirEnSwap->pid = pid;
	escribirEnSwap->contenido = malloc(marcosSize);
	memcpy(escribirEnSwap->contenido, frame->contenido, marcosSize);
	pthread_mutex_unlock(&accesoAFrames);

	t_buffer_tamanio * buffer_tamanio = serializarEscribirEnSwap(escribirEnSwap, marcosSize);
	enviarYRecibirMensajeSwap(buffer_tamanio, ESCRIBIR);

	free(buffer_tamanio);
	free(escribirEnSwap);
}

int buscarFrameLibre() {
	pthread_mutex_lock(&accesoAFrames);
	int i, j = -1;
	for(i = 0; i < list_size(frames); i++) {
		t_frame * frame = list_get(frames, i) ;
		if(frame->disponible == 1) {
			j = i;
			break;
		}
	}
	pthread_mutex_unlock(&accesoAFrames);
	return j;
}

void retardar(uint32_t retardoNuevo) {
	if (retardoNuevo != NULL) {
		retardo = retardoNuevo;
		printf("Se modifico el retardo a:%d", retardoNuevo);
	} else {
		printf("No se modifico el retardo, sigue siendo:%d", retardo);
	}
}

void flushMemory(uint32_t pid) {
	t_tabla_de_paginas * tablaAModificar = buscarTablaDelProceso(pid);
	int i;

	for (i = 0; i < list_size(tablaAModificar->tablaDePaginas); i++) {
		pthread_mutex_lock(&accesoAFrames);
		t_registro_tabla_de_paginas * registro = list_get(
				tablaAModificar->tablaDePaginas, i);

		registro->modificado = 1;

		pthread_mutex_unlock(&accesoAFrames);
		free(registro);

		printf("Se realizo flush a las TP del PID:%d", pid);
	}

}

void dumpDeUnPID(uint32_t pid) {

	//traigo la tabla del proceso
	t_tabla_de_paginas * tablaAMostrar = buscarTablaDelProceso(pid);
	int i;

	if (tablaAMostrar != NULL) {
		for (i = 0; i < list_size(tablaAMostrar->tablaDePaginas); i++) {
			//Traigo cada tabla de paginas
			t_registro_tabla_de_paginas * registro = list_get(
					tablaAMostrar->tablaDePaginas, i);

			if (registro->estaEnUMC == 1) {
				//CORROBORAR QUE VAYAN ESTOS SEMAFOROS
				pthread_mutex_lock(&accesoAFrames);
				t_frame * frame = list_get(frames, registro->frame);
				log_info(ptrLog, "Proceso: %d ; Pagina: %d ; Marco: %d ; Contenido: \"%s\"", pid,
						registro->paginaProceso, registro->frame, frame->contenido);

				escribirEnArchivo(pid,registro->paginaProceso,registro->frame, frame->contenido);

				pthread_mutex_unlock(&accesoAFrames);
			}
			free(registro);
		}
	} else {
		log_info(ptrLog, "No se encontro Tabla de Paginas del PID: %d", pid);
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
	}
}

void crearConsola() {
	char* comando = malloc(30);
	uint32_t parametro = malloc(sizeof(uint32_t));
	while (1) {
		printf("Ingrese un comando: ");
		scanf("%s %s", comando, parametro);
		if (strcmp("retardo", comando) == 0) {
			retardar(atoi(parametro));
			printf("\n");
		} else if (strcmp("dump", comando) == 0) {
			dump(atoi(parametro));
			printf("\n");
		} else if (strcmp("flush_memory", comando) == 0) {
			flushMemory(atoi(parametro));
			printf("\n");
		} else if (strcmp("flush_tlb", strcat(comando, parametro)) == 0) {
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
		char* contenido) {

	FILE*archivoDump = fopen("dump.txt", "a");

	fprintf(archivoDump,
			"Proceso: %d ; Pagina: %d ; Marco: %d ; Contenido: \"%s\" \n", pid,
			paginaProceso, frame, contenido);
	fclose(archivoDump);
}
