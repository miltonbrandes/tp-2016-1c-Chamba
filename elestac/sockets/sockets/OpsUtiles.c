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

t_buffer_tamanio * serializarInstruccion(t_instruccion * instruccion, int tamanioInstruccion) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	char * buffer = malloc(tamanioInstruccion + 4);

	memcpy(buffer, &tamanioInstruccion, tmp_size);
	offset += tmp_size;
	memcpy(buffer + offset, (instruccion->instruccion), tamanioInstruccion);
	t_buffer_tamanio * buffer_tamanio;
	buffer_tamanio = malloc(tamanioInstruccion + 8);
	buffer_tamanio->tamanioBuffer = tmp_size + tamanioInstruccion;
	buffer_tamanio->buffer = buffer;

	return buffer_tamanio;
}

t_instruccion * deserializarInstruccion(char * message) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	uint32_t longitud;

	memcpy(&longitud, message, tmp_size);
	offset += tmp_size;

	char * instruccion = malloc(longitud);
	memcpy(instruccion, message + offset, longitud);

	t_pagina_de_swap * paginaSwap = malloc(longitud);
	paginaSwap->paginaSolicitada = malloc(longitud);
	memcpy(paginaSwap->paginaSolicitada, instruccion, longitud);

	return paginaSwap;
}

t_buffer_tamanio * serializarCheckEspacio(t_check_espacio * check) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	char * buffer = malloc(sizeof(t_check_espacio));

	memcpy(buffer, &(check->pid), tmp_size);
	offset += tmp_size;
	memcpy(buffer + offset, &(check->cantidadDePaginas), tmp_size);

	t_buffer_tamanio * buffer_tamanio = malloc(sizeof(uint32_t) + sizeof(t_check_espacio));
	buffer_tamanio->tamanioBuffer = sizeof(t_check_espacio);
	buffer_tamanio->buffer = buffer;

	return buffer_tamanio;
}

t_check_espacio * deserializarCheckEspacio(char * buffer) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	t_check_espacio * solicitudPagina = malloc(sizeof(t_check_espacio));

	memcpy(&(solicitudPagina->pid), buffer, tmp_size);
	offset += tmp_size;
	memcpy(&(solicitudPagina->cantidadDePaginas), buffer + offset, tmp_size);

	return solicitudPagina;
}

t_buffer_tamanio * serializarEscribirEnSwap(t_escribir_en_swap * escribirEnSwap, uint32_t marcoSize) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	uint32_t longPag = marcoSize;
	char * buffer = malloc(longPag + (sizeof(uint32_t) * 3));

	memcpy(buffer, &(escribirEnSwap->pid), tmp_size);
	offset += tmp_size;
	memcpy(buffer + offset, &(escribirEnSwap->paginaProceso), tmp_size);
	offset += tmp_size;
	memcpy(buffer + offset, &longPag, tmp_size);
	offset += tmp_size;
	memcpy(buffer + offset, escribirEnSwap->contenido, longPag);

	t_buffer_tamanio * buffer_tamanio = malloc((sizeof(uint32_t) * 4) + longPag);
	buffer_tamanio->tamanioBuffer = longPag + (sizeof(uint32_t) * 3);
	buffer_tamanio->buffer = buffer;

	return buffer_tamanio;
}

t_escribir_en_swap * deserializarEscribirEnSwap(char * buffer) {
	int offset = 0, tmp_size = sizeof(uint32_t);

	uint32_t pid;
	memcpy(&pid, buffer, tmp_size);
	offset += tmp_size;

	uint32_t paginaProceso;
	memcpy(&paginaProceso, buffer + offset, tmp_size);
	offset += tmp_size;

	uint32_t tamanioBuffer;
	memcpy(&tamanioBuffer, buffer + offset, tmp_size);
	offset += tmp_size;

	char * pagina = malloc(tamanioBuffer);
	memcpy(pagina, buffer + offset, tamanioBuffer);

	t_escribir_en_swap * escribirEnSwap = malloc((sizeof(uint32_t) * 2) + tamanioBuffer);
	escribirEnSwap->pid = pid;
	escribirEnSwap->paginaProceso = paginaProceso;
	escribirEnSwap->contenido = pagina;

	return escribirEnSwap;
}

t_buffer_tamanio * serializarPaginaDeSwap(t_pagina_de_swap * pagina, uint32_t tamanioPagina) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	char * buffer = malloc(sizeof(uint32_t) + tamanioPagina);

	memcpy(buffer, &tamanioPagina, tmp_size);
	offset += tmp_size;
	memcpy(buffer + offset, (pagina->paginaSolicitada), tamanioPagina);

	t_buffer_tamanio * buffer_tamanio = malloc((sizeof(uint32_t) * 2) + tamanioPagina);
	buffer_tamanio->tamanioBuffer = tamanioPagina + sizeof(uint32_t);
	buffer_tamanio->buffer = buffer;

	return buffer_tamanio;
}

