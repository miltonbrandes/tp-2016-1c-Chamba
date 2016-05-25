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

#define SLEEP 1000000

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


//////FUNCIONES UMC//////

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
 	log_info(ptrLog, "Envio mensaje a Swap: %s", mensajeNucleo);
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

//			int pudoSwap = checkDisponibilidadPaginas(iniciarProg); //pregunto a swap si tiene paginas
			int pudoSwap = SUCCESS;
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
		log_error(ptrLog, "Error al notificar a Nucleo de Proceso Iniciado");
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

	return nuevoPrograma;
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
			char* codigoAnsisop = escribir->buffer;

		}else if (operacion == CAMBIOPROCESOACTIVO){
			t_cambio_proc_activo *procesoActivo = deserializarCambioProcesoActivo(mensajeRecibido);

			//ACA SE HACE FLUSH

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

void enviarDatoACPU(t_cpu * cpu, uint32_t pagina, uint32_t start, uint32_t offset) {
	t_tabla_de_paginas * tablaDeProceso = buscarTablaDelProceso(cpu->procesoActivo);
	if (tablaDeProceso != NULL) {
		log_info(ptrLog, "Se encontro la Tabla del Proceso %d", cpu->procesoActivo);

		t_list * listaRegistros = registrosABuscarParaPeticion(tablaDeProceso, pagina, start, offset);

		if(listaRegistros != NULL) {
			log_info(ptrLog, "Paginas involucradas en Solicitud: %d", list_size(listaRegistros));
			int i;
			for(i = 0; i < list_size(listaRegistros); i++) {
				t_registro_tabla_de_paginas * registro = list_get(listaRegistros, i);
				log_info(ptrLog, "Registro involucrado-> Pagina: %d - Esta: %d - Modif: %d - Frame: %d", registro->paginaProceso, registro->estaEnUMC, registro->modificado, registro->frame);
			}
		}
	}
}

t_list * registrosABuscarParaPeticion(t_tabla_de_paginas * tablaDeProceso, uint32_t pagina, uint32_t start, uint32_t offset) {
	log_info(ptrLog, "Tamanio de Marcos: %d", marcosSize);
	t_list * registros = list_create();

	//Armo el primer request
	t_registro_tabla_de_paginas * registro = buscarPaginaEnTabla(tablaDeProceso, pagina);
	log_info(ptrLog, "Registro principal-> Pagina: %d - Start: %d - Offset: %d", registro->paginaProceso, start, offset);

	if (((start + offset) <= (marcosSize * pagina)) || (pagina == 0 && ((start + offset) <= (marcosSize * 1)))) {
		//Es solo 1 pagina la que hay que agarrar
		start = -1;
		offset = -1;
	} else {
		offset = (start + offset) - marcosSize - (marcosSize * pagina);
		start = (marcosSize * pagina) + marcosSize;
		pagina++;
	}
	list_add(registros, registro);

	//Veo si tengo que armar mas request porque me pase de Pagina
	while ((start + offset) > (marcosSize * pagina)) {
		log_info(ptrLog, "Necesito otra pagina-> Start: %d - Offset: %d", start, offset);
		t_registro_tabla_de_paginas * registroAdicional = buscarPaginaEnTabla(tablaDeProceso, pagina);
		log_info(ptrLog, "Registro Adicional-> Pagina: %d - Start: %d - Offset: %d", registroAdicional->paginaProceso, start, offset);

		if ((start + offset) <= (marcosSize * pagina)) {
			//No tengo que buscar mas paginas
			start = -1;
			offset = -1;
		} else {
			offset = (start + offset) - marcosSize - (marcosSize * pagina);
			start = (marcosSize * pagina) + marcosSize;
		}

		list_add(registros, registroAdicional);
		pagina++;
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
}

int checkDisponibilidadPaginas(t_iniciar_programa * iniciarProg){
	t_buffer_tamanio * iniciarProgSerializado = serializarIniciarPrograma(iniciarProg);
	log_info(ptrLog, "Envio a Swap Cantidad de Paginas requeridas y PID: %d", iniciarProg->programID);
	enviarMensajeASwap(iniciarProgSerializado->buffer, iniciarProgSerializado->tamanioBuffer, NUEVOPROGRAMA); // enviar tmb el PID
	uint32_t operacion;
	uint32_t id;
	log_info(ptrLog, "Espero que Swap me diga si puede o no alojar el Proceso con PID: %d.", iniciarProg->programID);
	char* hayEspacio = recibirDatos(socketSwap, &operacion, &id);
	uint32_t pudoSwap = operacion;
	free(iniciarProgSerializado);
	return pudoSwap;
}

void liberarMemoria(char * memoriaALiberar){
	free(memoriaALiberar);
	log_info(ptrLog, "Memoria Liberada");
}

void solicitarBytesDePagina(uint32_t pagina, uint32_t offset, uint32_t tamanio){
}

void almacenarBytesEnPagina(uint32_t pagina, uint32_t offset, uint32_t tamanio, char* buffer){
}

//PROBAR SI ESTO ESTA BIEN//////////////////////////////////////////////////
t_finalizar_programa* deserializarFinalizarPrograma(char * mensaje) {
	t_finalizar_programa *respuesta = malloc(sizeof(t_finalizar_programa));
	int offset = 0, tmp_size = sizeof(uint32_t);
	memcpy(&(respuesta->programID), mensaje + offset, tmp_size);
	return respuesta;
}

t_cambio_proc_activo* deserializarCambioProcesoActivo(char * mensaje) {
	t_cambio_proc_activo *respuesta = malloc(sizeof(t_cambio_proc_activo));
	int offset = 0, tmp_size = sizeof(uint32_t);
	memcpy(&(respuesta->programID), mensaje + offset, tmp_size);
	return respuesta;
}
