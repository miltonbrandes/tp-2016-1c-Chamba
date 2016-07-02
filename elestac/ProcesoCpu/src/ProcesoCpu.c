/*
 * procesoCpu.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */

#include "ProcesoCpu.h"
//TODO: fijarse en una funcion el pc cuando deberia terminar!!!
#include <commons/collections/list.h>
#include <ctype.h>
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
#include <stdbool.h>
#include "PrimitivasAnSISOP.h"
#include <signal.h>
#define MAX_BUFFER_SIZE 4096

t_pcb *pcb;
t_config* config;
bool cerrarCPU = false;
bool huboStackOver = false;
AnSISOP_funciones functions = { .AnSISOP_asignar = asignar,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,
		.AnSISOP_definirVariable = definirVariable, .AnSISOP_dereferenciar =
				dereferenciar, .AnSISOP_entradaSalida = entradaSalida,
		.AnSISOP_imprimir = imprimir, .AnSISOP_imprimirTexto = imprimirTexto,
		.AnSISOP_irAlLabel = irAlLabel, .AnSISOP_obtenerPosicionVariable =
				obtenerPosicionVariable, .AnSISOP_obtenerValorCompartida =
				obtenerValorCompartida, .AnSISOP_retornar = retornar,
		.AnSISOP_llamarConRetorno = llamarConRetorno };

AnSISOP_kernel kernel_functions = { .AnSISOP_signal = ansisop_signal,
		.AnSISOP_wait = wait };


