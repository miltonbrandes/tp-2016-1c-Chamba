/*
 * estructurasNucleo.h
 *
 *  Created on: 9/5/2016
 *      Author: utnso
 */

#ifndef SRC_ESTRUCTURASNUCLEO_H_
#define SRC_ESTRUCTURASNUCLEO_H_
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>
#include <commons/log.h>
typedef struct pcb {
	int32_t pcb_id;
	uint32_t seg_codigo;
	uint32_t seg_stack;
	uint32_t cursor_stack;
	uint32_t index_codigo;
	uint32_t index_etiq;
	uint32_t tamanio_index_etiquetas;
	uint32_t tamanio_contexto;
	uint32_t PC;
} t_pcb;
typedef struct{
	t_pcb* pcb;
	int32_t valor;
}t_solicitudes;

typedef struct estructuraInicial {
	uint32_t Quantum;
	uint32_t RetardoQuantum;
} t_EstructuraInicial;

typedef struct clienteCpu{
	int socket;
	uint32_t id;
	t_pcb* pcbAsignado;
	bool fueAsignado;
	bool programaCerrado;
}t_clienteCpu;

typedef struct{
	char* valor;
	uint32_t PCB_ID;
	uint32_t tipoDeValor;
}t_imprimibles;

typedef struct{
	int32_t ID_IO;
	char* nombre;
	int32_t valor;
	sem_t semaforo;
	t_queue *colaSolicitudes; 		//va a ser una cola de t_solicitudes
}t_IO;

//Socket que recibe conexiones de CPU y Consola
int socketReceptorCPU, socketReceptorConsola;
int socketUMC;

int *listaCpus;
int i = 0;
uint32_t tamaniostack;
int32_t pcbAFinalizar; // pcb q vamos a cerrar porque el programa cerro mal
t_config* config;
//Archivo de Log
t_log* ptrLog;

//Variables de configuracion para establecer Conexiones
int puertoReceptorCPU, puertoReceptorConsola;
int puertoConexionUMC;
char *ipUMC;

typedef struct{
	t_pcb* pcb;
	char* nombreSemaforo;
}t_pcbBlockedSemaforo;

//Variables propias del Nucleo
int quantum, quantumSleep;
t_list *listaDispositivosIO, *listaSemaforos, *listaVariablesCompartidas;
extern int32_t pcbAFinalizar;
typedef struct indiceCodigo{
	uint32_t offset;
	uint32_t tamanio;
}t_indice_codigo;

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

typedef struct op_varCompartida {
	char* nombre;
	uint32_t longNombre;
	uint32_t valor;
} t_op_varCompartida;



typedef struct {
	int socket;
	int32_t pid;
	bool terminado;
}t_socket_pid;

#endif /* SRC_ESTRUCTURASNUCLEO_H_ */
