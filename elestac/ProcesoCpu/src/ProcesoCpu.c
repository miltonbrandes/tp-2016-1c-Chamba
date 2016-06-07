/*
 * procesoCpu.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */

#include "ProcesoCpu.h"

#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <parser/parser.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>
#include <sockets/OpsUtiles.h>
#include <sockets/StructsUtiles.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "PrimitivasAnSISOP.h"

#define MAX_BUFFER_SIZE 4096
t_pcb *pcb;
uint32_t quantum, quantumSleep;
//uint32_t tamanioPagina;

AnSISOP_funciones functions = {
		.AnSISOP_asignar = asignar,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,
		.AnSISOP_definirVariable = definirVariable,
		.AnSISOP_dereferenciar = dereferenciar,
		.AnSISOP_entradaSalida = entradaSalida,
		.AnSISOP_imprimir = imprimir,
		.AnSISOP_imprimirTexto = imprimirTexto,
		.AnSISOP_irAlLabel = irAlLabel,
		.AnSISOP_obtenerPosicionVariable = obtenerPosicionVariable,
		.AnSISOP_obtenerValorCompartida =obtenerValorCompartida,
		.AnSISOP_retornar = retornar,
		.AnSISOP_llamarConRetorno = llamarConRetorno };

AnSISOP_kernel kernel_functions = {
		.AnSISOP_signal = signal,
		.AnSISOP_wait = wait };

int crearLog() {
	ptrLog = log_create(getenv("CPU_LOG"), "ProcesoCpu", 1, 0);
	if (ptrLog) {
		return 1;
	} else {
		return 0;
	}
}
int main() {
	crearLog();

	t_config* config;
	config = config_create(getenv("CPU_CONFIG"));
	char *direccionUmc = config_get_string_value(config, "IP_UMC");
	int puertoUmc = config_get_int_value(config, "PUERTO_UMC");
	socketUMC = crearSocketCliente(direccionUmc, puertoUmc);

	if (socketUMC > 0) {
		if (manejarPrimeraConexionConUMC()) {
			char *direccionNucleo = config_get_string_value(config, "IP_NUCLEO");
			int puertoNucleo = config_get_int_value(config, "PUERTO_NUCLEO");
			socketNucleo = crearSocketCliente(direccionNucleo, puertoNucleo);

			return controlarConexiones();
		} else {
			log_error(ptrLog, "Error al recibir primer mensaje de UMC");
			return -1;
		}

	} else {
		log_error(ptrLog, "No se pudo abrir la conexion con UMC");
		return -1;
	}
}

int manejarPrimeraConexionConUMC() {
	if(recibirMensaje(socketUMC) == 0) {
		return 1;
	}else{
		return 0;
	}
}

int controlarConexiones() {
	while (1) {
		log_info(ptrLog, "Esperando mensajes de Nucleo");
		if (recibirMensaje(socketNucleo) == 0) {

		} else {
			return 0;
		}
	}
	return 0;

}

int crearSocketCliente(char* direccion, int puerto) {
	int socketConexion;
	socketConexion = AbrirConexion(direccion, puerto);
	if (socketConexion < 0) {
		log_error(ptrLog, "Error en la conexion con el servidor");
		return -1;
	}
	log_info(ptrLog, "Socket creado y conectado");
	return socketConexion;
}


int recibirMensaje(int socket) {
	uint32_t id;
	uint32_t op;

	char *respuestaServidor = recibirDatos(socket, &op, &id);
	log_info(ptrLog, "Recibo mensaje");

	if(strcmp(respuestaServidor, "ERROR") == 0) {
		free(respuestaServidor);
		if (socket == socketNucleo) {
			log_info(ptrLog, "No se recibio nada de Nucleo, cierro conexion");
			finalizarConexion(socketNucleo);
			return -1;
		} else if (socket == socketUMC) {
			log_info(ptrLog, "No se recibio nada de UMC, cierro conexion");
			finalizarConexion(socketUMC);
			return -1;
		}
	}else{
		manejarMensajeRecibido(id, op, respuestaServidor);
	}

	return 0;
}

