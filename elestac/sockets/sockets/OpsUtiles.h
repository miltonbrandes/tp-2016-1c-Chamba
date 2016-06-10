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

t_buffer_tamanio* serializarSolicitarBytes(t_solicitarBytes* solicitarBytes);
t_solicitarBytes* deserializarSolicitarBytes(char * message);

t_buffer_tamanio * serializarNuevoProgEnUMC(t_nuevo_prog_en_umc * nuevoProg);
t_nuevo_prog_en_umc * deserializarNuevoProgEnUMC(char * buffer);
char* enviarOperacion(uint32_t operacion, void* estructuraDeOperacion,int serverSocket);
t_buffer_tamanio * serializarEnviarBytes(t_enviarBytes* enviarBytes);
t_buffer_tamanio * serializarCambioProcActivo(t_cambio_proc_activo* cambioProcActivo);
t_buffer_tamanio * serializarCrearSegmento(t_iniciar_programa* crearSegmento, uint32_t *operacion);

t_buffer_tamanio * serializarFinalizarPrograma(t_finalizar_programa* destruirSegmento);
t_finalizar_programa * deserializarFinalizarPrograma(char * buffer);

t_pcb* deserializar_pcb(char* package);
t_buffer_tamanio* serializar_pcb(t_pcb* pcb);

t_buffer_tamanio * serializar_EstructuraInicial(t_EstructuraInicial * estructuraInicial);
t_EstructuraInicial* deserializar_EstructuraInicial(char* package);

t_buffer_tamanio * serializar_opVarCompartida(t_op_varCompartida* varCompartida);
t_op_varCompartida* deserializar_opVarCompartida(char* package);

t_buffer_tamanio* serializadoIndiceDeCodigo(t_list* indiceCodigo);
t_list* deserializarIndiceCodigo(char* indice, uint32_t tam);

t_buffer_tamanio* serializarIndiceStack(t_list* indiceStack);
t_list* deserializarIndiceStack(char* indice);

t_list* llenarLista(t_intructions * indiceCodigo, t_size cantInstruc);
t_list* llenarStack(t_list * lista, t_stack* lineaAgregar);

t_buffer_tamanio * serializarUint32(uint32_t number);
uint32_t deserializarUint32(char *package);

t_buffer_tamanio * serializarListaPaginaFrame(t_list * lista);
t_list * deserializarListaPaginaFrame(char * mensaje);

t_buffer_tamanio * serializarIniciarPrograma(t_iniciar_programa * iniciarPrograma);
t_iniciar_programa * deserializarIniciarPrograma(char * buffer);

t_buffer_tamanio * serializarSolicitudPagina(t_solicitud_pagina * solicitud);
t_solicitud_pagina * deserializarSolicitudPagina(char * buffer);

t_buffer_tamanio * serializarPaginaDeSwap(t_pagina_de_swap * pagina, uint32_t tamanioPagina);
t_pagina_de_swap * deserializarPaginaDeSwap(char * message);

t_buffer_tamanio * serializarEscribirEnSwap(t_escribir_en_swap * escribirEnSwap, uint32_t marcoSize);
t_escribir_en_swap * deserializarEscribirEnSwap(char * buffer);

t_buffer_tamanio * serializarCheckEspacio(t_check_espacio * check);
t_check_espacio * deserializarCheckEspacio(char * buffer);

t_buffer_tamanio * serializarInstruccion(t_instruccion * instruccion, int tamanioInstruccion);
t_instruccion * deserializarInstruccion(char * message);

#endif /* SOCKETS_UTILES_OPSUTILES_H_ */
