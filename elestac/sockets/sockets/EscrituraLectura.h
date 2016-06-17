/*
 * socketsEscrituraLectura.h
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */

#ifndef LIBRERIAS_SOCKETSESCRITURALECTURA_H_
#define LIBRERIAS_SOCKETSESCRITURALECTURA_H_
#include <sys/socket.h>
#include <stdint.h>
//int recibirDatos(int socket, char** datos, char* tamañoOcupa);

typedef enum {
	CONSOLA = 1, CPU = 2, NUCLEO = 3, SWAP = 4, UMC = 5
} quien_conecto;

typedef enum {
	QUANTUM,
	EXIT,
	IMPRIMIR_VALOR,
	IMPRIMIR_TEXTO,
	LEER_VAR_COMPARTIDA,
	ASIG_VAR_COMPARTIDA,
	WAIT,
	SIGNAL,
	SIGUSR,
	IO,
	FINALIZO_POR_ERROR_UMC,
	STACKOVERFLOW,
} cpu_ops;
typedef enum {
	ENVIAR_TAMANIO_PAGINA_A_CPU,
	ENVIAR_INSTRUCCION_A_CPU,
	NUEVOPROGRAMA,
	LEER,
	LEER_VALOR_VARIABLE,
	ESCRIBIR,
	FINALIZARPROGRAMA,
	HANDSHAKE,
	CAMBIOPROCESOACTIVO
} umc_ops;
typedef enum {
	EXECUTE_PCB,
	VAR_COMPARTIDA_ASIGNADA,
	VALOR_VAR_COMPARTIDA,
	SIGNAL_SEMAFORO,
	TAMANIO_STACK_PARA_CPU,
	TAMANIO_PAGINAS_NUCLEO
} nucleo_ops;
typedef enum {
	ERROR, NOTHING, SUCCESS
} opciones_generales_ops;
typedef enum {
	ENVIAR_TAMANIO_PAGINA_A_UMC,
	ENVIAR_INSTRUCCION_A_UMC,
	ESCRITURA_OK_UMC,
	ACEPTAR_PROCESO_A_UMC,
	RECHAZAR_PROCESO_A_UMC,
	RECHAZAR_FINALIZAR_PROCESO_A_UMC,
	RECHAZAR_LEER_PROCESO_A_UMC,
	RECHAZAR_ESCRIBIR_PROCESO_A_UMC,
	ENVIAR_PAGINA_A_UMC
} swap_ops;

typedef struct {
	int quienConecto;
	int operacion;
	int longitudPayload;
} t_header;

typedef struct {
	t_header* header;
	char* payload;
} t_paquete;

//int escribir(int socket, int id, int longitud, int operacion, void* payload);
//int leer(int socket, int* id, char** payload);

char* recibirDatos(int socket, uint32_t ** op, uint32_t ** id);
int enviarDatos(int socket, char* datos, uint32_t tamanio, uint32_t op, uint32_t id);
int leer(int socket, char* buffer, int longitud);
int escribir(int socket, char* buffer, int longitud);
int finalizarConexion(int socket);

#endif /* LIBRERIAS_SOCKETSESCRITURALECTURA_H_ */
