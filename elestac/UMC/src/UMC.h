/*
 * UMC.h
 *
 *  Created on: 25/5/2016
 *      Author: utnso
 */

#ifndef UMC_H_
#define UMC_H_

#include <sockets/ServidorFunciones.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>
#include <sockets/StructsUtiles.h>

pthread_mutex_t comunicacionConSwap = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t accesoAFrames = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t accesoATLB = PTHREAD_MUTEX_INITIALIZER;

typedef struct{
	int indice; //Cada vez que se asigna una se le da un indice (-1 si esta libre)
	int pid;
	int numPag;
	int numFrame;
} t_tlb;

typedef struct {
	uint32_t pID;
	uint32_t posibleProximaVictima;
	t_list * paginasEnUMC;
	t_list * tablaDePaginas;
} t_tabla_de_paginas;

typedef struct {
	uint32_t paginaProceso;
	uint32_t frame;
	uint32_t modificado; //1 modificado, 0 no modificado. Para paginas de codigo es siempre 0
	uint32_t estaEnUMC; //1 esta, 0 no esta. Para ver si hay que pedir o no la pagina a SWAP.
	uint32_t bitDeReferencia; //Para Clock.
	uint32_t esStack;
} t_registro_tabla_de_paginas;

typedef struct {
	uint32_t numCpu;
	uint32_t socket;
	uint32_t procesoActivo; //inicializa en -1
	pthread_t hiloCpu;
} t_cpu;

typedef struct {
	uint32_t disponible;
	uint32_t numeroFrame;
	char * contenido;
} t_frame;

typedef struct {
	t_registro_tabla_de_paginas * registro;
	uint32_t start;
	uint32_t offset;
} t_auxiliar_registro;

char * enviarYRecibirMensajeSwap(t_buffer_tamanio * bufferTamanio, uint32_t operacion);

int buscarFrameLibre();
int cargarValoresDeConfig();
int crearLog();
int iniciarUMC(t_config* config);
int init();

t_cambio_proc_activo* deserializarCambioProcesoActivo(char * mensaje);
t_finalizar_programa* deserializarFinalizarPrograma(char * mensaje);

t_frame * actualizarFramesConClock(t_pagina_de_swap * paginaSwap, uint32_t pid, uint32_t pagina);
t_frame * actualizarFramesConClockModificado(t_pagina_de_swap * paginaSwap, uint32_t pid, uint32_t pagina);
t_frame * agregarPaginaAUMC(t_pagina_de_swap * paginaSwap, uint32_t pid, uint32_t pagina);
t_frame * desalojarFrameConClock(t_pagina_de_swap * paginaSwap, uint32_t pid, uint32_t pagina, t_tabla_de_paginas * tablaDeProceso);
t_frame * desalojarFrameConClockModificado(t_pagina_de_swap * paginaSwap, uint32_t pid, uint32_t pagina, t_tabla_de_paginas * tablaDeProceso);
t_frame * solicitarPaginaASwap(t_cpu * cpu, uint32_t pagina);

t_list * registrosABuscarParaPeticion(t_tabla_de_paginas * tablaDeProceso, uint32_t pagina, uint32_t start, uint32_t offset);
t_nuevo_prog_en_umc * inicializarProceso(t_iniciar_programa *iniciarProg);
t_registro_tabla_de_paginas * buscarPaginaEnTabla(t_tabla_de_paginas * tabla, uint32_t pagina);
t_registro_tabla_de_paginas * crearRegistroPag(int pagina, int marco,int presencia, int modificado);
t_tabla_de_paginas * buscarTablaDelProceso(uint32_t procesoId);

uint32_t checkDisponibilidadPaginas(t_iniciar_programa * iniciarProg);

void aceptarConexionCpu();
void almacenarBytesEnPagina(uint32_t pagina, uint32_t offset, uint32_t tamanio, char* buffer);
void borrarEstructurasDeProceso(uint32_t pid);
void chequearSiHayQueEscribirEnSwapLaPagina(uint32_t pid, t_registro_tabla_de_paginas * registro);
void enviarDatoACPU(t_cpu * cpu, uint32_t pagina, uint32_t start, uint32_t offset, uint32_t operacion);
void enviarMensajeASwap(char *mensajeSwap, int tamanioMensaje, int operacion);
void enviarPaginasASwap(t_iniciar_programa * iniciarProg);
void enviarTamanioPaginaACPU(int socketCPU);
void enviarTamanioPaginaANUCLEO();
void escribirDatoDeCPU(t_cpu * cpu, uint32_t pagina, uint32_t offset, uint32_t tamanio, char * buffer);
void escribirFrameEnSwap(int nroFrame, uint32_t pid, uint32_t pagina);
void finalizarPrograma(uint32_t PID);
void liberarMemoria(char * memoriaALiberar);
void manejarConexionesRecibidas();
void notificarProcesoIniciadoANucleo(t_nuevo_prog_en_umc * nuevoPrograma);
void notificarProcesoNoIniciadoANucleo();
void recibirPeticionesCpu(t_cpu * cpuEnAccion);
void recibirPeticionesNucleo();
void reservarMemoria(int cantidadMarcos, int tamanioMarco);
void solicitarBytesDePagina(uint32_t pagina, uint32_t offset, uint32_t tamanio);

//funciones TLB
int entradaTLBAReemplazarPorLRU();
int pagEstaEnTLB(int pid, int numPag);
t_tlb * obtenerYActualizarRegistroTLB(int entradaTLB);
void agregarATLB(int pid, int pagina, int frame, char * contenidoFrame);
void finalizarTLB();
void iniciarTLB();
void tlbFlushDeUnPID(int PID);

//funciones consola
int punteroCharEsInt(char* cadena);
void crearConsola();
void dump(uint32_t pid);
void dumpDeUnPID(uint32_t pid);
void escribirEnArchivo(uint32_t pid, uint32_t paginaProceso, uint32_t frame, uint32_t presencia, uint32_t modificado,char* contenido);
void flushMemory(uint32_t pid);
void retardar(int retardoNuevo);
void tlbFlush();

#endif /* UMC_H_ */