t_pagina_de_swap * deserializarPaginaDeSwap(char * message) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	uint32_t longitudPag;

	memcpy(&longitudPag, message, tmp_size);
	offset += tmp_size;

	char * pagina = calloc(1, longitudPag);
	memcpy(pagina, message + offset, longitudPag);

	t_pagina_de_swap * paginaSwap = malloc(longitudPag);
	paginaSwap->paginaSolicitada = pagina;

	return paginaSwap;
}

t_buffer_tamanio * serializarSolicitudPagina(t_solicitud_pagina * solicitud) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	char * buffer = malloc(sizeof(t_solicitud_pagina));

	memcpy(buffer, &(solicitud->pid), tmp_size);
	offset += tmp_size;
	memcpy(buffer + offset, &(solicitud->paginaProceso), tmp_size);

	t_buffer_tamanio * buffer_tamanio = malloc(sizeof(uint32_t) + sizeof(t_solicitud_pagina));
	buffer_tamanio->tamanioBuffer = sizeof(t_solicitud_pagina);
	buffer_tamanio->buffer = buffer;

	return buffer_tamanio;
}

t_solicitud_pagina * deserializarSolicitudPagina(char * buffer) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	t_solicitud_pagina * solicitudPagina = malloc(sizeof(t_solicitud_pagina));

	memcpy(&(solicitudPagina->pid), buffer, tmp_size);
	offset += tmp_size;
	memcpy(&(solicitudPagina->paginaProceso), buffer + offset, tmp_size);

	return solicitudPagina;
}

t_buffer_tamanio * serializarNuevoProgEnUMC(t_nuevo_prog_en_umc * nuevoProg) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	char * buffer = malloc(sizeof(t_nuevo_prog_en_umc));

	memcpy(buffer + offset, &(nuevoProg->primerPaginaDeProc), tmp_size);
	offset += tmp_size;
	memcpy(buffer + offset, &(nuevoProg->primerPaginaStack), tmp_size);

	t_buffer_tamanio * buffer_tamanio = malloc(sizeof(t_nuevo_prog_en_umc) + sizeof(uint32_t));
	buffer_tamanio->tamanioBuffer = sizeof(t_nuevo_prog_en_umc);
	buffer_tamanio->buffer = buffer;

	return buffer_tamanio;
}

t_nuevo_prog_en_umc * deserializarNuevoProgEnUMC(char * buffer) {
	t_nuevo_prog_en_umc * nuevoProg = malloc(sizeof(t_nuevo_prog_en_umc));
	int offset = 0, tmp_size = sizeof(uint32_t);

	memcpy(&(nuevoProg->primerPaginaDeProc), buffer + offset, tmp_size);
	offset += tmp_size;
	memcpy(&(nuevoProg->primerPaginaStack), buffer + offset, tmp_size);

	return nuevoProg;
}

t_buffer_tamanio * serializarIniciarPrograma(t_iniciar_programa * iniciarPrograma) {
	int tmp_size = sizeof(uint32_t), offset = 0;
	uint32_t tamanioCodigo = strlen(iniciarPrograma->codigoAnsisop) + 1;
	uint32_t tamanioBuffer = (sizeof(uint32_t) * 3) + tamanioCodigo;
	char * buffer = malloc(tamanioBuffer);

	memcpy(buffer+offset, &(tamanioCodigo), tmp_size);
	offset+= tmp_size;
	memcpy(buffer + offset, &(iniciarPrograma->programID), tmp_size);
	offset += tmp_size;
	memcpy(buffer + offset, &(iniciarPrograma->tamanio), tmp_size);
	offset+= tmp_size;
	memcpy(buffer+offset, iniciarPrograma->codigoAnsisop, tamanioCodigo);

	t_buffer_tamanio * buffer_tamanio = malloc(tamanioBuffer + sizeof(uint32_t));
	buffer_tamanio->tamanioBuffer = tamanioBuffer;
	buffer_tamanio->buffer = buffer;

	return buffer_tamanio;
}

t_iniciar_programa * deserializarIniciarPrograma(char * buffer) {
	int tmp_size = sizeof(uint32_t), offset = 0;
	uint32_t tamCod;
	memcpy(&(tamCod), buffer+offset, tmp_size);
	offset+= tmp_size;
	t_iniciar_programa * iniciarPrograma = malloc((sizeof(uint32_t) * 2) + tamCod);
	memcpy(&(iniciarPrograma->programID), buffer + offset, tmp_size);
	offset += tmp_size;
	memcpy(&(iniciarPrograma->tamanio), buffer + offset, tmp_size);
	offset+=tmp_size;
	iniciarPrograma->codigoAnsisop = malloc(tamCod);
	memcpy(iniciarPrograma->codigoAnsisop, buffer + offset, tamCod - 1);
	return iniciarPrograma;
}

