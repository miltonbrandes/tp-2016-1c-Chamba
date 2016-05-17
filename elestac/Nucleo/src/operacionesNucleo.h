/*
 * operacionesNucleo.h
 *
 *  Created on: 15/5/2016
 *      Author: utnso
 */

#ifndef SRC_OPERACIONESNUCLEO_H_
#define SRC_OPERACIONESNUCLEO_H_
#include <parser/metadata_program.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/string.h>
#include <sockets/ServidorFunciones.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/collections/queue.h>
char* serializarSolicitarBytes(t_solicitarBytes* solicitarBytes, uint32_t *operacion);
char* serializarEnviarBytes(t_enviarBytes* enviarBytes, uint32_t *operacion);
char* serializarCambioProcActivo(t_cambio_proc_activo* cambioProcActivo, uint32_t *operacion);
char* serializarCrearSegmento(t_iniciar_programa* crearSegmento, uint32_t *operacion);
char* serializarDestruirSegmento(t_finalizar_programa* destruirSegmento,	uint32_t *operacion);
char* serializar_opVarCompartida(t_op_varCompartida* varCompartida);
t_op_varCompartida* deserializar_opVarCompartida(char** package);
t_EstructuraInicial* deserializar_EstructuraInicial(char** package);
char* serializar_EstructuraInicial(t_EstructuraInicial * estructuraInicial);
t_pcb* deserializar_pcb(char** package);
char* serializar_pcb(t_pcb* pcb);
char* serializadoIndiceDeCodigo(t_list* indiceCodigo);
char* serializarIndiceStack(t_list* indiceStack);
t_list* llenarLista(t_list* lista, t_intructions* indiceCodigo, t_size cantInstruc);
#endif /* SRC_OPERACIONESNUCLEO_H_ */
