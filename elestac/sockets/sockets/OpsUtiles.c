/*
 * OpsUtiles.c
 *
 *  Created on: 17/5/2016
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/collections/list.h>
#include <parser/metadata_program.h>
#include "OpsUtiles.h"
#include "StructsUtiles.h"

char * serializarNuevoProgEnUMC(t_nuevo_prog_en_umc * nuevoProg) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	char * buffer = malloc(sizeof(t_nuevo_prog_en_umc));

	memcpy(buffer + offset, &(nuevoProg->primerPaginaDeProc), tmp_size);
	offset += tmp_size;
	memcpy(buffer + offset, &(nuevoProg->primerPaginaStack), tmp_size);

	return buffer;
}

t_nuevo_prog_en_umc * deserializarNuevoProgEnUMC(char * buffer) {
	t_nuevo_prog_en_umc * nuevoProg = malloc(sizeof(t_nuevo_prog_en_umc));
	int offset = 0, tmp_size = sizeof(uint32_t);

	memcpy(&(nuevoProg->primerPaginaDeProc), buffer + offset, tmp_size);
	offset += tmp_size;
	memcpy(&(nuevoProg->primerPaginaStack), buffer + offset, tmp_size);

	return nuevoProg;
}

char * serializarIniciarPrograma(t_iniciar_programa * iniciarPrograma) {
	char * buffer = malloc(sizeof(t_iniciar_programa));
	int tmp_size = sizeof(uint32_t), offset = 0;

	memcpy(buffer + offset, &(iniciarPrograma->programID), tmp_size);
	offset += tmp_size;
	memcpy(buffer + offset, &(iniciarPrograma->tamanio), tmp_size);

	return buffer;
}

t_iniciar_programa * deserializarIniciarPrograma(char * buffer) {
	t_iniciar_programa * iniciarPrograma = malloc(sizeof(t_iniciar_programa));
	int tmp_size = sizeof(uint32_t), offset = 0;

	memcpy(&(iniciarPrograma->programID), buffer + offset, tmp_size);
	offset += tmp_size;
	memcpy(&(iniciarPrograma->tamanio), buffer + offset, tmp_size);

	return iniciarPrograma;
}

char* serializarListaPaginaFrame(t_list * lista) {
	uint32_t tamanioTotal = 0;

	uint32_t itemsEnLista = list_size(lista);
	//Reservo espacio para la lista y 1 entero para indicar cuantos items tiene la lista
	tamanioTotal = (itemsEnLista * sizeof(t_pagina_frame)) + sizeof(uint32_t);

	char * buffer = malloc(tamanioTotal);

	int i, offset = 0, tmpSize = sizeof(uint32_t);
	memcpy(buffer + offset, &itemsEnLista, tmpSize);
	offset += tmpSize;

	for(i = 0; i < itemsEnLista; i++) {
		t_pagina_frame * item = list_get(lista, i);

		memcpy(buffer + offset, &(item->numeroFrame), tmpSize);
		offset += tmpSize;
		memcpy(buffer + offset, &(item->numeroPagina), tmpSize);
		offset += tmpSize;
	}

	return buffer;
}

t_list * deserializarListaPaginaFrame(char * mensaje) {
	t_list * listaPaginaFrame = list_create();

	int i, offset = 0, tmpSize = sizeof(uint32_t);

	uint32_t itemsEnLista;
	memcpy(&itemsEnLista, mensaje, tmpSize);
	offset += tmpSize;

	for(i = 0; i < itemsEnLista; i++) {
		t_pagina_frame * item = malloc(sizeof(t_pagina_frame));
		uint32_t numeroFrame, numeroPagina;

		memcpy(&numeroFrame, mensaje + offset, tmpSize);
		offset += tmpSize;
		memcpy(&numeroPagina, mensaje + offset, tmpSize);
		offset += tmpSize;

		item->numeroFrame = numeroFrame;
		item->numeroPagina = numeroPagina;

		list_add(listaPaginaFrame, item);
	}

	return listaPaginaFrame;
}

char* serializarUint32(uint32_t number) {
	int offset = 0, tmp_size = 0;
	size_t messageSize = sizeof(uint32_t);

	char *serialized = malloc(messageSize);

	tmp_size = sizeof(uint32_t);
	memcpy(serialized + offset, &number, tmp_size);

	return serialized;
}

uint32_t deserializarUint32(char *package) {
	uint32_t number;
	int offset = 0;
	int tmp_size = sizeof(uint32_t);

	memcpy(&number, package + offset, tmp_size);

	return number;
}

char* serializarSolicitarBytes(t_solicitarBytes* solicitarBytes) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	size_t packageSize = sizeof(t_solicitarBytes);
	char * paqueteSerializado = malloc(packageSize);

	memcpy(paqueteSerializado + offset, &(solicitarBytes->pagina), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(solicitarBytes->offset), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(solicitarBytes->start), tmp_size);

	return paqueteSerializado;
}

t_solicitarBytes* deserializarSolicitarBytes(char * message) {
	t_solicitarBytes *respuesta = malloc(sizeof(t_solicitarBytes));
	int offset = 0, tmp_size = sizeof(uint32_t);

	memcpy(&(respuesta->pagina), message + offset, tmp_size);
	offset += tmp_size;
	memcpy(&(respuesta->offset), message + offset, tmp_size);
	offset += tmp_size;
	memcpy(&(respuesta->start), message + offset, tmp_size);

	return respuesta;
}

char* serializarEnviarBytes(t_enviarBytes* enviarBytes) {
	int offset = 0, tmp_size = 0;
	char * paqueteSerializado = malloc((sizeof(uint32_t) * 4) + (strlen(enviarBytes->buffer) + 1));

	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(enviarBytes->pid), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(enviarBytes->pagina), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(enviarBytes->offset), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(enviarBytes->tamanio), tmp_size);
	offset += tmp_size;
	tmp_size = strlen(enviarBytes->buffer) + 1;
	memcpy(paqueteSerializado + offset, &(enviarBytes->buffer), tmp_size);

	return paqueteSerializado;
}

t_enviarBytes* deserializarEnviarBytes(char* message) {
	uint32_t pid, pagina, offset, tamanio;
	int offsetMemcpy = 0, tmp_size = sizeof(uint32_t);

	memcpy(&pid, message + offsetMemcpy, tmp_size);
	offsetMemcpy += tmp_size;
	memcpy(&pagina, message + offsetMemcpy, tmp_size);
	offsetMemcpy += tmp_size;
	memcpy(&offset, message + offsetMemcpy, tmp_size);
	offsetMemcpy += tmp_size;
	memcpy(&tamanio, message + offsetMemcpy, tmp_size);
	offsetMemcpy += tmp_size;
	tmp_size = tamanio;
	char *datos = malloc(tamanio);
	memcpy(datos, message + offsetMemcpy, tmp_size);

	t_enviarBytes * respuesta = malloc((sizeof(uint32_t) * 4) + tamanio);
	respuesta->buffer = datos;
	respuesta->offset = offset;
	respuesta->pagina = pagina;
	respuesta->tamanio = tamanio;

	return respuesta;
}

char* serializarCambioProcActivo(t_cambio_proc_activo* cambioProcActivo, uint32_t *operacion) {
	int offset = 0, tmp_size = 0;
	size_t packageSize = sizeof(cambioProcActivo->programID) + sizeof(cambioProcActivo->programID);
	char * paqueteSerializado = malloc(packageSize + sizeof(uint32_t));

	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, operacion, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(cambioProcActivo->programID);
	memcpy(paqueteSerializado + offset, &(cambioProcActivo->programID), tmp_size);

	return paqueteSerializado;
}

char* serializarDestruirSegmento(t_finalizar_programa* destruirSegmento, uint32_t *operacion) {
	int offset = 0, tmp_size = 0;
	size_t packageSize = sizeof(destruirSegmento->programID);
	char * paqueteSerializado = malloc(packageSize + sizeof(uint32_t));

	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, operacion, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(destruirSegmento->programID);
	memcpy(paqueteSerializado + offset, &(destruirSegmento->programID), tmp_size);

	return paqueteSerializado;

}

char* serializar_pcb(t_pcb* pcb) {
	int offset = 0;
	uint32_t tmp_size = 0;
	printf("\n\nPC: %d - Codigo: %d - ID: %d - PrimerPagina: %d - StackP: %d\n",
			pcb->PC, pcb->codigo, pcb->pcb_id, pcb->posicionPrimerPaginaCodigo,
			pcb->stackPointer);
	printf("Items Indice de Codigo: %d\n", list_size(pcb->ind_codigo));
	printf("Items Indice de Stack: %d\n", list_size(pcb->ind_stack));

	t_buffer_tamanio* stackSerializado = serializarIndiceStack(pcb->ind_stack);
	t_buffer_tamanio* indcod = serializadoIndiceDeCodigo(pcb->ind_codigo);

	uint32_t tamanioPCB = 0;
	tamanioPCB += stackSerializado->tamanioBuffer;
	tamanioPCB += indcod->tamanioBuffer;
	if (pcb->ind_etiq) {
		tamanioPCB += sizeof(pcb->ind_etiq);
	}
	tamanioPCB += (sizeof(uint32_t) * 5); //Cantidad de uint32_t que tiene le PCB
	tamanioPCB += sizeof(uint32_t); //Para indicar el tamanio del PCB
	printf("\nTamanio PCB: %d\n", tamanioPCB);
	char* paqueteSerializado = malloc(tamanioPCB);
	tmp_size = sizeof(uint32_t);
//Indico tamanio de PCB
	memcpy(paqueteSerializado + offset, &tamanioPCB, tmp_size);
//Comienzo a serializar PCB
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(pcb->pcb_id), tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(pcb->PC), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(pcb->posicionPrimerPaginaCodigo),
			tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(pcb->codigo), tmp_size);
	offset += tmp_size;
//agrego tamaño de indice de codigo
	tmp_size = list_size(pcb->ind_codigo);
	memcpy(paqueteSerializado + offset, &tmp_size, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	tmp_size = 2 * sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, indcod,
			tmp_size * list_size(pcb->ind_codigo));
	offset += tmp_size * list_size(pcb->ind_codigo);
	tmp_size = sizeof(pcb->ind_etiq);
//mando tamanio de etiquetas para poder despues desserializarlo
	memcpy(paqueteSerializado + offset, &(tmp_size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(pcb->ind_etiq), tmp_size);
	offset += tmp_size;

	//Serializo el Stack
	memcpy(paqueteSerializado + offset, &(stackSerializado->tamanioBuffer), tmp_size);
	offset += sizeof(uint32_t);
	tmp_size = stackSerializado->tamanioBuffer;
	memcpy(paqueteSerializado + offset, &(stackSerializado->buffer), tmp_size);
	offset += tmp_size;


	return paqueteSerializado;
}

t_pcb* deserializar_pcb(char* package) {
 	int offset = 0;
 	int tmp_size = sizeof(uint32_t);
 	uint32_t tamIndCod = 0;
 	uint32_t tamIndEt = 0;
 	uint32_t tamIndSt = 0;
 	uint32_t tamanioPCB;
  	memcpy(&tamanioPCB, package + offset, tmp_size);
 	t_pcb *pcb = malloc(tamanioPCB);
  	offset += tmp_size;
 	memcpy(&(pcb->pcb_id), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->PC), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->posicionPrimerPaginaCodigo), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->codigo), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&tamIndCod, package + offset, tmp_size);
 	offset += tmp_size;
 	char* indCod = malloc(tamIndCod);
 	memcpy(&indCod, package + offset, tamIndCod * sizeof(uint32_t));
 	offset += tmp_size * sizeof(uint32_t);
 	pcb->ind_codigo = deserializarIndiceCodigo(indCod, tamIndCod);
 	memcpy(&tamIndEt, package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->ind_etiq), package + offset, tamIndEt);
 	offset += tamIndEt;
 	memcpy(&tamIndSt, package + offset, tmp_size);
 	offset += tmp_size;
 	char* indStack = malloc(tamIndSt);
 	memcpy(&indStack, package + offset, tamIndSt * sizeof(uint32_t));
 	offset += tamIndSt * sizeof(uint32_t);
 	pcb->ind_stack = deserializarIndiceStack(indStack, tamIndSt);
 	return pcb;
 }
 

char* serializar_EstructuraInicial(t_EstructuraInicial * estructuraInicial) {
	char* buffer = malloc(sizeof(t_EstructuraInicial)); //1B de op y 4B de long
	memcpy(buffer, &(estructuraInicial->Quantum), sizeof(uint32_t));
	memcpy(buffer + sizeof(uint32_t), &(estructuraInicial->RetardoQuantum), sizeof(uint32_t));

	return buffer;
}

t_EstructuraInicial* deserializar_EstructuraInicial(char* package) {
	t_EstructuraInicial * estructuraInicial = malloc(sizeof(t_EstructuraInicial));
	int offset = 0;
	int tmp_size = sizeof(uint32_t);

	memcpy(&estructuraInicial->Quantum, package + offset, tmp_size);
	offset += tmp_size;
	memcpy(&estructuraInicial->RetardoQuantum, package + offset, tmp_size);

	return estructuraInicial;
}

char* serializar_opVarCompartida(t_op_varCompartida* varCompartida) {
	int offset = 0, tmp_size = 0;
	char * paqueteSerializado = malloc(8 + (varCompartida->longNombre));

	tmp_size = sizeof(varCompartida->longNombre);
	memcpy(paqueteSerializado + offset, &(varCompartida->longNombre), tmp_size);
	offset += tmp_size;
	tmp_size = varCompartida->longNombre;
	memcpy(paqueteSerializado + offset, &(varCompartida->nombre), tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(varCompartida->valor);
	memcpy(paqueteSerializado + offset, &(varCompartida->valor), tmp_size);

	return paqueteSerializado;
}

t_op_varCompartida* deserializar_opVarCompartida(char* package) {
	t_op_varCompartida* varCompartida = malloc(sizeof(t_op_varCompartida));
	int offset = 0;
	int tmp_size = sizeof(uint32_t);

	memcpy(&(varCompartida->longNombre), package + offset, tmp_size);
	offset += tmp_size;
	tmp_size = varCompartida->longNombre;
	varCompartida->nombre = malloc(tmp_size);
	memcpy(&(varCompartida->nombre), package + offset, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(uint32_t);
	memcpy(&(varCompartida->valor), package + offset, tmp_size);

	return varCompartida;
}

t_buffer_tamanio* serializadoIndiceDeCodigo(t_list* indiceCodigo) {
	t_indice_codigo* linea;
	int offset = 0, m = 0;
	int tmp_size = sizeof(uint32_t);
	char* buffer = malloc(list_size(indiceCodigo) * (2 * sizeof(uint32_t)));

	for (m = 0; m < list_size(indiceCodigo); m++) {
		printf("buffer: %s\n", buffer);
		linea = list_get(indiceCodigo, m);

		printf("Indice: %d -- Offset: %d - Tamanio: %d\n", m, linea->offset,
				linea->start);

		memcpy(&linea->start, buffer + offset, tmp_size);
		offset += tmp_size;
		memcpy(&linea->offset, buffer + offset, tmp_size);
		offset += tmp_size;
	}

	offset -= tmp_size;

	t_buffer_tamanio * tamanioBuffer = malloc(sizeof(uint32_t) + offset);
	tamanioBuffer->buffer = buffer;
	tamanioBuffer->tamanioBuffer = offset;

	return tamanioBuffer;
}

t_list* deserializarIndiceCodigo(char* indice, uint32_t tam) {
	t_list* ret;
	int offset = 0;
	ret = list_create();
	int tmp_size = sizeof(uint32_t);
	int i = 0;
	for (i = 0; i < tam; i++) {
		t_indice_codigo* linea = malloc(sizeof(t_indice_codigo));
		memcpy(&linea->start, indice + offset, tmp_size);
		offset += tmp_size;
		memcpy(&linea->offset, indice + offset, tmp_size);
		offset += tmp_size;
		list_add(ret, linea);
	}
	return ret;
}

t_buffer_tamanio* serializarIndiceStack(t_list* indiceStack) {
	int a;
	//Lo inicializo con este valor porque en la primer posicion del buffer
	//va a estar el tamanio total del stack, y en la segunda la cantidad de items del stack
	uint32_t tamanioTotalBuffer = sizeof(uint32_t);

	t_list * tamanioStackStack = list_create();
	uint32_t tamanioStackParticular;
	for (a = 0; a < list_size(indiceStack); a++) {
		t_stack* linea = list_get(indiceStack, a);
		tamanioStackParticular = 0;

		tamanioTotalBuffer += sizeof(uint32_t); //tamanio de variable posicion
		tamanioStackParticular += sizeof(uint32_t);

		int cantidadArgumentos = list_size(linea->argumentos);
		tamanioTotalBuffer += cantidadArgumentos * sizeof(t_argumento); //tamanio de lista de argumentos
		tamanioStackParticular += cantidadArgumentos * sizeof(t_argumento);

		int cantidadVariables = list_size(linea->variables);
		tamanioTotalBuffer += cantidadVariables * sizeof(t_argumento); //tamanio de lista de variables
		tamanioStackParticular += cantidadVariables * sizeof(t_argumento);

		tamanioTotalBuffer += sizeof(uint32_t); //tamanio de variable direcretorno
		tamanioStackParticular += sizeof(uint32_t);

		tamanioTotalBuffer += sizeof(t_argumento); //tamanio de retVar
		tamanioStackParticular += sizeof(t_argumento);

		tamanioTotalBuffer += sizeof(uint32_t) * 3; //agrego 3 ints para indicar la cantidad de elemento de las 2 listas y los bytes de t_stack

		t_tamanio_stack_stack * auxiliar = malloc(sizeof(uint32_t) + tamanioStackParticular);
		list_add(tamanioStackStack, auxiliar);
	}

	char * buffer = malloc(tamanioTotalBuffer);
	int tamanioUint32 = sizeof(uint32_t), offset = 0;

	uint32_t cantidadItemsEnStack = list_size(indiceStack);

	memcpy(buffer + offset, &cantidadItemsEnStack, tamanioUint32);
	offset += tamanioUint32;

	for(a = 0; a < list_size(tamanioStackStack); a++) {
		t_tamanio_stack_stack * linea = list_get(tamanioStackStack, a);

		memcpy(buffer + offset, &(linea->tamanioStack), tamanioUint32);
		offset += tamanioUint32;

		t_stack * stack = linea->stack;

		memcpy(buffer + offset, &(stack->posicion), tamanioUint32);
		offset += tamanioUint32;

		uint32_t cantidadArgumentos = list_size(stack->argumentos);
		memcpy(buffer + offset, &cantidadArgumentos, tamanioUint32);
		offset += tamanioUint32;

		int p;
		for(p = 0; p < cantidadArgumentos; p++) {
			t_argumento *argumento = list_get(stack->argumentos, p);
			memcpy(buffer + offset, &(argumento->pagina), tamanioUint32);
			offset =+ tamanioUint32;
			memcpy(buffer + offset, &(argumento->offset), tamanioUint32);
			offset =+ tamanioUint32;
			memcpy(buffer + offset, &(argumento->size), tamanioUint32);
			offset =+ tamanioUint32;
		}

		uint32_t cantidadVariables = list_size(stack->variables);
		memcpy(buffer + offset, &cantidadVariables, tamanioUint32);
		offset += tamanioUint32;

		for(p = 0; p < cantidadVariables; p++) {
			t_argumento *argumento = list_get(stack->variables, p);
			memcpy(buffer + offset, &(argumento->pagina), tamanioUint32);
			offset =+ tamanioUint32;
			memcpy(buffer + offset, &(argumento->offset), tamanioUint32);
			offset =+ tamanioUint32;
			memcpy(buffer + offset, &(argumento->size), tamanioUint32);
			offset =+ tamanioUint32;
		}

		memcpy(buffer + offset, &(stack->direcretorno), tamanioUint32);
		offset += tamanioUint32;

		t_argumento * retVarStack = stack->retVar;
		memcpy(buffer + offset, &(retVarStack->pagina), tamanioUint32);
		offset += tamanioUint32;
		memcpy(buffer + offset, &(retVarStack->offset), tamanioUint32);
		offset += tamanioUint32;
		memcpy(buffer + offset, &(retVarStack->size), tamanioUint32);
		offset += tamanioUint32;
	}

	t_buffer_tamanio *bufferTamanio = malloc(sizeof(uint32_t) + tamanioTotalBuffer);
	bufferTamanio->tamanioBuffer = tamanioTotalBuffer;
	bufferTamanio->buffer = buffer;

	return bufferTamanio;
}

t_list* deserializarIndiceStack(char* buffer, uint32_t tamanioTotal) {
	t_list * stack = list_create();
	int offset = 0, tamanioUint32 = sizeof(uint32_t);

	uint32_t cantidadElementosEnStack;
	memcpy(&cantidadElementosEnStack, buffer + offset, tamanioUint32);
	offset += tamanioUint32;

	int i;
	for(i = 0; i < cantidadElementosEnStack; i++) {
		uint32_t tamanioItemStack;
		memcpy(&tamanioItemStack, buffer + offset, tamanioUint32);
		offset += tamanioUint32;

		t_stack * stack_item = malloc(tamanioUint32);

		uint32_t posicion;
		memcpy(&posicion, buffer + offset, tamanioUint32);
		offset += tamanioUint32;
		stack_item->posicion = posicion;

		t_list * argumentosStack = list_create();
		uint32_t cantidadArgumentosStack;
		memcpy(&cantidadArgumentosStack, buffer + offset, tamanioUint32);
		offset =+ tamanioUint32;
		int p;
		for(p = 0; p < cantidadArgumentosStack; p++) {
			t_argumento * argStack = malloc(sizeof(t_argumento));
			memcpy(&(argStack->pagina), buffer + offset, tamanioUint32);
			offset += tamanioUint32;
			memcpy(&(argStack->offset), buffer + offset, tamanioUint32);
			offset += tamanioUint32;
			memcpy(&(argStack->size), buffer + offset, tamanioUint32);
			offset += tamanioUint32;
			list_add(argumentosStack, argStack);
		}
		stack_item->argumentos = argumentosStack;

		t_list * variablesStack = list_create();
		uint32_t cantidadVariablesStack;
		memcpy(&cantidadVariablesStack, buffer + offset, tamanioUint32);
		offset =+ tamanioUint32;
		for(p = 0; p < cantidadVariablesStack; p++) {
			t_argumento * varStack = malloc(sizeof(t_argumento));
			memcpy(&(varStack->pagina), buffer + offset, tamanioUint32);
			offset += tamanioUint32;
			memcpy(&(varStack->offset), buffer + offset, tamanioUint32);
			offset += tamanioUint32;
			memcpy(&(varStack->size), buffer + offset, tamanioUint32);
			offset += tamanioUint32;
			list_add(variablesStack, varStack);
		}
		stack_item->variables = variablesStack;

		uint32_t direcRetorno;
		memcpy(&direcRetorno, buffer + offset, tamanioUint32);
		offset += tamanioUint32;
		stack_item->direcretorno = direcRetorno;

		t_argumento * retVarStack = malloc(sizeof(t_argumento));
		memcpy(&(retVarStack->pagina), buffer + offset, tamanioUint32);
		offset += tamanioUint32;
		memcpy(&(retVarStack->offset), buffer + offset, tamanioUint32);
		offset += tamanioUint32;
		memcpy(&(retVarStack->size), buffer + offset, tamanioUint32);
		offset += tamanioUint32;
		stack_item->retVar = retVarStack;

		list_add(stack, stack_item);
	}

	return stack;
}

t_list* llenarLista(t_list * lista, t_intructions * indiceCodigo,
		t_size cantInstruc) {
	int b = 0;
	for (b = 0; b < cantInstruc; ++b) {
		t_indice_codigo* linea = malloc(sizeof(t_indice_codigo));
		linea->start = indiceCodigo[b].start;
		linea->offset = indiceCodigo[b].offset;
		list_add(lista, linea);
	}
	return lista;
}

t_list* llenarStack(t_list * lista, t_stack* lineaAgregar) {
	if (lineaAgregar->posicion >= 0) {
		list_add(lista, lineaAgregar);
	}
	return lista;
}