char* enviarOperacion(uint32_t operacion, void* estructuraDeOperacion,int serverSocket) {
	char* respuestaOperacion;
	uint32_t id;
	t_iniciar_programa* est;
	t_buffer_tamanio *buffer_tamanio;

	switch (operacion) {
	case LEER:
		//esta parte iria en cpu, para pedirle a la umc la pagina que necesite...
		buffer_tamanio = serializarSolicitarBytes(estructuraDeOperacion);
		if ((enviarDatos(serverSocket, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, LEER, NUCLEO)) < 0) {
			free(buffer_tamanio->buffer);
			free(buffer_tamanio);
			return NULL;
		}
		//####		Recibo buffer pedidos		####
		respuestaOperacion = recibirDatos(serverSocket, NULL, &id);
		if (strcmp(respuestaOperacion, "ERROR") == 0) {
			free(buffer_tamanio->buffer);
			free(buffer_tamanio);
			free(respuestaOperacion);
			return NULL;
		}

		free(buffer_tamanio->buffer);
		free(buffer_tamanio);
		break;
	case LEER_VALOR_VARIABLE:
		//esta parte iria en cpu, para pedirle a la umc la pagina que necesite...
		buffer_tamanio = serializarSolicitarBytes(estructuraDeOperacion);
		if ((enviarDatos(serverSocket, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, LEER_VALOR_VARIABLE, NUCLEO)) < 0) {
			free(buffer_tamanio->buffer);
			free(buffer_tamanio);
			return NULL;
		}
		//####		Recibo buffer pedidos		####
		respuestaOperacion = recibirDatos(serverSocket, NULL, &id);
		if (strcmp(respuestaOperacion, "ERROR") == 0) {
			free(buffer_tamanio->buffer);
			free(buffer_tamanio);
			free(respuestaOperacion);
			return NULL;
		}

		free(buffer_tamanio->buffer);
		free(buffer_tamanio);
		break;
	case ESCRIBIR:
		//esta parte iria en cpu, para esciribir en la umc la pagina que necesite
		buffer_tamanio = serializarEnviarBytes((t_enviarBytes *) estructuraDeOperacion);

		if ((enviarDatos(serverSocket, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, ESCRIBIR, NUCLEO)) < 0) {
			free(buffer_tamanio->buffer);
			free(buffer_tamanio);
			return NULL;
		}

		free(buffer_tamanio->buffer);
		free(buffer_tamanio);

		respuestaOperacion = recibirDatos(serverSocket, NULL, &id);

		if(strcmp(respuestaOperacion, "ERROR") == 0) {
			free(respuestaOperacion);
			return NULL;
		}else{
			return respuestaOperacion;
		}

		break;

	case CAMBIOPROCESOACTIVO:
		buffer_tamanio = serializarCambioProcActivo(estructuraDeOperacion);

		//Envio paquete
		if ((enviarDatos(serverSocket, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, CAMBIOPROCESOACTIVO, NUCLEO)) < 0) {
			free(buffer_tamanio->buffer);
			free(buffer_tamanio);
			return NULL;
		}

		free(buffer_tamanio->buffer);
		free(buffer_tamanio);
		break;
	case NUEVOPROGRAMA:
		est = estructuraDeOperacion;
		uint32_t tamanioCodigo = (uint32_t)strlen(est->codigoAnsisop);
		buffer_tamanio = serializarIniciarPrograma((t_iniciar_programa *) estructuraDeOperacion);
		//Envio paquete
		if ((enviarDatos(serverSocket, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, NUEVOPROGRAMA, NUCLEO)) < 0) {
			free(buffer_tamanio->buffer);
			free(buffer_tamanio);
			return NULL;
		}
		//Recibo el valor respuesta de la operación
		respuestaOperacion = recibirDatos(serverSocket, NULL, &id);
		free(buffer_tamanio->buffer);
		free(buffer_tamanio);

		break;
	case FINALIZARPROGRAMA:
		buffer_tamanio = serializarFinalizarPrograma(estructuraDeOperacion);


		if ((enviarDatos(serverSocket, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, FINALIZARPROGRAMA, NUCLEO)) < 0) {
			free(buffer_tamanio->buffer);
			free(buffer_tamanio);
			return NULL;
		}

		free(buffer_tamanio->buffer);
		free(buffer_tamanio);
		return SUCCESS;

		break;
	default:
		printf("Operacion no admitida");
		break;
	}

	return respuestaOperacion;
}

