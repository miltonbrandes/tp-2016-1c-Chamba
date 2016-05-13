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

#define MAX_BUFFER_SIZE 4096
enum CPU {
	QUANTUM = 2,
	IO,
	EXIT,
	IMPRIMIR_VALOR,
	IMPRIMIR_TEXTO,
	LEER_VAR_COMPARTIDA,
	ASIG_VAR_COMPARTIDA,
	WAIT,
	SIGNAL,
	SIGUSR
};
t_log* ptrLog;
t_config* config;
typedef struct pcb {
	int32_t pcb_id;
	uint32_t seg_codigo;
	uint32_t seg_stack;
	uint32_t cursor_stack;
	uint32_t index_codigo;
	uint32_t index_etiq;
	uint32_t tamanio_index_etiquetas;
	uint32_t tamanio_contexto;
	uint32_t PC;
} t_pcb;
typedef struct{
	t_pcb* pcb;
	int32_t valor;
}t_solicitudes;

typedef struct estructuraInicial {
	uint32_t Quantum;
	uint32_t RetardoQuantum;
} t_EstructuraInicial;

int socketNucleo, socketUMC;

int crearLog() {
	ptrLog = log_create(getenv("CPU_LOG"), "ProcesoCpu", 1, 0);
	if (ptrLog) {
		return 1;
	} else {
		return 0;
	}
}

int crearSocketCliente(char* direccion, int puerto);

int main() {

	crearLog();

	t_config* config;
	config = config_create(getenv("CPU_CONFIG"));
	//leo del archivo de configuracion el puerto y el ip

	char *direccionNucleo = config_get_string_value(config, "IP_NUCLEO");
	int puertoNucleo = config_get_int_value(config, "PUERTO_NUCLEO");
	socketNucleo = crearSocketCliente(direccionNucleo, puertoNucleo);

	enviarMensaje(socketNucleo, "Hola nucleo, soy una nueva cpu");
	recibirMensaje(socketNucleo);

	char *direccionUmc = config_get_string_value(config, "IP_UMC");
	int puertoUmc = config_get_int_value(config, "PUERTO_UMC");
	socketUMC = crearSocketCliente(direccionUmc, puertoUmc);

	//enviarMensaje(socketUMC);
	while (1) {
		int bytesRecibidosOk = recibirMensaje(socketUMC);
		if(bytesRecibidosOk == -1) {
			//Cerramos el CPU
			log_info(ptrLog, "Se perdio conexion con UMC. Termino proceso CPU");
			break;
		}
		controlarConexiones();
	}


	return EXIT_SUCCESS;
}
int controlarConexiones()
{
	while(1)
		{
			if(recibirMensaje(socketNucleo) == 0)
			{
				//aca va lo que hace cada vez que nucleo solicita algo a la umc (cada vez que reciba un nuevo programa le tengo que pedir paginas)

				enviarMensaje(socketUMC, "Hola umc, soy una nueva cpu");
				break;
			}else if(recibirMensaje(socketUMC))
			{
				//aca deberia ir la logica de cada vez mas que reciba una conexion de umc pidiendo algo
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
	int sendBytes = enviarDatos(socket,mensaje, longitud, operacion, id);
	if (sendBytes< 0) {
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
	//Respuesta del socket servidor
	int bytesRecibidos = recibirDatos(socket, &respuestaServidor, &operacion, &id);
	if (bytesRecibidos < 0) {
		log_info(ptrLog, "Error en al lectura del mensaje del servidor");
		//no pude recibir nada del nucleo/umc
		return -1;
	}else if(bytesRecibidos == 0) {
		//Matamos socket
		if(socket == socketNucleo) {
			//Cierro conexion con Nucleo
			log_info(ptrLog, "No se recibio nada de Nucleo, cierro conexion");
			finalizarConexion(socketNucleo);
		}else if(socket == socketUMC) {
			//Cierro conexion con UMC
			log_info(ptrLog, "No se recibio nada de UMC, cierro conexion");
			finalizarConexion(socketUMC);
		}
		return -1;
	} else {
		log_info(ptrLog, "Servidor dice: %s", respuestaServidor);
	}

	return 0;
}
