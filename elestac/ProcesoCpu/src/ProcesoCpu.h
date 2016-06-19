/*
 * ProcesoCpu.h
 *
 *  Created on: 17/5/2016
 *      Author: utnso
 */

#ifndef SRC_PROCESOCPU_H_
#define SRC_PROCESOCPU_H_
/*
 * procesoCpu.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */
#include <commons/collections/list.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>

uint32_t tamanioStack;
uint32_t tamanioPagina;
t_log* ptrLog;
t_config* config;
uint32_t operacion;
int socketNucleo, socketUMC;

char* filtrarInstruccion(char* instr, int request);
char* solicitarProximaInstruccionAUMC();

int controlarConexiones();
int crearLog();
int crearSocketCliente(char* direccion, int puerto);
int manejarPrimeraConexionConUMC();
int recibirMensaje(int socket);

t_list * crearRequestsParaUMC();

void comenzarEjecucionDePrograma();
void finalizarEjecucionPorExit();
void finalizarEjecucionPorIO();
void finalizarEjecucionPorQuantum();
void finalizarEjecucionPorWait();
void finalizarProcesoPorErrorEnUMC();
void finalizarProcesoPorStackOverflow();
void freePCB();
void limpiarInstruccion(char * instruccion);
void manejarMensajeRecibido(uint32_t id, uint32_t operacion, char *mensaje);
void manejarMensajeRecibidoNucleo(uint32_t operacion, char *mensaje);
void manejarMensajeRecibidoUMC(uint32_t operacion, char *mensaje);
void notificarAUMCElCambioDeProceso(uint32_t pid);
void recibirAsignacionVariableCompartida(char *mensaje);
void recibirInstruccion(char *mensaje);
void recibirPCB(char *mensaje);
void recibirSignalSemaforo(char *mensaje);
void recibirTamanioPagina(char *mensaje);
void recibirTamanioStack(char *mensaje);
void recibirValorVariableCompartida(char *mensaje);
void revisarFinalizarCPU();
void revisarSigusR1(int signo);


#endif /* SRC_PROCESOCPU_H_ */
