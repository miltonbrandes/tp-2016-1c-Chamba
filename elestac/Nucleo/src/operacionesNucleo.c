/*
 * operacionesNucleo.c
 *
 *  Created on: 10/5/2016
 *      Author: Milton Brandes
 */

#include "estructurasNucleo.h"
#include "operacionesNucleo.h"
#include <parser/metadata_program.h>
int tamanioTotal;
char* serializarSolicitarBytes(t_solicitarBytes* solicitarBytes, uint32_t *operacion) {
	int offset = 0, tmp_size = 0;
	size_t packageSize = sizeof(t_solicitarBytes);
	char * paqueteSerializado = malloc(packageSize + sizeof(uint32_t));

	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, operacion, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(solicitarBytes->pagina);
	memcpy(paqueteSerializado + offset, &(solicitarBytes->pagina), tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(solicitarBytes->offset);
	memcpy(paqueteSerializado + offset, &(solicitarBytes->offset), tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(solicitarBytes->tamanio);
	memcpy(paqueteSerializado + offset, &(solicitarBytes->tamanio), tmp_size);

	return paqueteSerializado;
}

char* serializarEnviarBytes(t_enviarBytes* enviarBytes, uint32_t *operacion) {
	int offset = 0, tmp_size = 0;
	size_t packageSize = sizeof(t_enviarBytes) + enviarBytes->tamanio;
	char * paqueteSerializado = malloc(packageSize + sizeof(uint32_t));

	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, operacion, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(enviarBytes->pagina);
	memcpy(paqueteSerializado + offset, &(enviarBytes->pagina), tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(enviarBytes->offset);
	memcpy(paqueteSerializado + offset, &(enviarBytes->offset), tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(enviarBytes->tamanio);
	memcpy(paqueteSerializado + offset, &(enviarBytes->tamanio), tmp_size);
	offset += tmp_size;
	tmp_size = enviarBytes->tamanio;
	memcpy(paqueteSerializado + offset, (enviarBytes->buffer), tmp_size);
	offset++;

	return paqueteSerializado;

}

char* serializarCambioProcActivo(t_cambio_proc_activo* cambioProcActivo, uint32_t *operacion) {
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

char* serializarCrearSegmento(t_iniciar_programa* crearSegmento, uint32_t *operacion) {
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

char* serializarDestruirSegmento(t_finalizar_programa* destruirSegmento,	uint32_t *operacion) {
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
t_pcb* deserializar_pcb(char** package) {
	t_pcb *pcb = malloc(sizeof(t_pcb));
	int offset = 0;
	int tmp_size = sizeof(uint32_t);

	memcpy(&pcb->pcb_id, *package + offset, tmp_size);
	offset += tmp_size;
	memcpy(&pcb->codigo, *package + offset, tmp_size);
	offset += tmp_size;
	memcpy(&pcb->ind_codigo, *package + offset, tmp_size);
	offset += tmp_size;
	memcpy(&pcb->ind_etiq, *package + offset, tmp_size);
	offset += tmp_size;
	memcpy(&pcb->PC, *package + offset, tmp_size);
	offset += tmp_size;
	memcpy(&pcb->ind_stack, *package + offset, tmp_size);
	//me falta desserializar el stack, como carajo lo hago?
	return pcb;
}

char* serializar_pcb(t_pcb* pcb) {
	int offset = 0, tmp_size = 0;
	char* paqueteSerializado = NULL;
	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(pcb->pcb_id), tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(pcb->codigo), tmp_size);
	offset += tmp_size;
	//agrego tamaño de indice de codigo
	tmp_size=list_size(pcb->ind_codigo);
	memcpy(paqueteSerializado+offset, &tmp_size, sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	char* indcod= malloc(list_size(pcb->ind_codigo)*8);
	indcod =serializadoIndiceDeCodigo(pcb->ind_codigo);
	tmp_size = 2*sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(indcod), tmp_size*list_size(pcb->ind_codigo));
	offset += tmp_size;
	tmp_size = sizeof(pcb->ind_etiq);
	//mando tamanio de etiquetas para poder despues desserializarlo
	memcpy(paqueteSerializado+ offset, &(tmp_size), sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(pcb->ind_etiq), tmp_size);
	offset += tmp_size;
	char* stackSerializado = serializarIndiceStack(pcb->ind_stack);
	tmp_size = tamanioTotal;
	memcpy(paqueteSerializado + offset, &(stackSerializado), tamanioTotal);
	offset += tmp_size;
	return paqueteSerializado;
}

char* serializar_EstructuraInicial(t_EstructuraInicial * estructuraInicial) {
	int offset = 0, tmp_size = 0;
	char * paqueteSerializado = malloc(sizeof(t_EstructuraInicial));

	tmp_size = sizeof(estructuraInicial->Quantum);
	memcpy(paqueteSerializado + offset, &(estructuraInicial->Quantum),
			tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(estructuraInicial->RetardoQuantum);
	memcpy(paqueteSerializado + offset, &(estructuraInicial->RetardoQuantum),
			tmp_size);

	return paqueteSerializado;
}

t_EstructuraInicial* deserializar_EstructuraInicial(char** package) {
	t_EstructuraInicial * estructuraInicial = malloc(
			sizeof(t_EstructuraInicial));
	int offset = 0;
	int tmp_size = sizeof(uint32_t);

	memcpy(&estructuraInicial->Quantum, *package + offset, tmp_size);
	offset += tmp_size;
	memcpy(&estructuraInicial->RetardoQuantum, *package + offset, tmp_size);

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

t_op_varCompartida* deserializar_opVarCompartida(char** package) {
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

char* serializadoIndiceDeCodigo(t_list* indiceCodigo){

	int offset=0,m=0;
	int tmp_size=sizeof(uint32_t);
	char* buffer=malloc(list_size(indiceCodigo)*8);

	for(m=0;m<list_size(indiceCodigo);++m){
		t_indice_codigo* linea;
		linea = list_get(indiceCodigo, m);

		memcpy(buffer+offset,&(linea->tamanio),tmp_size);
		offset+=tmp_size;
		memcpy(buffer+offset,&(linea->offset),tmp_size);
		offset+=tmp_size;
	}
	return buffer;
}
/*t_list* deserializarIndiceCodigo(char* indice)
{
	t_list* ret;
	int offset = 0;
	ret = list_create();
	int tmp_size=sizeof(uint32_t);
	memcpy(&ret, *indice + offset, tmp_size);
		offset += tmp_size;
		memcpy(&estructuraInicial->RetardoQuantum, *package + offset, tmp_size);

	return ret;
}*/
char* serializarIndiceStack(t_list* indiceStack)
{
	int a;
	int tmp_size= sizeof(uint32_t);
	int tamanioStack = list_size(indiceStack);
	tamanioTotal=0;
	for(a=0; a<list_size(indiceStack); a++)
	{

		t_stack* linea = list_get(indiceStack, a);
		if(linea->posicion != -1)
		{
			tamanioTotal+= sizeof(uint32_t);
		}
		tamanioTotal+= list_size(linea->argumentos)*sizeof(uint32_t)*3;
		tamanioTotal+= list_size(linea->variables)*sizeof(uint32_t)*4;
		if(linea->direcretorno != -1)
		{
			tamanioTotal+= sizeof(uint32_t);
		}
		if(linea->retVar.pagina != -1)
		{
			tamanioTotal+= sizeof(uint32_t)*3;
		}
		tamanioTotal+= sizeof(uint32_t)*2;
	}
	int offset = 0;
	char* buffer= malloc(tamanioTotal);
	//agrego adelante el size del stack
	memcpy(buffer+offset, &(tamanioTotal), tmp_size);
	for(a=0; a<list_size(indiceStack); a++)
		{
			t_stack* linea = list_get(indiceStack, a);
			memcpy(buffer+offset,&(linea->posicion),tmp_size);
			offset+=tmp_size;
			//esta es el size de la lista de argumentos para poder despues deserializarlo
			uint32_t cantArgumentos = list_size(linea->argumentos);
			memcpy(buffer+offset,&cantArgumentos, tmp_size);
			offset+= tmp_size;
			int j=0;
			for(j = 0; j<list_size(linea->argumentos);j++)
			{
				t_argumento* lineaArgumento = list_get(linea->argumentos, j);
				memcpy(buffer+offset, &(lineaArgumento->pagina), tmp_size);
				offset+=tmp_size;
				memcpy(buffer+offset, &(lineaArgumento->offset), tmp_size);
				offset+=tmp_size;
				memcpy(buffer+offset, &(lineaArgumento->size), tmp_size);
				offset+=tmp_size;
			}
			//esta es el size de la lista de variables para poder despues deserializarlo
			uint32_t cantVariables = list_size(linea->variables);
			memcpy(buffer+offset, &cantVariables, tmp_size);
			offset+= tmp_size;
			int h=0;
			for(h=0; h<list_size(linea->variables); h++)
			{
				t_variable* lineaVariable = list_get(linea->variables,h);
				memcpy(buffer+offset, &(lineaVariable->idVariable), tmp_size);
				offset+=tmp_size;
				memcpy(buffer+offset, &(lineaVariable->pagina), tmp_size);
				offset+=tmp_size;
				memcpy(buffer+offset, &(lineaVariable->offset), tmp_size);
				offset+=tmp_size;
				memcpy(buffer+offset, &(lineaVariable->size), tmp_size);
				offset+=tmp_size;
			}
			memcpy(buffer+offset, &(linea->direcretorno), tmp_size);
			offset+=tmp_size;
			memcpy(buffer+offset, &(linea->retVar.pagina), tmp_size);
			offset+= tmp_size;
			memcpy(buffer+offset, &(linea->retVar.offset), tmp_size);
			offset+=tmp_size;
			memcpy(buffer+offset, &(linea->retVar.size), tmp_size);
			offset+=tmp_size;
		}
	return buffer;
}

t_list* llenarLista(t_list * lista, t_intructions * indiceCodigo, t_size cantInstruc)
{
	int b = 0;
	for(b=0;b<cantInstruc;++b){
		t_indice_codigo* linea;
		linea->tamanio = indiceCodigo[b].start;
		linea->offset = indiceCodigo[b].offset;
		list_add(lista, linea);
	}
	return lista;
}

t_list* llenarStack(t_list * lista, t_stack* lineaAgregar)
{
	if(lineaAgregar->posicion >=0)
	{
		list_add(lista, lineaAgregar);
	}
	return lista;
}



