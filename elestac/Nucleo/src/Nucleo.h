/*
 * Nucleo.h
 *
 *  Created on: 19/5/2016
 *      Author: utnso
 */

#ifndef SRC_NUCLEO_H_
#define SRC_NUCLEO_H_
#include <pthread.h>

uint32_t nroProg = 1;
int cpuAcerrar = 0;
int cpuConProgramaACerrar = 0;
char *ipUMC;
int *listaCpus;
int puertoConexionUMC;
//Variables de configuracion para establecer Conexiones
int puertoReceptorCPU, puertoReceptorConsola;
//Variables propias del Nucleo
int quantum, quantumSleep;

//Variables necesarias, vienen de UMC
uint32_t tamanioStack;
uint32_t tamanioMarcos;

//Para manejo del select
fd_set sockets, tempSockets;

//Sockets para comunicacion
int socketReceptorCPU, socketReceptorConsola, socketUMC;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_exit = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_pid_counter = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cpu = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaPCBEjecutando = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexVariablesCompartidas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSemaforos = PTHREAD_MUTEX_INITIALIZER;

pthread_t hiloCpuOciosa;
pthread_t threadSocket;
pthread_t hiloIO;
pthread_t threadExit;
pthread_t hiloPcbFinalizarError;
sem_t semProgramaEstabaEjecutando;
sem_t semLiberoPrograma;
sem_t semCpuOciosa;
sem_t semNuevoPcbColaReady;
sem_t semNuevoProg;
sem_t semProgExit;
sem_t semProgramaFinaliza;

t_list *listaDispositivosIO, *listaSemaforos, *listaVariablesCompartidas;
t_list* listaSocketsConsola;
t_list* listaSocketsCPUs;

t_list* colaNew;
t_list* colaBloqueados;
t_list* colaExecute;
t_queue* colaExit;
t_queue* colaReady;

uint32_t pcbAFinalizar; // pcb q vamos a cerrar porque el programa cerro mal

t_pcb* crearPCB(char* programa, int socket);
void envioPCBaClienteOcioso(t_clienteCpu *clienteSeleccionado, t_pcb * unPCB);
void operacionesConSemaforos(uint32_t operacion, char* buffer, t_clienteCpu *unCliente);
void comprobarMensajesDeClientes(t_clienteCpu *unCliente, int socketFor, uint32_t operacion, char * buffer);
t_socket_pid * buscarConsolaPorProceso(uint32_t pid);
int indiceConsolaEnLista(uint32_t pid);
void finalizarProgramaPorErrorEnUMC(t_clienteCpu* unCliente, char* buffer);
void mensajesPrograma(uint32_t pcbId, uint32_t tipoDeValor, char * mensaje);
void borrarPCBDeColaExecute(uint32_t pcbId);
void borrarPCBDeColaExecuteYMeterEnColaExit(uint32_t pcbId);

#endif /* SRC_NUCLEO_H_ */