t_buffer_tamanio * serializarUint32(uint32_t number) {
	int offset = 0, tmp_size = 0;
	size_t messageSize = sizeof(uint32_t);

	char *serialized = malloc(messageSize);

	tmp_size = sizeof(uint32_t);
	memcpy(serialized + offset, &number, tmp_size);

	t_buffer_tamanio * buffer_tamanio = malloc(sizeof(uint32_t) + messageSize);
	buffer_tamanio->tamanioBuffer = messageSize;
	buffer_tamanio->buffer = serialized;

	return buffer_tamanio;
}

t_buffer_tamanio * serializar (int number){
	int offset = 0, tmp_size = 0;
	size_t messageSize = sizeof(int);
	char *serialized = malloc(messageSize);

	tmp_size = sizeof(int);
	memcpy(serialized + offset, &number, tmp_size);

	t_buffer_tamanio * buffer_tamanio = malloc(sizeof(uint32_t) + messageSize);
	buffer_tamanio->tamanioBuffer = messageSize;
	buffer_tamanio->buffer = serialized;

	return buffer_tamanio;
}

int deserializar(char* buffer){
	int number;
	int offset = 0;
	int tmp_size = sizeof(int);
	memcpy(&number, buffer + offset, tmp_size);
	return number;

}

uint32_t deserializarUint32(char *package) {
	uint32_t number;
	int offset = 0;
	int tmp_size = sizeof(uint32_t);
	memcpy(&number, package + offset, tmp_size);
	return number;
}

t_buffer_tamanio * serializarSolicitarBytes(t_solicitarBytes* solicitarBytes) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	uint32_t packageSize = sizeof(t_solicitarBytes);
	char * paqueteSerializado = malloc(packageSize);

	memcpy(paqueteSerializado + offset, &(solicitarBytes->pagina), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(solicitarBytes->offset), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(solicitarBytes->start), tmp_size);

	t_buffer_tamanio * buffer_tamanio = malloc(sizeof(uint32_t) + packageSize);
	buffer_tamanio->tamanioBuffer = packageSize;
	buffer_tamanio->buffer = paqueteSerializado;

	return buffer_tamanio;
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

t_buffer_tamanio * serializarEnviarBytes(t_enviarBytes* enviarBytes) {
	int offset = 0, tmp_size = 0;
	uint32_t tamanioPaquete = (sizeof(uint32_t) * 4) + (strlen(enviarBytes->buffer) + 1);
	char * paqueteSerializado = malloc(tamanioPaquete);

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
	memcpy(paqueteSerializado + offset, enviarBytes->buffer, tmp_size);

	t_buffer_tamanio * buffer_tamanio = malloc(sizeof(uint32_t) + tamanioPaquete);
	buffer_tamanio->tamanioBuffer = tamanioPaquete;
	buffer_tamanio->buffer = paqueteSerializado;

	return buffer_tamanio;
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
	char *datos = malloc(tamanio + 1);
	memcpy(datos, message + offsetMemcpy, tamanio);

	t_enviarBytes * respuesta = malloc((sizeof(uint32_t) * 4) + tamanio);
	respuesta->pid = pid;
	respuesta->buffer = datos;
	respuesta->offset = offset;
	respuesta->pagina = pagina;
	respuesta->tamanio = tamanio;

	return respuesta;
}

t_buffer_tamanio * serializarCambioProcActivo(t_cambio_proc_activo* cambioProcActivo) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	uint32_t packageSize = sizeof(uint32_t) * 2;
	char * paqueteSerializado = malloc(packageSize);

	memcpy(paqueteSerializado + offset, &(cambioProcActivo->programID), tmp_size);

	t_buffer_tamanio * buffer_tamanio = malloc(sizeof(uint32_t) + packageSize);
	buffer_tamanio->tamanioBuffer = packageSize;
	buffer_tamanio->buffer = paqueteSerializado;

	return buffer_tamanio;
}


char* serializar_opIO(t_dispositivo_io* op_IO){
	uint32_t longitud = strlen(op_IO->nombre) +1;
	uint32_t offset = 0, tmp_size = 0;
	char * paqueteSerializado = malloc(sizeof(uint32_t)+sizeof(int) + longitud);
	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(longitud), tmp_size);
	offset += tmp_size;
	tmp_size = longitud;
	memcpy(paqueteSerializado + offset, op_IO->nombre, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(op_IO->tiempo);
	memcpy(paqueteSerializado + offset, &(op_IO->tiempo), tmp_size);

	return paqueteSerializado;
}