//Veo quien me mando mensajes
void manejarMensajeRecibido(uint32_t id, uint32_t op, char *mensaje) {
	switch (id) {
	case NUCLEO:
		manejarMensajeRecibidoNucleo(op, mensaje);
		break;
	case UMC:
		manejarMensajeRecibidoUMC(op, mensaje);
		break;
	}
}

void manejarMensajeRecibidoNucleo(uint32_t op, char *mensaje) {
	switch (op) {
	case QUANTUM_PARA_CPU:
		recibirQuantum(mensaje);
		break;
	case EXECUTE_PCB:
		recibirPCB(mensaje);
		break;
	case VALOR_VAR_COMPARTIDA:
		recibirValorVariableCompartida(mensaje);
		break;
	case ASIG_VAR_COMPARTIDA:
		recibirAsignacionVariableCompartida(mensaje);
		break;
	case SIGNAL_SEMAFORO:
		recibirSignalSemaforo(mensaje);
		break;
	default:
		break;
	}
}

void manejarMensajeRecibidoUMC(uint32_t op, char *mensaje) {
	switch (op) {
	case ENVIAR_TAMANIO_PAGINA_A_CPU:
		recibirTamanioPagina(mensaje);
		break;
	case ENVIAR_INSTRUCCION_A_CPU:
		recibirInstruccion(mensaje);
		break;
	}
}
//Fin veo quien me mando mensajes

//Manejo de mensajes recibidos
void recibirPCB(char *mensaje) {
	pcb = deserializar_pcb(mensaje);
	free(mensaje);
	setPCB(pcb);
	notificarAUMCElCambioDeProceso(pcb->pcb_id);
	comenzarEjecucionDePrograma();
}

void recibirQuantum(char *mensaje) {
	t_EstructuraInicial *estructuraInicial = deserializar_EstructuraInicial(mensaje);
	quantum = estructuraInicial->Quantum;
	quantumSleep = estructuraInicial->RetardoQuantum;
	log_info(ptrLog, "Quantum: %d - Quantum Sleep: %d\n", quantum, quantumSleep);
	free(mensaje);
}

void recibirTamanioPagina(char *mensaje) {
	tamanioPagina = deserializarUint32(mensaje);
	log_info(ptrLog, "Tamanio Pagina: %d\n", tamanioPagina);
	free(mensaje);
}

void recibirValorVariableCompartida(char *mensaje) {

	free(mensaje);
}

void recibirAsignacionVariableCompartida(char *mensaje) {

	free(mensaje);
}

void recibirSignalSemaforo(char *mensaje) {

	free(mensaje);
}

void recibirInstruccion(char *mensaje) {

	free(mensaje);
}
//Fin manejo de mensajes recibidos

void notificarAUMCElCambioDeProceso(uint32_t pid) {
	t_cambio_proc_activo * cambioProcActivo = malloc(sizeof(uint32_t));
	cambioProcActivo->programID = pid;

	t_buffer_tamanio * buffer_tamanio = serializarCambioProcActivo(cambioProcActivo);

	int bytesEnviados = enviarDatos(socketUMC, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, CAMBIOPROCESOACTIVO, CPU);

	if(bytesEnviados<=0) {
		log_error(ptrLog, "Algo malo ocurrio al enviar el Cambio de Proceso a UMC.");
	}
}

void finalizarEjecucionPorExit() {
	operacion = NOTHING;
	t_buffer_tamanio * buffer_tamanio = serializar_pcb(pcb);
	int bytesEnviados = enviarDatos(socketNucleo, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, EXIT, CPU);
	if(bytesEnviados <= 0) {
		log_error(ptrLog, "Error al devolver el PCB por Finalizacion a Nucleo");
	}
	else{
		log_info(ptrLog, "Programa Finalizado con exito");
	}
	free(buffer_tamanio);
}

void finalizarEjecucionPorIO(){
	operacion = NOTHING;
	t_buffer_tamanio* buffer_tamanio = serializar_pcb(pcb);
	int bytesEnviados = enviarDatos(socketNucleo, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, IO, CPU);
	if(bytesEnviados <= 0){
		log_error(ptrLog, "Error al devolver el PCB por finalizacion de ejecucion por io a nucleo");
	}
	free(buffer_tamanio);
}

