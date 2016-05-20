/*
 * StructsUtiles.h
 *
 *  Created on: 17/5/2016
 *      Author: utnso
 */

#ifndef SOCKETS_UTILES_STRUCTSUTILES_H_
#define SOCKETS_UTILES_STRUCTSUTILES_H_

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>
#include <commons/log.h>

typedef struct _package_solicitarBytes {
	uint32_t pagina;
	uint32_t start;
	uint32_t offset;
} t_solicitarBytes;
typedef struct{
	uint32_t pagina;
	uint32_t offset;
	uint32_t size;
}t_argumento;
typedef struct stack{
	uint32_t posicion;
	t_list* argumentos;
	t_list* variables;
	uint32_t direcretorno;
	t_argumento retVar;
}t_stack;
typedef struct pcb {
	uint32_t pcb_id;
	uint32_t codigo;
	t_list* ind_codigo;
	t_list* ind_stack;
	char* ind_etiq;
	uint32_t PC;
	uint32_t posicionPrimerPaginaCodigo;
	uint32_t stackPointer;
} t_pcb;

typedef struct
{
	uint32_t idVariable;
	uint32_t pagina;
	uint32_t offset;
	uint32_t size;
}t_variable;

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

typedef struct{
	t_pcb* pcb;
	char* nombreSemaforo;
}t_pcbBlockedSemaforo;

typedef struct indiceCodigo{
	uint32_t offset;
	uint32_t start;
}t_indice_codigo;

typedef struct package_iniciar_programa {
	uint32_t programID;
	uint32_t tamanio;
} t_iniciar_programa;

typedef struct _package_cambio_proc_activo {
	uint32_t programID;
} t_cambio_proc_activo;

typedef struct _package_enviarBytes {
	uint32_t pagina;
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
	char *nombre;
	int tiempo;
} t_dispositivo_io;

typedef struct {
	char* nombre;
	int valor;
} t_semaforo;

typedef struct {
	char *nombre;
	int valor;
} t_variable_compartida;

typedef struct _package_finalizar_programa {
	uint32_t programID;
} t_finalizar_programa;

typedef struct {
	int socket;
	int32_t pid;
	bool terminado;
}t_socket_pid;

#endif /* SOCKETS_UTILES_STRUCTSUTILES_H_ */
