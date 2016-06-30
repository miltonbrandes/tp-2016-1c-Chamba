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

int abrirSocketsImportantes();
int crearLog();
int crearThreadsUtiles();
int datosEnSocketUMC();
int indiceConsolaEnLista(uint32_t pid);
int iniciarNucleo();
int init();
int initInotify();
int obtenerSocketMaximoInicial();

t_IO* BuscarIO(t_dispositivo_io* element);
t_pcb* crearPCB(char* programa, int socket);
t_socket_pid * buscarConsolaPorProceso(uint32_t pid);
t_variable_compartida* obtenerVariable(t_op_varCompartida * varCompartida);
t_variable_compartida* obtenerVariable2(char* buffer);

void *hiloClienteOcioso();
void *hiloPorIO(void* es);
void aceptarConexionEnSocketReceptorConsola(int socketConexion);
void aceptarConexionEnSocketReceptorCPU(int nuevoSocketConexion);
void borrarPCBDeColaExecute(uint32_t pcbId);
void borrarPCBDeColaExecuteYMeterEnColaExit(uint32_t pcbId);
void cerrarConexionCliente(t_clienteCpu *unCliente);
void comprobarMensajesDeClientes(t_clienteCpu *unCliente, int socketFor, uint32_t operacion, char * buffer);
void crearListaDispositivosIO(char **ioIds, char **ioSleepValues);
void crearListaSemaforos(char **semIds, char **semInitValues);
void crearListaVariablesCompartidas(char **sharedVars);
void envioPCBaClienteOcioso(t_clienteCpu *clienteSeleccionado, t_pcb * unPCB);
void escucharPuertos();
void finalizarConexionDeUnSocketEnParticular(int socketFor);
void finalizarProgramaPorErrorEnUMC(t_clienteCpu* unCliente, char* buffer);
void finalizarProgramaPorStackOverflow(t_clienteCpu* unCliente, char* buffer);
void finHilosIo(void* entradaSalida);
void hilosIO(void* entradaSalida);
void mensajesPrograma(uint32_t pcbId, uint32_t tipoDeValor, char * mensaje);
void operacionesConSemaforos(uint32_t operacion, char* buffer, t_clienteCpu *unCliente);
void operacionesConVariablesCompartidas(char operacion, char *buffer, uint32_t socketCliente);
void operacionEXIT(t_clienteCpu* unCliente, char* buffer);
void operacionIO(t_clienteCpu* unCliente, char* buffer);
void operacionQuantum(t_clienteCpu* unCliente, char* buffer);
void recibirDatosDeSocketCPU(char * buffer, int socketFor, uint32_t operacion);
void* hiloPCBaFinalizar();
void* vaciarColaExit();


#endif /* SRC_NUCLEO_H_ */