t_dispositivo_io* deserializar_opIO(char* package){
	t_dispositivo_io* op_IO = malloc(sizeof(t_dispositivo_io));
	uint32_t longitud, offset = 0;
	int tmp_size = sizeof(uint32_t);

	memcpy(&longitud, package + offset, tmp_size);
	offset += tmp_size;
	tmp_size = longitud;
	op_IO->nombre = malloc(tmp_size);
	memcpy(op_IO->nombre, package + offset, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(int);
	memcpy(&(op_IO->tiempo), package + offset, tmp_size);

	return op_IO;
}

t_buffer_tamanio * serializarFinalizarPrograma(t_finalizar_programa* destruirPaginas) {
	int tmp_size = sizeof(uint32_t);
	char * paqueteSerializado = malloc(sizeof(t_finalizar_programa));

	memcpy(paqueteSerializado, &(destruirPaginas->programID), tmp_size);

	t_buffer_tamanio * buffer_tamanio = malloc(sizeof(uint32_t) + sizeof(t_finalizar_programa));
	buffer_tamanio->tamanioBuffer = sizeof(t_finalizar_programa);
	buffer_tamanio->buffer = paqueteSerializado;

	return buffer_tamanio;
}

t_finalizar_programa * deserializarFinalizarPrograma(char * buffer) {
	int tmp_size = sizeof(uint32_t);
	t_finalizar_programa * finalizarPrograma = malloc(sizeof(t_finalizar_programa));

	memcpy(&(finalizarPrograma->programID), buffer, tmp_size);

	return finalizarPrograma;
}

t_buffer_tamanio* serializar_pcb(t_pcb* pcb) {
	t_buffer_tamanio* indcod = serializadoIndiceDeCodigo(pcb->ind_codigo);
	t_buffer_tamanio* stackSerializado = serializarIndiceStack(pcb->ind_stack);

	uint32_t tamanioPCB = 0;
	tamanioPCB += stackSerializado->tamanioBuffer;
	tamanioPCB += indcod->tamanioBuffer;
	tamanioPCB += sizeof(uint32_t); //Tamanio Etiquetas
	if (pcb->ind_etiq != NULL && pcb->tamanioEtiquetas > 0) {
		tamanioPCB += pcb->tamanioEtiquetas;
	}
	tamanioPCB += (sizeof(uint32_t) * 11); //Cantidad de uint32_t que tiene PCB
	tamanioPCB += sizeof(uint32_t); //Para indicar tamanio de PCBs

	//Comienzo serializacion
	char* paqueteSerializado = malloc(tamanioPCB);
	uint32_t offset = 0;
	uint32_t tmp_size = sizeof(uint32_t);

	//Serializo tamanio de PCB
	memcpy(paqueteSerializado + offset, &tamanioPCB, tmp_size);
	offset += tmp_size;

	//Serializo los 11 uint32_t del PCB
	memcpy(paqueteSerializado + offset, &(pcb->pcb_id), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(pcb->codigo), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(pcb->PC), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(pcb->paginaCodigoActual), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(pcb->stackPointer), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(pcb->tamanioEtiquetas), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(pcb->numeroContextoEjecucionActualStack), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(pcb->paginaStackActual), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(pcb->primerPaginaStack), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(pcb->quantum), tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado + offset, &(pcb->quantumSleep), tmp_size);
	offset += tmp_size;

	//Serializo Indice de Codigo
	memcpy(paqueteSerializado + offset, indcod->buffer, indcod->tamanioBuffer);
	offset += indcod->tamanioBuffer;

	//Serializo Indice de Stack
	memcpy(paqueteSerializado + offset, stackSerializado->buffer, stackSerializado->tamanioBuffer);
	offset += stackSerializado->tamanioBuffer;

	//Serializo Indice de Etiquetas
	if(pcb->ind_etiq != NULL && pcb->tamanioEtiquetas > 0) {
		//uint32_t longitudIndiceEtiquetas = strlen(pcb->ind_etiq);
		memcpy(paqueteSerializado + offset, &pcb->tamanioEtiquetas, tmp_size);
		offset += tmp_size;
		memcpy(paqueteSerializado + offset, pcb->ind_etiq, pcb->tamanioEtiquetas);
	}else{
		uint32_t longitudIndiceEtiquetas = 0;
		memcpy(paqueteSerializado + offset, &longitudIndiceEtiquetas, tmp_size);
	}


	free(indcod->buffer);
	free(indcod);
	free(stackSerializado->buffer);
	free(stackSerializado);

	t_buffer_tamanio * buffer_tamanio = malloc(sizeof(uint32_t) + tamanioPCB);
	buffer_tamanio->tamanioBuffer = tamanioPCB;
	buffer_tamanio->buffer = paqueteSerializado;

	return buffer_tamanio;
}

t_pcb* deserializar_pcb(char* package) {
 	uint32_t offset = 0, tmp_size = sizeof(uint32_t);
 	uint32_t tamanioPCB;

 	//Tomo el tamanio del PCB
  	memcpy(&tamanioPCB, package + offset, tmp_size);
 	t_pcb *pcb;

 	pcb = malloc(tamanioPCB);
  	offset += tmp_size;
  	printf("");

  	//Tomo los 11 uint32_t del PCB
 	memcpy(&(pcb->pcb_id), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->codigo), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->PC), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->paginaCodigoActual), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->stackPointer), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->tamanioEtiquetas), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->numeroContextoEjecucionActualStack), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->paginaStackActual), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->primerPaginaStack), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->quantum), package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&(pcb->quantumSleep), package + offset, tmp_size);
 	offset += tmp_size;

 	//Tomo Indice de Codigo
 	uint32_t itemsIndiceDeCodigo;
 	memcpy(&itemsIndiceDeCodigo, package + offset, tmp_size);
 	offset += tmp_size;
 	uint32_t tamanioIndiceDeCodigo = sizeof(t_indice_codigo) * itemsIndiceDeCodigo;
 	char * bufferIndiceDeCodigo = malloc(tamanioIndiceDeCodigo);
 	memcpy(bufferIndiceDeCodigo, package + offset, tamanioIndiceDeCodigo);
 	offset += tamanioIndiceDeCodigo;

 	t_list * indiceDeCodigo = deserializarIndiceCodigo(bufferIndiceDeCodigo, itemsIndiceDeCodigo);
 	pcb->ind_codigo = indiceDeCodigo;

 	//Tomo Indice de Stack
 	uint32_t tamanioStack;
 	memcpy(&tamanioStack, package + offset, tmp_size);
 	offset += tmp_size;
 	char * bufferIndiceStack = malloc(tamanioStack);
 	memcpy(bufferIndiceStack, package + offset, tamanioStack);
 	offset += tamanioStack;

 	t_list * indiceStack = deserializarIndiceStack(bufferIndiceStack);
 	pcb->ind_stack = indiceStack;

 	//Tomo Indice de Etiquetas
 	uint32_t longitudIndiceEtiquetas;
 	memcpy(&longitudIndiceEtiquetas, package + offset, tmp_size);
 	offset+= tmp_size;
 	if(longitudIndiceEtiquetas>0) {
 		char * indiceEtiquetas = malloc(pcb->tamanioEtiquetas);
 		memcpy(indiceEtiquetas, package + offset, longitudIndiceEtiquetas);
 		pcb->ind_etiq = indiceEtiquetas;
 		//memcpy(pcb->ind_etiq, package + offset, longitudIndiceEtiquetas);
 	}else{
 		pcb->ind_etiq = NULL;
 	}

 	free(bufferIndiceDeCodigo);
 	free(bufferIndiceStack);

 	return pcb;
 }
 

