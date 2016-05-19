/*
 * Nucleo.h
 *
 *  Created on: 19/5/2016
 *      Author: utnso
 */

#ifndef SRC_NUCLEO_H_
#define SRC_NUCLEO_H_
#include <pthread.h>
uint32_t nroProg = 0;
char *ipUMC;
fd_set sockets, tempSockets; //descriptores
int *listaCpus;
int cpuAcerrar = 0;
int i = 0;
int puertoConexionUMC;
//Variables de configuracion para establecer Conexiones
int puertoReceptorCPU, puertoReceptorConsola;
//Variables propias del Nucleo
int quantum, quantumSleep;
//Socket que recibe conexiones de CPU y Consola
int socketReceptorCPU, socketReceptorConsola;
int socketUMC;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_exit = PTHREAD_MUTEX_INITIALIZER;
pthread_t hiloCpuOciosa;
pthread_t threadPlanificador;
pthread_t threadSocket;
pthread_t hiloIO;
pthread_t threadExit;
pthread_t hiloPcbFinalizarError;
sem_t semCpuOciosa;
sem_t semMensajeImpresion;
sem_t semNuevoPcbColaReady;
sem_t semNuevoProg;
sem_t semProgExit;
sem_t semProgramaFinaliza;
t_config* config;
t_EstructuraInicial *estructuraInicial;
t_list *listaDispositivosIO,
*listaSemaforos, *listaVariablesCompartidas;
t_list* colaNew;
t_list* listaIndCodigo;
t_list* listaSocketsConsola;
t_list* listaSocketsCPUs;
t_list* pcbStack;
//Archivo de Log

t_pcb* crearPCB(char* programa, int socket);
t_list* colaBloqueados;
t_queue* colaExit;
t_queue* colaImprimibles;
t_queue* colaReady;
uint32_t handshakeumv = 0;
uint32_t pcbAFinalizar;
uint32_t pcbAFinalizar; // pcb q vamos a cerrar porque el programa cerro mal
uint32_t tamanioStack;
uint32_t tamanioMarcos = 4;
void envioPCBaClienteOcioso(t_clienteCpu *clienteSeleccionado);
void operacionesConSemaforos(char operacion, char* buffer,
		t_clienteCpu *unCliente);


#endif /* SRC_NUCLEO_H_ */
