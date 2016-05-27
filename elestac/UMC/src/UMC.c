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

//Variables frames, tlb, tablas
t_list * listaCpus;
t_list * frames;
t_list * tablaProcesosPaginas;

//Variables Hilos
pthread_t hiloConexiones;
pthread_t hiloCpu;
pthread_t hiloNucleo;

//variables semaforos
sem_t semEmpezarAceptarCpu;

int main() {
	if (init()) {
		tablaProcesosPaginas = list_create();
		frames = list_create();

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
		manejarConexionesRecibidas();

	} else {
		log_info(ptrLog, "La UMC no pudo inicializarse correctamente");
		return -1;
	}
	//pthread_join(hiloConexiones, NULL);
	log_info(ptrLog, "Proceso UMC finalizado");
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
		t_frame * frame = malloc(sizeof(uint32_t) + tamanioMarco);
		frame->numeroFrame = i;
		frame->contenido = malloc(tamanioMarco);

		list_add(frames, frame);
	}
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
		log_info(ptrLog, "El archivo de configuracion no contiene la clave TLB_HABILITADA");
		return 0;
	}

	return 1;
}

int cargarValoresDeConfig() {
	t_config* config;
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

		if (operacion == NUEVOPROGRAMA) {
			t_iniciar_programa *iniciarProg = deserializarIniciarPrograma(mensajeRecibido);

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
		}else if (operacion == FINALIZARPROGRAMA) {
			t_finalizar_programa *finalizar = deserializarFinalizarPrograma( mensajeRecibido); //deserializar finalizar
			uint32_t PID = finalizar->programID;
			log_info(ptrLog, "Se recibio orden de finalizacion del PID: %d", PID);
			finalizarPrograma(PID);

		} else {
			operacion = ERROR;
			log_info(ptrLog, "Nucleo no entiendo que queres, atte: UMC");
			enviarMensajeANucleo("Error, operacion no reconocida", operacion);
			break;
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
			memcpy(escribirEnSwap->contenido, (iniciarProg->codigoAnsisop) + offset, marcosSize - 1);
			offset += marcosSize - 1;
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

		if (operacion == LEER) {
			t_solicitarBytes *leer  = deserializarSolicitarBytes(mensajeRecibido);

			uint32_t pagina = leer->pagina;
			uint32_t start = leer->start;
			uint32_t offset = leer->offset;
			log_debug(ptrLog, "Recibo una solicitud de lectura de la CPU %d -> Pagina %d - Start %d - Offset %d", cpuEnAccion->numCpu, pagina, start, offset);

			enviarDatoACPU(cpuEnAccion, pagina, start, offset);
		}else if (operacion == ESCRIBIR) {
			t_enviarBytes *escribir = deserializarEnviarBytes(mensajeRecibido);

			uint32_t pagina = escribir->pagina;
			uint32_t tamanio = escribir->tamanio;
			uint32_t offset = escribir->offset;
			char* buffer = escribir->buffer;
			log_debug(ptrLog, "Recibo una solicitud de escritura de la CPU %d -> Pagina %d - Tamanio %d - Offset %d - Buffer: %s", cpuEnAccion->numCpu, pagina, tamanio, offset, buffer);

			escribirDatoDeCPU(cpuEnAccion, pagina, offset, tamanio, buffer);
		}else if (operacion == CAMBIOPROCESOACTIVO){
			t_cambio_proc_activo *procesoActivo = deserializarCambioProcesoActivo(mensajeRecibido);

			uint32_t PID_Activo = procesoActivo->programID;
			log_info(ptrLog, "CPU %d notifica Cambio de Proceso Activo. Proceso %d en ejecucion", cpuEnAccion->numCpu, PID_Activo);
			cpuEnAccion->procesoActivo = PID_Activo;

		} else {
			operacion = ERROR;
			log_info(ptrLog, "CPU no entiendo que queres, atte: UMC");
			enviarMensajeACpu("Error, operacion no reconocida", operacion, socketCpu);
			break;
		}
	}
}

void escribirDatoDeCPU(t_cpu * cpu, uint32_t pagina, uint32_t offset, uint32_t tamanio, char * buffer) {
	t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(cpu->procesoActivo);
	if(tablaDeProceso != NULL) {
		t_registro_tabla_de_paginas * registro = buscarPaginaEnTabla(tablaDeProceso, pagina);
		if(registro->estaEnUMC == 1) {
			t_frame * frame = list_get(frames, registro->frame);
			memcpy((frame->contenido) + offset, buffer, tamanio);
		}else{
			t_frame * frameSolicitado = solicitarPaginaASwap(cpu, pagina);
			memcpy((frameSolicitado->contenido) + offset, buffer, tamanio);
		}
	}
}

