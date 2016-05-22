/*
 * OpsUtiles.h
 *
 *  Created on: 17/5/2016
 *      Author: utnso
 */

#ifndef SOCKETS_UTILES_OPSUTILES_H_
#define SOCKETS_UTILES_OPSUTILES_H_
#include <parser/metadata_program.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/string.h>
#include "ServidorFunciones.h"
#include "ClienteFunciones.h"
#include "EscrituraLectura.h"
#include <pthread.h>
#include <semaphore.h>
#include <commons/collections/queue.h>
#include "StructsUtiles.h"

char* serializarSolicitarBytes(t_solicitarBytes* solicitarBytes);
t_solicitarBytes* deserializarSolicitarBytes(char * message);

char * serializarNuevoProgEnUMC(t_nuevo_prog_en_umc * nuevoProg);
t_nuevo_prog_en_umc * deserializarNuevoProgEnUMC(char * buffer);

char* serializarEnviarBytes(t_enviarBytes* enviarBytes);
char* serializarCambioProcActivo(t_cambio_proc_activo* cambioProcActivo, uint32_t *operacion);
char* serializarCrearSegmento(t_iniciar_programa* crearSegmento, uint32_t *operacion);
char* serializarDestruirSegmento(t_finalizar_programa* destruirSegmento, uint32_t *operacion);

t_pcb* deserializar_pcb(char* package);
char* serializar_pcb(t_pcb* pcb);

char* serializar_EstructuraInicial(t_EstructuraInicial * estructuraInicial);
t_EstructuraInicial* deserializar_EstructuraInicial(char* package);

char* serializar_opVarCompartida(t_op_varCompartida* varCompartida);
t_op_varCompartida* deserializar_opVarCompartida(char* package);

t_buffer_tamanio* serializadoIndiceDeCodigo(t_list* indiceCodigo);
t_list* deserializarIndiceCodigo(char* indice, uint32_t tam);

t_buffer_tamanio* serializarIndiceStack(t_list* indiceStack);
t_list* deserializarIndiceStack(char* indice, uint32_t tamanioTotal);

t_list* llenarLista(t_list * lista, t_intructions * indiceCodigo, t_size cantInstruc);
t_list* llenarStack(t_list * lista, t_stack* lineaAgregar);

char* serializarUint32(uint32_t number);
uint32_t deserializarUint32(char *package);

char* serializarListaPaginaFrame(t_list * lista);
t_list * deserializarListaPaginaFrame(char * mensaje);

char * serializarIniciarPrograma(t_iniciar_programa * iniciarPrograma);
t_iniciar_programa * deserializarIniciarPrograma(char * buffer);

#endif /* SOCKETS_UTILES_OPSUTILES_H_ */
