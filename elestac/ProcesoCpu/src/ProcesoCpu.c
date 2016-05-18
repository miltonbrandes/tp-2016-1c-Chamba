/*
 * procesoCpu.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */
#include <stdlib.h>
#include <stdio.h>
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
	//leo del archivo de configuracion el puerto y el ip

	char *direccionNucleo = config_get_string_value(config, "IP_NUCLEO");
	log_info(ptrLog, direccionNucleo);
	int puertoNucleo = config_get_int_value(config, "PUERTO_NUCLEO");
	log_info(ptrLog, "%d", puertoNucleo);
	socketNucleo = crearSocketCliente(direccionNucleo, puertoNucleo);

	char *direccionUmc = config_get_string_value(config, "IP_UMC");
	int puertoUmc = config_get_int_value(config, "PUERTO_UMC");
	socketUMC = crearSocketCliente(direccionUmc, puertoUmc);

	return controlarConexiones();
}

int controlarConexiones() {
	while (1) {
		if (recibirMensaje(socketNucleo) == 0) {

		}else{
			return 0;
		}
	}
	return 0;

}

int crearSocketCliente(char* direccion, int puerto) {

	int socketConexion;
	socketConexion = AbrirConexion(direccion, puerto);
	if (socketConexion < 0) {
		//aca me deberia mostrar por log que hubo un error
		log_info(ptrLog, "Error en la conexion con el servidor");
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
		//error, no pudo escribir
		log_info(ptrLog, "Error al escribir");
		return -1;
	}
	log_info(ptrLog, "Mensaje Enviado");
	return 0;
}

int recibirMensaje(int socket) {
	char *respuestaServidor;
	uint32_t id;
	uint32_t operacion;

	int bytesRecibidos = recibirDatos(socket, &respuestaServidor, &operacion, &id);

	if (bytesRecibidos < 0) {
		log_info(ptrLog, "Error en al lectura del mensaje del servidor");
		return -1;

	} else if (bytesRecibidos == 0) {

		if(socket == socketNucleo) {
			log_info(ptrLog, "No se recibio nada de Nucleo, cierro conexion");
			finalizarConexion(socketNucleo);
		}else
		if(socket == socketUMC) {
			log_info(ptrLog, "No se recibio nada de UMC, cierro conexion");
			finalizarConexion(socketUMC);
		}
		return -1;

	} else {
		log_info(ptrLog, "Servidor dice: %s", respuestaServidor);

		manejarMensajeRecibido(id, operacion, &respuestaServidor);
	}

	return 0;
}

//Veo quien me mando mensajes
void manejarMensajeRecibido(uint32_t id, uint32_t operacion, char **mensaje) {
	switch(id) {
	case NUCLEO:
		manejarMensajeRecibidoNucleo(operacion, mensaje);
		break;
	case UMC:
		manejarMensajeRecibidoUMC(operacion, mensaje);
		break;
	}
}

void manejarMensajeRecibidoNucleo(uint32_t operacion, char **mensaje) {
	switch(operacion) {
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

void manejarMensajeRecibidoUMC(uint32_t operacion, char **mensaje) {
	switch(operacion) {
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
void recibirPCB(char **mensaje) {
	pcb = deserializar_pcb(mensaje);
	comenzarEjecucionDePrograma();
}

void recibirQuantum(char **mensaje) {
	t_EstructuraInicial *estructuraInicial = deserializar_EstructuraInicial(mensaje);
	quantum = estructuraInicial->Quantum;
	quantumSleep = estructuraInicial->RetardoQuantum;
}

void recibirTamanioPagina(char **mensaje) {
	tamanioPagina = deserializarUint32((void**)mensaje);
}

void recibirValorVariableCompartida(char **mensaje) {

}

void recibirAsignacionVariableCompartida(char **mensaje) {

}

void recibirSignalSemaforo(char **mensaje) {

}

void recibirInstruccion(char **mensaje) {

}
//Fin manejo de mensajes recibidos

void comenzarEjecucionDePrograma() {
	int contador = 1;
	while(contador <= quantum) {
		char* proximaInstruccion = solicitarProximaInstruccionAUMC();
		analizadorLinea(proximaInstruccion, &functions, &kernel_functions);
		sleep(quantumSleep);
	}
}

char* solicitarProximaInstruccionAUMC() {
	void *message;
	uint32_t operation, id;

	int bytesReceived = recibirDatos(socketUMC, &message, &operation, &id);
	if(bytesReceived < 0) {

	}else if(bytesReceived == 0) {

	}else{

	}

	return (char*) message;
}
