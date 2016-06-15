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
int crearSocketCliente(char* direccion, int puerto);
int controlarConexiones();
int manejarPrimeraConexionConUMC();

void manejarMensajeRecibido(uint32_t id, uint32_t operacion, char *mensaje);
void manejarMensajeRecibidoNucleo(uint32_t operacion, char *mensaje);
void manejarMensajeRecibidoUMC(uint32_t operacion, char *mensaje);
void revisarFinalizarCPU();
void recibirPCB(char *mensaje);
void recibirTamanioStack(char *mensaje);
void recibirTamanioPagina(char *mensaje);
void recibirSignalSemaforo(char *mensaje);
void recibirAsignacionVariableCompartida(char *mensaje);
void recibirValorVariableCompartida(char *mensaje);
void recibirInstruccion(char *mensaje);
void limpiarInstruccion(char * instruccion);
void comenzarEjecucionDePrograma();
void limpiarInstruccion(char * instruccion);
char* solicitarProximaInstruccionAUMC();
void finalizarEjecucionPorExit();
void finalizarEjecucionPorQuantum();
void notificarAUMCElCambioDeProceso(uint32_t pid);
void freePCB();

void finalizarProcesoPorErrorEnUMC();

t_list * crearRequestsParaUMC();

int recibirMensaje(int socket);

#endif /* SRC_PROCESOCPU_H_ */