void finalizarEjecucionPorWait(){
	operacion = NOTHING;
	t_buffer_tamanio* buffer_tamanio = serializar_pcb(pcb);
	int bytesEnviados = enviarDatos(socketNucleo, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, WAIT, CPU);
	if(bytesEnviados <= 0){
		log_error(ptrLog, "Error al devolver el PCB por finalizacion de ejecucion por wait a nucleo");
	}
	free(buffer_tamanio);
}

void finalizarEjecucionPorQuantum() {
	operacion = NOTHING;
	t_buffer_tamanio * message = serializar_pcb(pcb);
	int bytesEnviados = enviarDatos(socketNucleo, message->buffer, message->tamanioBuffer, QUANTUM, CPU);
	if (bytesEnviados <= 0) {
		log_error(ptrLog, "Error al devolver el PCB por Quantum a Nucleo");
	}
	free(message);
}

void limpiarInstruccion(char * instruccion) {
     char *p2 = instruccion;
     int a = 0;
     while(*instruccion != '\0') {
     	if(*instruccion != '\t' && *instruccion != '\n') {
     		if(a == 0 && isdigit((int)*instruccion)){
     			++instruccion;
     		}else{
				*p2++ = *instruccion++;
				a++;
     		}
     	}
     	else {
     		++instruccion;
     	}
     }
     *p2 = '\0';
 }


void comenzarEjecucionDePrograma() {
	log_info(ptrLog, "Recibo PCB");
	int contador = 1;

	while (contador <= quantum) {
		if(pcb->PC >= (pcb->codigo-1)) {
			finalizarEjecucionPorExit();
			return;
		}else{
			char* proximaInstruccion = solicitarProximaInstruccionAUMC();
			limpiarInstruccion(proximaInstruccion);
			log_debug(ptrLog, "Instruccion a ejecutar: %s", proximaInstruccion);
			analizadorLinea(proximaInstruccion, &functions, &kernel_functions);
			contador++;
			pcb->PC = (pcb->PC) + 1;
		}
		switch(operacion){
			case IO:
				log_debug(ptrLog, "Finalizo ejecucion por operacion IO");
				finalizarEjecucionPorIO();
				return;
			case WAIT:
				log_debug(ptrLog, "Finalizo ejecucion por un wait ansisop");
				finalizarEjecucionPorWait();
				return;
			default:
				break;
		}
		sleep(quantumSleep);
	}
	log_debug(ptrLog, "Finalizo ejecucion por fin de quantum");
	finalizarEjecucionPorQuantum();
	free(pcb);
}

char * solicitarProximaInstruccionAUMC() {

	t_indice_codigo *indice = list_get(pcb->ind_codigo, pcb->PC);
	uint32_t requestStart = indice->start;
	uint32_t requestOffset = indice->offset;

	uint32_t contador = 0;
	while (requestStart >= (tamanioPagina + (tamanioPagina * contador))) {
		contador++;
	}
	uint32_t paginaAPedir = contador;
	pcb->paginaCodigoActual = paginaAPedir;

	t_solicitarBytes * solicitarBytes = malloc(sizeof(t_solicitarBytes));
	solicitarBytes->pagina = paginaAPedir;
	solicitarBytes->start = requestStart - (tamanioPagina * paginaAPedir);
	solicitarBytes->offset = requestOffset;

	log_info(ptrLog, "Pido a UMC-> Pagina: %d - Start: %d - Offset: %d", paginaAPedir, requestStart - (tamanioPagina * paginaAPedir), solicitarBytes->offset);

	t_buffer_tamanio * buffer_tamanio = serializarSolicitarBytes(solicitarBytes);

	int enviarBytes = enviarDatos(socketUMC, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, LEER, CPU);
	if (enviarBytes <= 0) {
		log_error(ptrLog, "Ocurrio un error al enviar una solicitud de instruccion a CPU");
		return NULL;
	} else {
		log_info(ptrLog, "Recibo instruccion %d del Proceso %d -> Pagina: %d - Start: %d - Offset: %d", pcb->PC, pcb->pcb_id, paginaAPedir, requestStart, requestOffset);
		uint32_t op, id;
		char* instruccionRecibida = recibirDatos(socketUMC, &op, &id);
		t_instruccion * instruccion = deserializarInstruccion(instruccionRecibida);
		free(instruccionRecibida);
		return instruccion->instruccion;
	}
}