int main() {
	crearLog();
	config = config_create(getenv("CPU_CONFIG"));
	char *direccionUmc = config_get_string_value(config, "IP_UMC");
	int puertoUmc = config_get_int_value(config, "PUERTO_UMC");
	socketUMC = crearSocketCliente(direccionUmc, puertoUmc);
	/*
	 * Manejo de la interrupcion SIGUSR1
	 */
	signal(SIGUSR1, revisarSigusR1);
	if (socketUMC > 0) {
		if (manejarPrimeraConexionConUMC()) {
			char *direccionNucleo = config_get_string_value(config,
					"IP_NUCLEO");
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
	free(config);
}

////FUNCIONES CPU////

int crearLog() {
	ptrLog = log_create(getenv("CPU_LOG"), "ProcesoCpu", 1, 0);
	if (ptrLog) {
		return 1;
	} else {
		return 0;
	}
}

void revisarSigusR1(int signo) {
	if (signo == SIGUSR1) {
		char * buffer = "SIGUSR1";
		uint32_t tam = 8;
		log_info(ptrLog, "Se recibe SIGUSR1");
//		if (operacion == NOTHING) {
//			finalizarConexion(socketNucleo);
//			return;
//		}
		cerrarCPU = true;
		enviarDatos(socketNucleo, buffer, tam, SIGUSR, CPU);
		log_debug(ptrLog, "Termina la rafaga actual y luego se cierra esta CPU.");
	}
}

int manejarPrimeraConexionConUMC() {
	if (recibirMensaje(socketUMC) == 0) {
		return 1;
	} else {
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

	if (strcmp(respuestaServidor, "ERROR") == 0) {
		//free(respuestaServidor);
		if (socket == socketNucleo) {
			log_info(ptrLog, "No se recibio nada de Nucleo, cierro conexion");
			finalizarConexion(socketNucleo);
			return -1;
		} else if (socket == socketUMC) {
			log_info(ptrLog, "No se recibio nada de UMC, cierro conexion");
			finalizarConexion(socketUMC);
			return -1;
		}
	} else {
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
	case TAMANIO_STACK_PARA_CPU:
		recibirTamanioStack(mensaje);
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

void recibirTamanioStack(char *mensaje) {
	t_EstructuraInicial *estructuraInicial = deserializar_EstructuraInicial(
			mensaje);
	tamanioStack = estructuraInicial->tamanioStack;
	log_info(ptrLog, "Tamanio Stack: %d", tamanioStack);
	free(mensaje);
	free(estructuraInicial);
}

void recibirTamanioPagina(char *mensaje) {
	tamanioPagina = deserializarUint32(mensaje);
	log_info(ptrLog, "Tamanio Pagina: %d", tamanioPagina);
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

	t_buffer_tamanio * buffer_tamanio = serializarCambioProcActivo(
			cambioProcActivo);

	int bytesEnviados = enviarDatos(socketUMC, buffer_tamanio->buffer,
			buffer_tamanio->tamanioBuffer, CAMBIOPROCESOACTIVO, CPU);

	free(buffer_tamanio->buffer);
	free(buffer_tamanio);

	if (bytesEnviados <= 0) {
		log_error(ptrLog, "Error al enviar Cambio de Proceso a UMC.");
	}
}

void finalizarEjecucionPorExit() {
	operacion = NOTHING;
	t_buffer_tamanio * buffer_tamanio = serializar_pcb(pcb);
	int bytesEnviados = enviarDatos(socketNucleo, buffer_tamanio->buffer,
			buffer_tamanio->tamanioBuffer, EXIT, CPU);
	if (bytesEnviados <= 0) {
		log_error(ptrLog, "Error al devolver el PCB por Finalizacion a Nucleo");
	} else {
		log_info(ptrLog, "Programa Finalizado con exito");
	}
	free(buffer_tamanio->buffer);
	free(buffer_tamanio);
	if (cerrarCPU) {
		log_debug(ptrLog, "Cerrando CPU");
		if (operacion != NOTHING)
			finalizarConexion(socketNucleo);
		finalizarConexion(socketUMC);
		log_info(ptrLog, "CPU cerrada");
		log_destroy(ptrLog);
		config_destroy(config);
		return;
	}
}

void finalizarEjecucionPorIO() {
	operacion = NOTHING;
	t_buffer_tamanio* buffer_tamanio = serializar_pcb(pcb);
	int bytesEnviados = enviarDatos(socketNucleo, buffer_tamanio->buffer,
			buffer_tamanio->tamanioBuffer, IO, CPU);
	if (bytesEnviados <= 0) {
		log_error(ptrLog,
				"Error al devolver el PCB por finalizacion de ejecucion por I/O a Nucleo");
	}
	free(buffer_tamanio->buffer);
	free(buffer_tamanio);
}

void finalizarEjecucionPorWait() {
	operacion = NOTHING;
	t_buffer_tamanio* buffer_tamanio = serializar_pcb(pcb);
	int bytesEnviados = enviarDatos(socketNucleo, buffer_tamanio->buffer,
			buffer_tamanio->tamanioBuffer, WAIT, CPU);
	if (bytesEnviados <= 0) {
		log_error(ptrLog,
				"Error al devolver el PCB por finalizacion de ejecucion por Wait a Nucleo");
	}
	free(buffer_tamanio->buffer);
	free(buffer_tamanio);
}

void finalizarEjecucionPorQuantum() {
	operacion = NOTHING;
	t_buffer_tamanio * message = serializar_pcb(pcb);
	int bytesEnviados = enviarDatos(socketNucleo, message->buffer,
			message->tamanioBuffer, QUANTUM, CPU);
	if (bytesEnviados <= 0) {
		log_error(ptrLog, "Error al devolver el PCB por Quantum a Nucleo");
	}
	free(message->buffer);
	free(message);
}

void limpiarInstruccion(char * instruccion) {
	char *p2 = instruccion;
	int a = 0;
	while (*instruccion != '\0') {
		if (*instruccion
				!= '\t'&& *instruccion != '\n' && !iscntrl(*instruccion)) {
			if (a == 0 && isdigit((int )*instruccion)) {
				++instruccion;
			} else {
				*p2++ = *instruccion++;
				a++;
			}
		} else {
			++instruccion;
		}
	}
	*p2 = '\0';
}

void revisarFinalizarCPU() {
	if (cerrarCPU) {
		log_debug(ptrLog, "Cerrando CPU");
		if (operacion != NOTHING)
			finalizarConexion(socketNucleo);
		finalizarConexion(socketUMC);
		log_info(ptrLog, "CPU cerrada");
		log_destroy(ptrLog);
		config_destroy(config);
		return;
	}
}

void finalizarProcesoPorErrorEnUMC() {
	operacion = NOTHING;
	t_buffer_tamanio * message = serializar_pcb(pcb);
	int bytesEnviados = enviarDatos(socketNucleo, message->buffer,
			message->tamanioBuffer, FINALIZO_POR_ERROR_UMC, CPU);
	if (bytesEnviados <= 0) {
		log_error(ptrLog, "Error al devolver el PCB por Error en UMC a Nucleo");
	}
	free(message->buffer);
	free(message);
}

void finalizarProcesoPorStackOverflow() {
	operacion = NOTHING;
	t_buffer_tamanio * message = serializar_pcb(pcb);
	huboStackOver = false;
	int bytesEnviados = enviarDatos(socketNucleo, message->buffer,
			message->tamanioBuffer, STACKOVERFLOW, CPU);
	if (bytesEnviados <= 0) {
		log_error(ptrLog, "Error al devolver el PCB por StackOverflow a Nucleo");
	}
	free(message->buffer);
	free(message);
}

void comenzarEjecucionDePrograma() {
	log_info(ptrLog, "Recibo PCB id: %i", pcb->pcb_id);
	int contador = 1;

	while (contador <= pcb->quantum) {
		char* proximaInstruccion = solicitarProximaInstruccionAUMC();
		limpiarInstruccion(proximaInstruccion);
		if (pcb->PC >= (pcb->codigo - 1) && (strcmp(proximaInstruccion, "end") == 0)) {
			finalizarEjecucionPorExit();
			revisarFinalizarCPU();
			return;
		} else {

			if (proximaInstruccion != NULL) {
				if (strcmp(proximaInstruccion, "FINALIZAR") == 0) {
					log_error(ptrLog, "Instruccion no pudo leerse. Hay que finalizar el Proceso.");
					finalizarProcesoPorErrorEnUMC();
					return;
				} else {
					log_debug(ptrLog, "Instruccion recibida: %s", proximaInstruccion);
					if (strcmp(proximaInstruccion, "end") == 0) {
						log_debug(ptrLog, "Finalizo la ejecucion del programa");
						finalizarEjecucionPorExit();
						revisarFinalizarCPU();
						return;
					}
					analizadorLinea(proximaInstruccion, &functions,
							&kernel_functions);
					if (huboStackOver) {
						finalizarProcesoPorStackOverflow();
						revisarFinalizarCPU();
						return;
					}
					contador++;
					pcb->PC = (pcb->PC) + 1;
					switch (operacion) {
					case IO:
						log_debug(ptrLog, "Finalizo ejecucion por operacion I/O");
						finalizarEjecucionPorIO();
						revisarFinalizarCPU();
						return;
					case WAIT:
						log_debug(ptrLog, "Finalizo ejecucion por un Wait.");
						finalizarEjecucionPorWait();
						revisarFinalizarCPU();
						return;
					default:
						break;
					}
					usleep(pcb->quantumSleep * 1000);
				}
			} else {
				log_info(ptrLog, "No se pudo recibir la instruccion de UMC. Cierro la conexion");
				finalizarConexion(socketUMC);
				return;
			}
		}

	}
	log_debug(ptrLog, "Finalizo ejecucion por fin de Quantum");
	finalizarEjecucionPorQuantum();

	free(pcb);
	revisarFinalizarCPU();
}

void freePCB() {
	int a, b;

	if (pcb->ind_etiq != NULL) {
		free(pcb->ind_etiq);
	}

	for (a = 0; a < list_size(pcb->ind_codigo); a++) {
		t_indice_codigo * indice = list_get(pcb->ind_codigo, a);
		list_remove(pcb->ind_codigo, a);
		free(indice);
	}
	free(pcb->ind_codigo);

	for (a = 0; a < list_size(pcb->ind_stack); a++) {
		t_stack* linea = list_get(pcb->ind_stack, a);
		if (linea->argumentos != NULL) {
			uint32_t cantidadArgumentos = list_size(linea->argumentos);

			for (b = 0; b < cantidadArgumentos; b++) {
				if (linea->argumentos != NULL) {
					t_argumento *argumento = list_get(linea->argumentos, b);
					list_remove(linea->argumentos, b);
					free(argumento);
				}
			}
			free(linea->argumentos);
		}
		if (linea->variables != NULL) {
			uint32_t cant = list_size(linea->variables);

			for (b = 0; b < cant; b++) {
				t_variable *variable = list_get(linea->variables, b);
				list_remove(linea->variables, b);
				free(variable->idVariable);
				free(variable);
			}
			free(linea->variables);
		}
	}
	free(pcb->ind_stack);
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

	log_info(ptrLog, "Pido a UMC-> Pagina: %d - Start: %d - Offset: %d",
			paginaAPedir, requestStart - (tamanioPagina * paginaAPedir),
			solicitarBytes->offset);

	t_buffer_tamanio * buffer_tamanio = serializarSolicitarBytes(
			solicitarBytes);

	int enviarBytes = enviarDatos(socketUMC, buffer_tamanio->buffer,
			buffer_tamanio->tamanioBuffer, LEER, CPU);
	free(buffer_tamanio->buffer);
	free(buffer_tamanio);
	if (enviarBytes <= 0) {
		log_error(ptrLog, "Ocurrio un error al enviar una solicitud de instruccion a CPU");
		return NULL;
	} else {
		uint32_t op, id;
		char* instruccionRecibida = recibirDatos(socketUMC, &op, &id);
		if (strcmp(instruccionRecibida, "ERROR") == 0) {
			return NULL;
		} else {
			t_instruccion * instruccion = deserializarInstruccion(
					instruccionRecibida);
			free(instruccionRecibida);
			return instruccion->instruccion;
		}
	}
}
