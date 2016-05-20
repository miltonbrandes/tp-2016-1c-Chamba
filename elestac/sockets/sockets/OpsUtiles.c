/*
 * OpsUtiles.c
 *
 *  Created on: 17/5/2016
 *      Author: utnso
 */

#include <commons/collections/list.h>
#include <parser/metadata_program.h>
#include "OpsUtiles.h"
#include "StructsUtiles.h"

uint32_t tamanioTotal;

void* serializarUint32(uint32_t number) {
	int offset = 0, tmp_size = 0;
	size_t messageSize = sizeof(uint32_t);

	void *serialized = malloc(messageSize);

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

	memccpy((&respuesta->pagina), message + offset, tmp_size);
	offset += tmp_size;
	memccpy((&respuesta->offset), message + offset, tmp_size);
	offset += tmp_size;
	memccpy((&respuesta->start), message + offset, tmp_size);

	return respuesta;
}

char* serializarEnviarBytes(t_enviarBytes* enviarBytes, uint32_t *operacion) {
	int offset = 0, tmp_size = 0;
	size_t packageSize = sizeof(t_enviarBytes) + enviarBytes->tamanio;
	char * paqueteSerializado = malloc(packageSize + sizeof(uint32_t));

	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, operacion, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(enviarBytes->pagina), tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(enviarBytes->offset), tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(enviarBytes->tamanio), tmp_size);
	offset += tmp_size;
	tmp_size = enviarBytes->tamanio;
	memcpy(paqueteSerializado + offset, (enviarBytes->buffer), tmp_size);
	offset++;

	return paqueteSerializado;
}

t_enviarBytes* deserializarEnviarBytes(char* message) {
	uint32_t pagina, offset, tamanio;
	int offsetMemcpy = 0, tmp_size = sizeof(uint32_t);

	memcpy(&pagina, message + offsetMemcpy, tmp_size);
	offsetMemcpy += tmp_size;
	memcpy(&offset, message + offsetMemcpy, tmp_size);
	offsetMemcpy += tmp_size;
	memcpy(&tamanio, message + offsetMemcpy, tmp_size);
	offsetMemcpy += tmp_size;
	tmp_size = tamanio;
	char *datos = malloc(tamanio);
	memcpy(datos, message + offsetMemcpy, tmp_size);

	t_enviarBytes * respuesta = malloc((sizeof(uint32_t) * 3) + tamanio);
	respuesta->buffer = datos;
	respuesta->offset = offset;
	respuesta->pagina = pagina;
	respuesta->tamanio = tamanio;

	return respuesta;
}

char* serializarCambioProcActivo(t_cambio_proc_activo* cambioProcActivo,
		uint32_t *operacion) {
	int offset = 0, tmp_size = 0;
	size_t packageSize = sizeof(cambioProcActivo->programID)
			+ sizeof(cambioProcActivo->programID);
	char * paqueteSerializado = malloc(packageSize + sizeof(uint32_t));

	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, operacion, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(cambioProcActivo->programID);
	memcpy(paqueteSerializado + offset, &(cambioProcActivo->programID),
			tmp_size);

	return paqueteSerializado;
}

