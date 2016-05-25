/*
 * procesoCpu.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>
#include <sockets/StructsUtiles.h>
#include <sockets/OpsUtiles.h>
#include "ProcesoCpu.h"
#include "PrimitivasAnSISOP.h"

#define MAX_BUFFER_SIZE 4096
t_log* ptrLog;
t_config* config;

t_pcb *pcb;
int socketNucleo, socketUMC;
uint32_t quantum, quantumSleep, tamanioPagina;

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

int enviarMensaje(int socket, char *mensaje) {
	uint32_t id = 2;
	uint32_t longitud = strlen(mensaje);
	uint32_t operacion = 1;
	int sendBytes = enviarDatos(socket, mensaje, longitud, operacion, id);
	if (sendBytes < 0) {
		log_error(ptrLog, "Error al escribir");
		return -1;
	}
	log_info(ptrLog, "Mensaje Enviado");
	return 0;
}

int recibirMensaje(int socket) {
	uint32_t id;
	uint32_t operacion;

	char *respuestaServidor = recibirDatos(socket, &operacion, &id);
	int bytesRecibidos = strlen(respuestaServidor);

	if (bytesRecibidos < 0) {
		log_error(ptrLog, "Error en al lectura del mensaje del servidor");
		return -1;

	} else if (bytesRecibidos == 0) {

		if (socket == socketNucleo) {
			log_info(ptrLog, "No se recibio nada de Nucleo, cierro conexion");
			finalizarConexion(socketNucleo);
		} else if (socket == socketUMC) {
			log_info(ptrLog, "No se recibio nada de UMC, cierro conexion");
			finalizarConexion(socketUMC);
		}
		return -1;

	} else {
		if (strcmp("ERROR", respuestaServidor) == 0) {
			return -1;
		} else {
			manejarMensajeRecibido(id, operacion, respuestaServidor);
		}
	}

	return 0;
}

//Veo quien me mando mensajes
void manejarMensajeRecibido(uint32_t id, uint32_t operacion, char *mensaje) {
	switch (id) {
	case NUCLEO:
		manejarMensajeRecibidoNucleo(operacion, mensaje);
		break;
	case UMC:
		manejarMensajeRecibidoUMC(operacion, mensaje);
		break;
	}
}

void manejarMensajeRecibidoNucleo(uint32_t operacion, char *mensaje) {
	switch (operacion) {
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

void manejarMensajeRecibidoUMC(uint32_t operacion, char *mensaje) {
	switch (operacion) {
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
	comenzarEjecucionDePrograma();
}

void recibirQuantum(char *mensaje) {
	t_EstructuraInicial *estructuraInicial = deserializar_EstructuraInicial(mensaje);
	quantum = estructuraInicial->Quantum;
	quantumSleep = estructuraInicial->RetardoQuantum;
	log_info(ptrLog, "Quantum: %d - Quantum Sleep: %d\n", quantum, quantumSleep);
}

void recibirTamanioPagina(char *mensaje) {
	tamanioPagina = deserializarUint32(mensaje);
	log_info(ptrLog, "Tamanio Pagina: %d\n", tamanioPagina);
}

void recibirValorVariableCompartida(char *mensaje) {

}

void recibirAsignacionVariableCompartida(char *mensaje) {

}

void recibirSignalSemaforo(char *mensaje) {

}

void recibirInstruccion(char *mensaje) {

}
//Fin manejo de mensajes recibidos

void finalizarEjecucionPorQuantum() {
	char *message = serializar_pcb(pcb);
	int bytesEnviados = enviarDatos(socketNucleo, message, strlen(message), QUANTUM, CPU);
	if (bytesEnviados <= 0) {
		log_error(ptrLog, "Error al devolver el PCB por Quantum a CPU");
	}
}

void comenzarEjecucionDePrograma() {
	int contador = 1;
	while (contador <= quantum) {
		char* proximaInstruccion = solicitarProximaInstruccionAUMC();
		analizadorLinea(proximaInstruccion, &functions, &kernel_functions);
		contador++;
		pcb->PC = (pcb->PC) + 1;
		sleep(quantumSleep);
	}
	finalizarEjecucionPorQuantum();
}

char* solicitarProximaInstruccionAUMC() {
	int tamanioTotalInstruccion = 0;
	t_list * listaInstrucciones = list_create();
	t_list * requestsUMC = crearRequestsParaUMC();

	char *message;
	uint32_t operation, id;

	int i;
	for (i = 0; i < list_size(requestsUMC); i++) {
		t_solicitarBytes *request = list_get(requestsUMC, i);
		t_buffer_tamanio *requestSerializado = serializarSolicitarBytes(request);
		int bytesEnviados = enviarDatos(socketUMC, requestSerializado->buffer, requestSerializado->tamanioBuffer, LEER, CPU);
		if (bytesEnviados <= 0) {
			//Error
		} else {
			message = recibirDatos(socketUMC, &operation, &id);
			if (strlen(message) < 0) {
				//Error
			} else if (strlen(message) == 0) {
				//Error
			} else {
				if (strcmp("ERROR", message) == 0) {
					//Error
				} else {
					t_enviarBytes *bytesRecibidos = deserializarEnviarBytes( message);
					char *datos = malloc(bytesRecibidos->tamanio);
					memcpy(datos, &(bytesRecibidos->buffer), bytesRecibidos->tamanio);
					list_add(listaInstrucciones, datos);
					tamanioTotalInstruccion += bytesRecibidos->tamanio;
				}
			}

			free(message);
		}
		free(requestSerializado);
	}

	char *instruccionCompleta = malloc(tamanioTotalInstruccion);
	int j;
	for (j = 0; j < list_size(listaInstrucciones); j++) {
		instruccionCompleta = strcpy(instruccionCompleta, list_get(listaInstrucciones, j));
	}

	return instruccionCompleta;
}

t_list * crearRequestsParaUMC() {
	t_list * requestsParaUMC = list_create();

	t_indice_codigo *indice = list_get(pcb->ind_codigo, pcb->PC);
	uint32_t requestStart = indice->start;
	uint32_t requestOffset = indice->offset;

	uint32_t contador = 0;
	while (requestStart > (tamanioPagina + (tamanioPagina * contador))) {
		contador++;
	}
	uint32_t paginaAPedir = contador + pcb->posicionPrimerPaginaCodigo;

	//Armo el primer request
	t_solicitarBytes *request = malloc(sizeof(t_solicitarBytes));
	request->pagina = paginaAPedir;
	request->start = requestStart;
	if ((requestStart + requestOffset) <= (tamanioPagina * contador)) {
		request->offset = requestOffset;
		requestStart = -1;
		requestOffset = -1;
	} else {
		request->offset = tamanioPagina * contador;
		requestStart = (tamanioPagina * contador) + 1;
		requestOffset = requestOffset - tamanioPagina;
		contador++;
	}
	list_add(requestsParaUMC, request);

	//Veo si tengo que armar mas request porque me pase de Pagina
	while ((requestStart + requestOffset) > tamanioPagina) {
		paginaAPedir++;

		t_solicitarBytes *requestWhile = malloc(sizeof(t_solicitarBytes));
		requestWhile->pagina = paginaAPedir;
		requestWhile->start = requestStart;

		if ((requestStart + requestOffset) <= (tamanioPagina * contador)) {
			requestWhile->offset = requestOffset;
			requestStart = -1;
			requestOffset = -1;
		} else {
			requestWhile->offset = tamanioPagina * contador;
			requestStart = (tamanioPagina * contador) + 1;
			requestOffset = requestOffset - tamanioPagina;
			contador++;
		}

		list_add(requestsParaUMC, requestWhile);
	}

	return requestsParaUMC;
}