t_buffer_tamanio * serializar_EstructuraInicial(t_EstructuraInicial * estructuraInicial) {
	char* buffer = malloc(sizeof(t_EstructuraInicial));
	memcpy(buffer, &(estructuraInicial->tamanioStack), sizeof(uint32_t));

	t_buffer_tamanio * buffer_tamanio = malloc(sizeof(uint32_t) + sizeof(t_EstructuraInicial));
	buffer_tamanio->tamanioBuffer = sizeof(t_EstructuraInicial);
	buffer_tamanio->buffer = buffer;

	return buffer_tamanio;
}

t_EstructuraInicial* deserializar_EstructuraInicial(char* package) {
	t_EstructuraInicial * estructuraInicial = malloc(sizeof(t_EstructuraInicial));
	int offset = 0;
	int tmp_size = sizeof(uint32_t);

	memcpy(&estructuraInicial->tamanioStack, package + offset, tmp_size);

	return estructuraInicial;
}

t_buffer_tamanio * serializar_opVarCompartida(t_op_varCompartida* varCompartida) {
	int offset = 0, tmp_size = 0;
	uint32_t tamanioPaquete = (sizeof(uint32_t) * 2) + varCompartida->longNombre;
	char * paqueteSerializado = malloc(tamanioPaquete);

	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(varCompartida->longNombre), tmp_size);
	offset += tmp_size;
	tmp_size = varCompartida->longNombre;
	memcpy(paqueteSerializado + offset, varCompartida->nombre, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(uint32_t);
	memcpy(paqueteSerializado + offset, &(varCompartida->valor), tmp_size);

	t_buffer_tamanio * buffer_tamanio = malloc(sizeof(uint32_t) + tamanioPaquete);
	buffer_tamanio->tamanioBuffer = tamanioPaquete;
	buffer_tamanio->buffer = paqueteSerializado;

	return buffer_tamanio;
}