char* serializarCrearSegmento(t_iniciar_programa* crearSegmento,
		uint32_t *operacion) {
	int offset = 0, tmp_size = 0;
	size_t packageSize = sizeof(crearSegmento->programID)
			+ sizeof(crearSegmento->tamanio);
	char * paqueteSerializado = malloc(packageSize + sizeof(uint32_t));

	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, operacion, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(crearSegmento->programID);
	memcpy(paqueteSerializado + offset, &(crearSegmento->programID), tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(crearSegmento->tamanio);
	memcpy(paqueteSerializado + offset, &(crearSegmento->tamanio), tmp_size);

	return paqueteSerializado;
}

char* serializarDestruirSegmento(t_finalizar_programa* destruirSegmento,
		uint32_t *operacion) {
	int offset = 0, tmp_size = 0;
	size_t packageSize = sizeof(destruirSegmento->programID);
	char * paqueteSerializado = malloc(packageSize + sizeof(uint32_t));

	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, operacion, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(destruirSegmento->programID);
	memcpy(paqueteSerializado + offset, &(destruirSegmento->programID),
			tmp_size);

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
	char* stackSerializado = serializarIndiceStack(pcb->ind_stack);
	char* indcod = serializadoIndiceDeCodigo(pcb->ind_codigo);
	uint32_t tamanioPCB = 0;
	tamanioPCB += strlen(stackSerializado);
	tamanioPCB += strlen(indcod);
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
	memcpy(paqueteSerializado + offset, &(indcod),
			tmp_size * list_size(pcb->ind_codigo));
	offset += tmp_size * list_size(pcb->ind_codigo);
	tmp_size = sizeof(pcb->ind_etiq);
//mando tamanio de etiquetas para poder despues desserializarlo
	memcpy(paqueteSerializado + offset, &(tmp_size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(pcb->ind_etiq), tmp_size);
	offset += tmp_size;
	tmp_size = tamanioTotal;
	memcpy(paqueteSerializado + offset, &(stackSerializado), tamanioTotal);
	offset += tmp_size;
	return paqueteSerializado;
}

char* serializar_EstructuraInicial(t_EstructuraInicial * estructuraInicial) {
	char* buffer = malloc(sizeof(t_EstructuraInicial)); //1B de op y 4B de long
	memcpy(buffer, &(estructuraInicial->Quantum), sizeof(uint32_t));
	memcpy(buffer + sizeof(uint32_t), &(estructuraInicial->RetardoQuantum),
			sizeof(uint32_t));

	return buffer;
}

t_EstructuraInicial* deserializar_EstructuraInicial(char* package) {
	t_EstructuraInicial * estructuraInicial = malloc(
			sizeof(t_EstructuraInicial));
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
	memcpy(paqueteSerializado + offset, varCompartida->nombre, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(varCompartida->valor);
	memcpy(paqueteSerializado + offset, &(varCompartida->valor), tmp_size);

	return paqueteSerializado;
}

t_op_varCompartida* deserializar_opVarCompartida(char* package) {
	t_op_varCompartida* varCompartida = malloc(sizeof(t_op_varCompartida));
	int offset = 0;
	int tmp_size = sizeof(uint32_t);

	memcpy(&varCompartida->longNombre, *package + offset, tmp_size);
	offset += tmp_size;
	tmp_size = varCompartida->longNombre;
	varCompartida->nombre = malloc(tmp_size);
	memcpy(varCompartida->nombre, *package + offset, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(uint32_t);
	memcpy(&varCompartida->valor, *package + offset, tmp_size);

	return varCompartida;
}

char* serializadoIndiceDeCodigo(t_list* indiceCodigo) {

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
	return buffer;
}

t_list* deserializarIndiceCodigo(char* indice, uint32_t tam) {
	t_list* ret;
	int offset = 0;
	ret = list_create();
	int tmp_size = sizeof(uint32_t);
	int i = 0;
	for (i = 0; i < tam; i++) {
		t_indice_codigo* linea;
		memcpy(&linea->start, indice + offset, tmp_size);
		offset += tmp_size;
		memcpy(&linea->offset, indice + offset, tmp_size);
		offset += tmp_size;
		list_add(ret, linea);
	}
	return ret;
}

char* serializarIndiceStack(t_list* indiceStack) {
	int a;
	int tmp_size = sizeof(uint32_t);
	tamanioTotal = 0;
	for (a = 0; a < list_size(indiceStack); a++) {

		t_stack* linea = list_get(indiceStack, a);
		if (linea->posicion != -1) {
			tamanioTotal += sizeof(uint32_t);
			printf("Tamanio Stack: %d\n", tamanioTotal);
		}
		tamanioTotal += list_size(linea->argumentos) * sizeof(uint32_t) * 3;
		printf("Tamanio Stack: %d\n", tamanioTotal);
		tamanioTotal += list_size(linea->variables) * sizeof(uint32_t) * 4;
		printf("Tamanio Stack: %d\n", tamanioTotal);
		if (linea->direcretorno != -1) {
			tamanioTotal += sizeof(uint32_t);
			printf("Tamanio Stack: %d\n", tamanioTotal);
		}
		if (linea->retVar.pagina != -1) {
			tamanioTotal += sizeof(uint32_t) * 3;
			printf("Tamanio Stack: %d\n", tamanioTotal);
		}
		tamanioTotal += sizeof(uint32_t) * 2;
		printf("Tamanio Stack: %d\n", tamanioTotal);
	}

	int offset = 0;
	char* buffer = malloc(tamanioTotal);
	//agrego adelante el size del stack
	memcpy(buffer + offset, &(tamanioTotal), tmp_size);
	for (a = 0; a < list_size(indiceStack); a++) {
		t_stack* linea = list_get(indiceStack, a);
		memcpy(buffer + offset, &(linea->posicion), tmp_size);
		offset += tmp_size;
		//esta es el size de la lista de argumentos para poder despues deserializarlo
		uint32_t cantArgumentos = list_size(linea->argumentos);
		memcpy(buffer + offset, &cantArgumentos, tmp_size);
		offset += tmp_size;
		int j = 0;
		for (j = 0; j < list_size(linea->argumentos); j++) {
			t_argumento* lineaArgumento = list_get(linea->argumentos, j);
			memcpy(buffer + offset, &(lineaArgumento->pagina), tmp_size);
			offset += tmp_size;
			memcpy(buffer + offset, &(lineaArgumento->offset), tmp_size);
			offset += tmp_size;
			memcpy(buffer + offset, &(lineaArgumento->size), tmp_size);
			offset += tmp_size;
		}
		//esta es el size de la lista de variables para poder despues deserializarlo
		uint32_t cantVariables = list_size(linea->variables);
		memcpy(buffer + offset, &cantVariables, tmp_size);
		offset += tmp_size;
		int h = 0;
		for (h = 0; h < list_size(linea->variables); h++) {
			t_variable* lineaVariable = list_get(linea->variables, h);
			memcpy(buffer + offset, &(lineaVariable->idVariable), tmp_size);
			offset += tmp_size;
			memcpy(buffer + offset, &(lineaVariable->pagina), tmp_size);
			offset += tmp_size;
			memcpy(buffer + offset, &(lineaVariable->offset), tmp_size);
			offset += tmp_size;
			memcpy(buffer + offset, &(lineaVariable->size), tmp_size);
			offset += tmp_size;
		}
		memcpy(buffer + offset, &(linea->direcretorno), tmp_size);
		offset += tmp_size;
		memcpy(buffer + offset, &(linea->retVar.pagina), tmp_size);
		offset += tmp_size;
		memcpy(buffer + offset, &(linea->retVar.offset), tmp_size);
		offset += tmp_size;
		memcpy(buffer + offset, &(linea->retVar.size), tmp_size);
		offset += tmp_size;
	}
	return buffer;
}

t_list* deserializarIndiceStack(char* buffer, uint32_t tamanioTotal) {
	t_list* ret;
	ret = list_create();
	return ret;
	int a;
	int tmp_size = sizeof(uint32_t);
	int offset = 0;
	//agrego adelante el size del stack

	for (a = 0; a < tamanioTotal; a++) {
		t_stack* linea;
		memcpy(&(linea->posicion), buffer + offset, tmp_size);
		offset += tmp_size;
		//esta es el size de la lista de argumentos para poder despues deserializarlo
		uint32_t cantArgumentos;
		memcpy(&cantArgumentos, buffer + offset, tmp_size);
		offset += tmp_size;
		int j = 0;
		for (j = 0; j < cantArgumentos; j++) {
			t_argumento* lineaArgumento;
			memcpy(&(lineaArgumento->pagina), buffer + offset, tmp_size);
			offset += tmp_size;
			memcpy(&(lineaArgumento->offset), buffer + offset, tmp_size);
			offset += tmp_size;
			memcpy(&(lineaArgumento->size), buffer + offset, tmp_size);
			offset += tmp_size;
		}
		//esta es el size de la lista de variables para poder despues deserializarlo
		uint32_t cantVariables;
		memcpy(&cantVariables, buffer + offset, tmp_size);
		offset += tmp_size;
		int h = 0;
		for (h = 0; h < cantVariables; h++) {
			t_variable* lineaVariable;
			memcpy(&(lineaVariable->idVariable), buffer + offset, tmp_size);
			offset += tmp_size;
			memcpy(&(lineaVariable->pagina), buffer + offset, tmp_size);
			offset += tmp_size;
			memcpy(&(lineaVariable->offset), buffer + offset, tmp_size);
			offset += tmp_size;
			memcpy(&(lineaVariable->size), buffer + offset, tmp_size);
			offset += tmp_size;
		}
		memcpy(&(linea->direcretorno), buffer + offset, tmp_size);
		offset += tmp_size;
		memcpy(&(linea->retVar.pagina), buffer + offset, tmp_size);
		offset += tmp_size;
		memcpy(&(linea->retVar.offset), buffer + offset, tmp_size);
		offset += tmp_size;
		memcpy(&(linea->retVar.size), buffer + offset, tmp_size);
		offset += tmp_size;
	}
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