void enviarDatoACPU(t_cpu * cpu, uint32_t pagina, uint32_t start, uint32_t offset) {
	uint32_t offsetMemcpy = 0;

	t_list * datosParaCPU = list_create();

	t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(cpu->procesoActivo);
	if (tablaDeProceso != NULL) {
		t_list * listaRegistros = registrosABuscarParaPeticion(tablaDeProceso, pagina, start, offset);
		if(listaRegistros != NULL) {
			int i;
			for(i = 0; i < list_size(listaRegistros); i++) {
				t_auxiliar_registro * auxiliar = list_get(listaRegistros, i);
				t_registro_tabla_de_paginas * registro = auxiliar->registro;
				log_info(ptrLog, "Registro involucrado-> Pagina: %d - Esta: %d - Modif: %d - Frame: %d", registro->paginaProceso, registro->estaEnUMC, registro->modificado, registro->frame);
				log_info(ptrLog, "Start del Registro: %d - Offset del Registro: %d", auxiliar->start, auxiliar->offset);
				if(registro->estaEnUMC == 1) {
					t_frame * frame = list_get(frames, registro->frame);
					char * bufferAux = malloc(auxiliar->offset + 2);
					memcpy(bufferAux, (frame->contenido) + (auxiliar->start), auxiliar->offset);
					bufferAux[auxiliar->offset + 1] = '\0';
					list_add(datosParaCPU, bufferAux);
					offsetMemcpy += auxiliar->offset + 1;
				}else{
					log_info(ptrLog, "UMC no tiene la Pagina %d del Proceso %d. Pido a Swap", registro->paginaProceso, cpu->procesoActivo);
					t_frame * frameSolicitado = solicitarPaginaASwap(cpu, registro->paginaProceso);
					char * bufferAux = malloc(auxiliar->offset + 2);
					memcpy(bufferAux, (frameSolicitado->contenido) + (auxiliar->start), auxiliar->offset);
					bufferAux[auxiliar->offset + 1] = '\0';
					list_add(datosParaCPU, bufferAux);
					offsetMemcpy += auxiliar->offset + 1;
				}
			}
			free(listaRegistros);
		}
	}

	//Falta concatenar todas las instrucciones que estan en la lista datosParaCPU, y mandarle eso
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

	free(buffer_tamanio);
	free(instruccion);
	free(datosParaCPU);
	free(instruccionPosta);
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

		//Agrego siempre en el Frame 0.
		t_frame * frame = list_get(frames, 0);
		frame->contenido = paginaSwap->paginaSolicitada;
		return frame;
		//Aca hay que hacer el algoritmo que me devuelva el Frame pero
		//teniendo en cuenta ya todoo el reemplazo, osea, me devuelve un Frame
		//que solicito y metio en memoria
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

		if(offset > 0 && (start + offset) <= marcosSize) {
			pagina++;
			t_registro_tabla_de_paginas * registroAdicional2 = buscarPaginaEnTabla(tablaDeProceso, pagina);
			t_auxiliar_registro * auxiliarAdicional2 = malloc(sizeof(t_registro_tabla_de_paginas) + (sizeof(uint32_t)*2));
			auxiliarAdicional2->registro = registroAdicional2;
			auxiliarAdicional2->start = start;
			auxiliarAdicional2->offset = offset + 1;
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

///////ESTO ES POSIBLE QUE NO ESTE DEL TODOOO BIEN
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
			t_frame * frame = list_get(frames, registro->frame);
			free(frame);
		}
		free(registro);
	}

	int indexDeTablaAEliminar = indiceDeTablaDelProceso(pid);
	list_remove(tablaProcesosPaginas, indexDeTablaAEliminar);
	free(tablaAEliminar);
}

uint32_t checkDisponibilidadPaginas(t_iniciar_programa * iniciarProg){
	t_check_espacio * check = malloc(sizeof(t_check_espacio));
	check->pid = iniciarProg->programID;
	check->cantidadDePaginas = iniciarProg->tamanio;

	t_buffer_tamanio * iniciarProgSerializado = serializarCheckEspacio(check);
	log_info(ptrLog, "Envio a Swap Cantidad de Paginas requeridas y PID: %d", iniciarProg->programID);
	enviarMensajeASwap(iniciarProgSerializado->buffer, iniciarProgSerializado->tamanioBuffer, NUEVOPROGRAMA); // enviar tmb el PID
	uint32_t operacion;
	uint32_t id;

	log_info(ptrLog, "Espero que Swap me diga si puede o no alojar el Proceso con PID: %d.", iniciarProg->programID);
	char* hayEspacio = recibirDatos(socketSwap, &operacion, &id);
	uint32_t pudoSwap = deserializarUint32(hayEspacio);

	free(iniciarProgSerializado);
	return pudoSwap;
}

void liberarMemoria(char * memoriaALiberar){
	free(memoriaALiberar);
	log_info(ptrLog, "Memoria Liberada");
}

//PROBAR SI ESTO ESTA BIEN//////////////////////////////////////////////////

t_cambio_proc_activo* deserializarCambioProcesoActivo(char * mensaje) {
	t_cambio_proc_activo *respuesta = malloc(sizeof(t_cambio_proc_activo));
	int offset = 0, tmp_size = sizeof(uint32_t);
	memcpy(&(respuesta->programID), mensaje + offset, tmp_size);
	return respuesta;
}