t_op_varCompartida* deserializar_opVarCompartida(char* package) {
	int offset = 0;
	int tmp_size = sizeof(uint32_t);
	uint32_t longNombre;
	memcpy(&longNombre, package + offset, tmp_size);
	offset += tmp_size;
	uint32_t tamanioPaquete = (sizeof(uint32_t) * 2) + longNombre;
	t_op_varCompartida* varCompartida = malloc(tamanioPaquete);
	varCompartida->longNombre = longNombre;
	tmp_size = longNombre;
	varCompartida->nombre = malloc(tmp_size);
	memcpy(varCompartida->nombre, package + offset, tmp_size);
	offset += tmp_size;
	tmp_size = sizeof(uint32_t);
	memcpy(&(varCompartida->valor), package + offset, tmp_size);
	return varCompartida;
}

t_buffer_tamanio* serializadoIndiceDeCodigo(t_list* indiceCodigo) {
	uint32_t offset = 0, m = 0;
	int tmp_size = sizeof(uint32_t);
	char* bufferIndCod = malloc((list_size(indiceCodigo) * (2 * sizeof(uint32_t))) + sizeof(uint32_t));
	uint32_t itemsEnLista = list_size(indiceCodigo);

	memcpy(bufferIndCod + offset, &itemsEnLista, tmp_size);
	offset += tmp_size;

	t_indice_codigo* linea;
	for (m = 0; m < itemsEnLista; m++) {
		linea = list_get(indiceCodigo, m);
		memcpy(bufferIndCod + offset, &(linea->start), tmp_size);
		offset += tmp_size;
		memcpy(bufferIndCod + offset, &(linea->offset), tmp_size);
		offset += tmp_size;
	}

	t_buffer_tamanio * tamanioBuffer = malloc(sizeof(uint32_t) + offset);
	tamanioBuffer->tamanioBuffer = offset;
	tamanioBuffer->buffer = bufferIndCod;

	return tamanioBuffer;
}

t_list* deserializarIndiceCodigo(char* indice, uint32_t tam) {
	t_list* ret = list_create();
	uint32_t offset = 0;
	uint32_t tmp_size = sizeof(uint32_t);
	int i = 0;
	for (i = 0; i < tam; i++) {
		t_indice_codigo* linea = malloc(sizeof(t_indice_codigo));
		memcpy(&(linea->start), indice + offset, tmp_size);
		offset += tmp_size;
		memcpy(&(linea->offset), indice + offset, tmp_size);
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

		int cantidadArgumentos = list_size(linea->argumentos);
		tamanioTotalBuffer += cantidadArgumentos * sizeof(t_argumento); //tamanio de lista de argumentos
		tamanioStackParticular += cantidadArgumentos * sizeof(t_argumento);

		int cantidadVariables = list_size(linea->variables);
		tamanioTotalBuffer += cantidadVariables * sizeof(t_variable); //tamanio lista de variables
		tamanioStackParticular += cantidadVariables * sizeof(t_variable);

		tamanioTotalBuffer += sizeof(uint32_t); //tamanio de variable direcretorno
		tamanioStackParticular += sizeof(uint32_t);

		tamanioTotalBuffer += sizeof(t_argumento); //tamanio de retVar
		tamanioStackParticular += sizeof(t_argumento);

		tamanioTotalBuffer += sizeof(uint32_t) * 3; //agrego 3 ints para indicar la cantidad de elemento de las 2 listas y los bytes de t_stack

		t_tamanio_stack_stack * auxiliar = malloc(sizeof(uint32_t) + tamanioStackParticular);
		auxiliar->stack = linea;
		auxiliar->tamanioStack = tamanioStackParticular;
		list_add(tamanioStackStack, auxiliar);
	}

	char * buffer = malloc(tamanioTotalBuffer + sizeof(uint32_t));
	int tamanioUint32 = sizeof(uint32_t), offset = 0;

	memcpy(buffer + offset, &tamanioTotalBuffer, tamanioUint32);
	offset += tamanioUint32;

	uint32_t cantidadItemsEnStack = list_size(indiceStack);

	memcpy(buffer + offset, &cantidadItemsEnStack, tamanioUint32);
	offset += tamanioUint32;

	for(a = 0; a < list_size(tamanioStackStack); a++) {
		t_tamanio_stack_stack * linea = list_get(tamanioStackStack, a);

		memcpy(buffer + offset, &(linea->tamanioStack), tamanioUint32);
		offset += tamanioUint32;

		t_stack * stack = linea->stack;

		uint32_t cantidadArgumentos = list_size(stack->argumentos);
		memcpy(buffer + offset, &cantidadArgumentos, tamanioUint32);
		offset += tamanioUint32;

		int p;
		for(p = 0; p < cantidadArgumentos; p++) {
			t_argumento *argumento = list_get(stack->argumentos, p);
			memcpy(buffer + offset, &(argumento->pagina), tamanioUint32);
			offset += tamanioUint32;
			memcpy(buffer + offset, &(argumento->offset), tamanioUint32);
			offset += tamanioUint32;
			memcpy(buffer + offset, &(argumento->size), tamanioUint32);
			offset += tamanioUint32;
			free(argumento);
		}

		uint32_t cantidadVariables = list_size(stack->variables);
		memcpy(buffer + offset, &cantidadVariables, tamanioUint32);
		offset += tamanioUint32;

		for(p = 0; p < cantidadVariables; p++) {
			t_variable *variable = list_get(stack->variables, p);
			memcpy(buffer + offset, &(variable->idVariable), sizeof(char));
			offset += sizeof(char);
			memcpy(buffer + offset, &(variable->pagina), tamanioUint32);
			offset += tamanioUint32;
			memcpy(buffer + offset, &(variable->offset), tamanioUint32);
			offset += tamanioUint32;
			memcpy(buffer + offset, &(variable->size), tamanioUint32);
			offset += tamanioUint32;
			free(variable);
		}

		memcpy(buffer + offset, &(stack->direcretorno), tamanioUint32);
		offset += tamanioUint32;

		t_argumento * retVarStack = stack->retVar;
		if(retVarStack == NULL) {
			retVarStack = malloc(sizeof(t_argumento));
			retVarStack->offset = 0;
			retVarStack->pagina = 0;
			retVarStack->size = 0;
			memcpy(buffer + offset, &(retVarStack->pagina), tamanioUint32);
			offset += tamanioUint32;
			memcpy(buffer + offset, &(retVarStack->offset), tamanioUint32);
			offset += tamanioUint32;
			memcpy(buffer + offset, &(retVarStack->size), tamanioUint32);
			offset += tamanioUint32;
			free(retVarStack);
		}else{
			memcpy(buffer + offset, &(retVarStack->pagina), tamanioUint32);
			offset += tamanioUint32;
			memcpy(buffer + offset, &(retVarStack->offset), tamanioUint32);
			offset += tamanioUint32;
			memcpy(buffer + offset, &(retVarStack->size), tamanioUint32);
			offset += tamanioUint32;
			free(retVarStack);
		}
	}

	for(a = 0; a < list_size(tamanioStackStack); a++) {
		t_tamanio_stack_stack * tamanioStack = list_get(tamanioStackStack, a);
		t_stack * stack = tamanioStack->stack;
		list_remove(tamanioStackStack, a);
		a--;
		free(stack);
		free(tamanioStack);
	}
	free(tamanioStackStack);

	t_buffer_tamanio *bufferTamanio = malloc((sizeof(uint32_t)*2) + tamanioTotalBuffer);
	bufferTamanio->tamanioBuffer = tamanioTotalBuffer + sizeof(uint32_t);
	bufferTamanio->buffer = buffer;

	return bufferTamanio;
}

