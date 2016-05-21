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
	pthread_t hiloCpu;
} t_cpu;

#define MAX_BUFFER_SIZE 4096

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
int marcos, marcosSize, marcoXProc, entradasTLB, retardo, tlbHabilitada;

char *ipSwap;
int i = 0;

//Variables frames, tlb
t_list * framesOcupados;
t_list * framesVacios;
t_list * listaCpus;

//Variables Hilos
pthread_t hiloConexiones;

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

	if (config_has_property(config, "ENTRADAS_TLB")) {
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

	if (config_has_property(config, "TLB_HABILITADA")) {
		tlbHabilitada = config_get_int_value(config, "TLB_HABILITADA");
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

void datosEnSocketReceptorNucleoCPU(int socketNuevaConexion) {
	char *buffer;
	uint32_t id;
	uint32_t operacion;
	buffer = recibirDatos(socketSwap, &operacion, &id);
	int bytesRecibidos = strlen(buffer);

	if (bytesRecibidos < 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos en un Socket Nucleo o CPU");
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
		log_info(ptrLog, "No se recibieron datos de Swap. Se cierra la conexion");
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

void AceptarConexionCpu(){
	int socketCpu = AceptarConexionCliente(socketReceptorCPU);
	if (socketCpu < 0) {
		log_info(ptrLog, "Ocurrio un error al intentar aceptar una conexion de CPU");
	} else {
		log_info(ptrLog, "Nueva conexion de CPU");
		t_cpu* cpu;
		int num = list_size(listaCpus);
		cpu->socket = socketCpu;
		cpu->numCpu = num + 1;
		cpu->enUso = false;
		list_add(listaCpus, cpu);
		}
}

int recibirPeticionesCpu(char* datosRecibidos){
	return 0;
}

void finalizarPrograma(PID){
}

int checkDisponibilidadPaginas(paginasRequeridas){
	int hayEspacio = enviarMensajeASwap(paginasRequeridas, 1); // devuelve -1 si hay error, sino ?
	return hayEspacio;
}

int recibirPeticionesNucleo(){
	int operacion;
	int id;

	char* mensajeRecibido = recibirDatos(socketClienteNucleo,operacion,id);
	operacion = deserializarUint32(mensajeRecibido);
	id = deserializarUint32(mensajeRecibido);
	int tamanio = deserializarUint32(mensajeRecibido);
	int paginasRequeridas= deserializarUint32(mensajeRecibido);

	int PID;
	if (operacion == 1) { //INICIAR
		int check = checkDisponibilidadPaginas(paginasRequeridas); //pregunto a swap si tiene paginas
		if (check != -1) {
			PID = fork(); // a que estructura agrego PID?
			//enviarMensajeASwap(paginasRequeridas, 1); //pido las paginas
			enviarMensajeANucleo("Se inicializo el programa",operacion);
		} else {
			enviarMensajeANucleo("Error al inicializar programa, no hay espacio", operacion);
		}
	}
	if (operacion == 4) {
		finalizarPrograma(PID);
	} else {
		enviarMensajeANucleo("Error, operacion no reconocida",operacion);
	}
}



void manejarConexionesRecibidas() {

	socketClienteNucleo = AceptarConexionCliente(socketReceptorNucleo);
	if (socketClienteNucleo < 0) {
		log_info(ptrLog, "Ocurrio un error al intentar aceptar una conexion de Nucleo");
	} else {
		log_info(ptrLog, "Se conecto Nucleo");
	}

	listaCpus = list_create();

	while (1) {
		log_info(ptrLog, "Esperando conexiones CPU");
		pthread_t hiloCpu;
		pthread_create(&hiloCpu, NULL, (void*) AceptarConexionCpu, NULL);
		t_cpu* cpu = list_take_and_remove(listaCpus, list_size(listaCpus));
		cpu->hiloCpu = hiloCpu;
		list_add(listaCpus, cpu);
	}
}

char * reservarMemoria(int cantidadMarcos, int tamanioMarco) {
	// calloc me la llena de \0
	char * memoria = calloc(cantidadMarcos, tamanioMarco);
	return memoria;
}

void enviarMensajeANucleo(char *mensajeNucleo, int operacion) {
 	log_info(ptrLog, "Envio mensaje a Swap: %s", mensajeNucleo);
 	int tamanio = strlen(mensajeNucleo);
 	int sendBytes = enviarDatos(socketClienteNucleo,mensajeNucleo,tamanio, operacion, 5); //5 es UMC
 }

void enviarMensajeASwap(char *mensajeSwap, int operacion) {
 	log_info(ptrLog, "Envio mensaje a Swap: %s", mensajeSwap);
 	int tamanio = strlen(mensajeSwap);
 	int sendBytes = enviarDatos(socketSwap,mensajeSwap,tamanio, operacion, 5); //5 es UMC
 }

void solicitarBytesDePagina(uint32_t pagina, uint32_t offset, uint32_t tamanio){
}

void almacenarBytesEnPagina(uint32_t pagina, uint32_t offset, uint32_t tamanio, char* buffer){
}

int main() {
	if (init()) {

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

		enviarMensajeASwap("Abrimos bien, soy la umc", 5); //operacion handshake

		pthread_create(&hiloConexiones, NULL, (void*) manejarConexionesRecibidas, NULL);
		log_info(ptrLog, "Se creo el thread para manejar conexiones");
		recibirPeticionesNucleo();

	} else {
		log_info(ptrLog, "La UMC no pudo inicializarse correctamente");
		return -1;
	}
	pthread_join(hiloConexiones, NULL);
	log_info(ptrLog, "Proceso UMC finalizado");
	finalizarConexion(socketSwap);
	finalizarConexion(socketReceptorNucleo);
	finalizarConexion(socketReceptorCPU);
	return EXIT_SUCCESS;
}
