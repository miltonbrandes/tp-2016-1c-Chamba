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
typedef struct{
	uint32_t pagina;
	uint32_t offset;
	uint32_t size;
}t_posicion_stack;
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
	t_list* argumentos;
	t_list* variables;
	uint32_t direcretorno;
	t_argumento * retVar;
}t_stack;
typedef struct pcb {
	uint32_t quantum;
	uint32_t quantumSleep;
	uint32_t pcb_id;
	uint32_t codigo;
	t_list* ind_codigo;
	t_list* ind_stack;
	char* ind_etiq;
	uint32_t PC;
	uint32_t paginaCodigoActual;
	uint32_t stackPointer;
	uint32_t paginaStackActual;
	uint32_t tamanioEtiquetas;
	uint32_t numeroContextoEjecucionActualStack;
	uint32_t primerPaginaStack;
} t_pcb;

typedef struct
{
	char idVariable;
	uint32_t pagina;
	uint32_t offset;
	uint32_t size;
}t_variable;

typedef struct{
	t_pcb* pcb;
	int32_t valor;
}t_solicitudes;

typedef struct estructuraInicial {
	uint32_t tamanioStack;
} t_EstructuraInicial;

typedef struct clienteCpu{
	int socket;
	uint32_t id;
	uint32_t pcbId;
	bool fueAsignado;
	bool programaCerrado;
}t_clienteCpu;

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
	char* codigoAnsisop;
} t_iniciar_programa;

typedef struct _package_cambio_proc_activo {
	uint32_t programID;
} t_cambio_proc_activo;

typedef struct _package_enviarBytes {
	uint32_t pid;
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
	uint32_t socket;
	uint32_t pid;
	bool terminado;
}t_socket_pid;

typedef struct {
	uint32_t numeroPagina;
	uint32_t numeroFrame;
} t_pagina_frame;

typedef struct {
	uint32_t primerPaginaDeProc;
	uint32_t primerPaginaStack;
} t_nuevo_prog_en_umc;

typedef struct {
	uint32_t pid;
	uint32_t paginaProceso;
} t_solicitud_pagina;

typedef struct {
	char * paginaSolicitada;
} t_pagina_de_swap;

typedef struct {
	uint32_t pid;
	uint32_t paginaProceso;
	char * contenido;
} t_escribir_en_swap;

typedef struct {
	uint32_t pid;
	uint32_t cantidadDePaginas;
} t_check_espacio;

typedef struct {
	char * instruccion;
} t_instruccion;

//auxiliares
typedef struct {
	uint32_t tamanioStack;
	void * stack;
} t_tamanio_stack_stack;

typedef struct {
	uint32_t tamanioBuffer;
	char *buffer;
} t_buffer_tamanio;

#endif /* SOCKETS_UTILES_STRUCTSUTILES_H_ */
