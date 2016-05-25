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

#define SLEEP 1000000

// ENTRADAS A LA TLB //
typedef struct {
	int pid;
	int pagina;
	char * direccion_fisica;
	int marco;
} t_tlb;

//Tabla de paginas de un Proceso
typedef struct {
	uint32_t pID;
	t_list * tablaDePaginas;
} t_tabla_de_paginas;

//Valor del dictionary de t_tabla_de_paginas. La clave es el numero de pagina
typedef struct {
	uint32_t frame;
	uint32_t modificado; //1 modificado, 0 no modificado. Para paginas de codigo es siempre 0
	uint32_t estaEnUMC; //1 esta, 0 no esta. Para ver si hay que pedir o no la pagina a SWAP.
} t_registro_tabla_de_paginas;

//PARA INCLUIR EN LA LIBRERIA
typedef struct {
	uint32_t numCpu;
	uint32_t socket;
	uint32_t procesoActivo; //inicializa en -1
	pthread_t hiloCpu;
} t_cpu;

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

//Variables frames, tlb
t_list * framesOcupados;
t_list * framesVacios;
t_list * listaCpus;

//Variables Hilos
pthread_t hiloConexiones;
pthread_t hiloCpu;
pthread_t hiloNucleo;

//variables semaforos
sem_t semEmpezarAceptarCpu;

char * reservarMemoria(int cantidadMarcos, int tamanioMarco);
int crearLog();
int iniciarUMC(t_config* config);
int cargarValoresDeConfig();
int init();
void manejarConexionesRecibidas();
void recibirPeticionesNucleo();
void enviarTamanioPaginaANUCLEO();
void aceptarConexionCpu();
void recibirPeticionesCpu(t_cpu cpuEnAccion);
void finalizarPrograma(PID);
void enviarTamanioPaginaACPU(int socketCPU);
int checkDisponibilidadPaginas(t_iniciar_programa * iniciarProg);
void liberarMemoria(char * memoriaALiberar);
void solicitarBytesDePagina(uint32_t pagina, uint32_t offset, uint32_t tamanio);
void almacenarBytesEnPagina(uint32_t pagina, uint32_t offset, uint32_t tamanio, char* buffer);
void enviarMensajeACpu(char *mensaje, int operacion, int socketCpu);
void enviarMensajeASwap(char *mensajeSwap, int tamanioMensaje, int operacion);
void enviarMensajeANucleo(char *mensajeNucleo, int operacion);
t_iniciar_programa* deserializarIniciarPrograma(char* mensaje);
t_finalizar_programa* deserializarFinalizarPrograma(char * mensaje);
t_cambio_proc_activo* deserializarCambioProcesoActivo(char * mensaje);


int main() {
	if (init()) {
		sem_init(&semEmpezarAceptarCpu, 1, 0);
		char * memoria_real = reservarMemoria(marcos,marcosSize);

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
		//pthread_create(&hiloConexiones, NULL, (void*) manejarConexionesRecibidas, NULL);
		//log_info(ptrLog, "Se creo el thread para manejar conexiones");

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

char * reservarMemoria(int cantidadMarcos, int tamanioMarco){
	// calloc me la llena de \0
	char * memoria = calloc(cantidadMarcos, tamanioMarco);
	return memoria;
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
		log_info(ptrLog, "Se recibe Peticion de Nucleo", operacion, id);

		if (operacion == NUEVOPROGRAMA) { //INICIAR
			t_iniciar_programa *iniciarProg = deserializarIniciarPrograma(mensajeRecibido);

			log_info(ptrLog, "Nucleo quiere iniciar Proceso %d. Vemos si Swap tiene espacio.", iniciarProg->programID);

			int pudoSwap = checkDisponibilidadPaginas(iniciarProg); //pregunto a swap si tiene paginas
			if (pudoSwap == SUCCESS) {
				enviarMensajeANucleo("Se inicializo el programa", pudoSwap);
			} else {
				operacion = ERROR;
				log_info(ptrLog, "No hay espacio, no inicializa el PID: %d", iniciarProg->programID);
				enviarMensajeANucleo( "Error al inicializar programa, no hay espacio", operacion);
			}
		}
		if (operacion == FINALIZARPROGRAMA) {
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

void recibirPeticionesCpu(t_cpu cpuEnAccion) {
	uint32_t operacion;
	uint32_t id;
	uint32_t socketCpu = cpuEnAccion.socket;

	while(1){
		log_debug(ptrLog, "Esperando pedidos de una cpu de lectura o escritura de pagina");
		char* mensajeRecibido = recibirDatos(socketCpu, &operacion, &id);

		if (operacion == LEER) {
			t_solicitarBytes *leer;
			leer = deserializarSolicitarBytes(mensajeRecibido);

			uint32_t pagina;
			pagina = leer->pagina;
			uint32_t start;
			start = leer->start;
			uint32_t offset;
			offset = leer->offset;
		}

		if (operacion == ESCRIBIR) {
			t_enviarBytes *escribir = deserializarEnviarBytes(mensajeRecibido);

			uint32_t pagina = escribir->pagina;
			uint32_t tamanio = escribir->tamanio;
			uint32_t offset = escribir->offset;
			char* codigoAnsisop = escribir->buffer;

		}

		if (operacion == CAMBIOPROCESOACTIVO){
			t_cambio_proc_activo *procesoActivo = deserializarCambioProcesoActivo(mensajeRecibido);

			//ACA SE HACE FLUSH

			uint32_t PID_Activo = procesoActivo->programID;
			cpuEnAccion.procesoActivo = PID_Activo;

		} else {
			operacion = ERROR;
			log_info(ptrLog, "CPU no entiendo que queres, atte: UMC");
			enviarMensajeACpu("Error, operacion no reconocida", operacion, socketCpu);
			break;
		}
	}
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

void finalizarPrograma(PID){
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
t_iniciar_programa* deserializarIniciarPrograma(char* mensaje){
	t_iniciar_programa *respuesta = malloc(sizeof(t_iniciar_programa));
	int offset = 0, tmp_size = sizeof(uint32_t);
	memcpy(&(respuesta->programID), mensaje + offset, tmp_size);
	offset += tmp_size;
	memcpy(&(respuesta->tamanio), mensaje + offset, tmp_size);
	return respuesta;
}

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