t_list* deserializarIndiceStack(char* buffer) {
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

		t_stack * stack_item = malloc(tamanioItemStack);

		t_list * argumentosStack = list_create();
		uint32_t cantidadArgumentosStack;
		memcpy(&cantidadArgumentosStack, buffer + offset, tamanioUint32);
		offset += tamanioUint32;
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
		offset += tamanioUint32;
		for(p = 0; p < cantidadVariablesStack; p++) {
			t_variable * varStack = malloc(sizeof(t_variable));
			memcpy(&(varStack->idVariable), buffer + offset, sizeof(char));
			offset += sizeof(char);
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
		stack_item->direcretorno = direcRetorno;
		offset += tamanioUint32;

		t_argumento * retVarStack = malloc(sizeof(t_argumento));
		memcpy(&(retVarStack->pagina), buffer + offset, tamanioUint32);
		offset += tamanioUint32;
		memcpy(&(retVarStack->offset), buffer + offset, tamanioUint32);
		offset += tamanioUint32;
		memcpy(&(retVarStack->size), buffer + offset, tamanioUint32);
		offset += tamanioUint32;
		if(retVarStack->pagina == 0 && retVarStack->offset == 0 && retVarStack->size == 0){
			retVarStack = NULL;
		}
		stack_item->retVar = retVarStack;

		list_add(stack, stack_item);
	}

	return stack;
}

t_list* llenarLista(t_intructions * indiceCodigo, t_size cantInstruc) {
	t_list * lista = list_create();
	int b = 0;
	for (b = 0; b < cantInstruc; b++) {
		t_indice_codigo* linea = malloc(sizeof(t_indice_codigo));
		linea->start = indiceCodigo[b].start;
		linea->offset = indiceCodigo[b].offset;
		list_add(lista, linea);
	}
	return lista;
}

t_list* llenarStack(t_list * lista, t_stack* lineaAgregar) {
		list_add(lista, lineaAgregar);
	return lista;
}
