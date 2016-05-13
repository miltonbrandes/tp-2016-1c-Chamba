

/*
 * UMC.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <sockets/ServidorFunciones.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>
#include <pthread.h>
#include <semaphore.h>
enum UMC{
	NUEVOPROGRAMA = 1,
	SOLICITARBYTES,
	ALMACENARBYTES,
	FINALIZARPROGRAMA,
	HANDSHAKE,
	CAMBIOPROCESOACTIVO
};

typedef struct package_iniciar_programa {
	uint32_t programID;
	uint32_t tamanio;
} t_iniciar_programa;

typedef struct _package_cambio_proc_activo {
	uint32_t programID;
} t_cambio_proc_activo;

typedef struct _package_enviarBytes {
	uint32_t base;
	uint32_t offset;
	uint32_t tamanio;
	char* buffer;
} t_enviarBytes;
//PARA INCLUIR EN LA LIBRERIA
typedef struct tlb_t { //Registros de la tlb
	int indice; //Cada vez que se asigna una se le da un indice, el indice menor es el que se saca. (-1 si esta libre)
	int pid;
	int nPag;
	int numMarco;
} tlb;

typedef struct tablaPag_t{
	int valido; //0 si no esta en memoria, 1 si si esta
	int numMarco;//marco en el que se encuentra si valido vale 1
} tablaPag;

typedef struct tMarco_t { //Registro de la tabla de marcos
	int pid;
	int nPag;
	int indice; //-1 si esta libre
	int modif; // 0 no, 1 si
} tMarco;

typedef struct nodoListaTP_t{ //Nodos de la lista de tablas de paginas del adm
	struct nodoListaTP_t* ant;
	struct nodoListaTP_t* sgte;
	int pid;
	int cantPaginas;
	int marcosAsignados; //Chequear si es menor al tamanio de marcos maximo.
	int cantPaginasAcc;
	int cantFallosPag;
	int indiceClockM;
	tablaPag* tabla; //aca va la tabla en si malloc(sizeof(tablaPag)*cantPaginas)
} nodoListaTP;

//TODO: falta que el umc reciba cuando hay un programa nuevo de cpu!!!!
//casi finalizado el circuito
#define MAX_BUFFER_SIZE 4096

//Socket que recibe conexiones de Nucleo y CPU
int socketReceptorNucleo;
int socketReceptorCPU;
int socketSwap;

//Archivo de Log
t_log* ptrLog;

//Variables de configuracion
int puertoTCPRecibirConexionesCPU;
int puertoTCPRecibirConexionesNucleo;
int puertoReceptorSwap;
char *ipSwap;
int *listaCpus;
int i = 0;

//Variables tlb, hilos y semáforos
int aciertosTLB;
int fallosTLB;
int indiceTLB;
int indiceMarcos;
int indiceClockM; //es el indice de lectura actual en clock m para reemplazo de paginas
int zonaCritica;
int flushMemoria;
char* memoria;
nodoListaTP* raizTP;
tlb* TLB;
tMarco* tMarcos;
pthread_t hMPFlush;
pthread_mutex_t MUTEXTLB;
pthread_mutex_t MUTEXTM;
pthread_mutex_t MUTEXLP;
pthread_mutex_t MUTEXLOG;
int marcos, marcosSize, marcoXProc, entradasTLB, retardo, tlbHabilitada;

//Metodos para Iniciar valores de la UMC
int crearLog() {
	ptrLog = log_create(getenv("UMC_LOG"), "UMC", 1, 0);
	if (ptrLog) {
		return 1;
	} else {
		return 0;
	}
}

int iniciarUMC(t_config* config) {
	//int marcos, marcosSize, marcoXProc, entradasTLB, retardo, tlbHabilitada;

	if (config_has_property(config, "MARCOS")) {
		marcos = config_get_int_value(config, "MARCOS");
	} else {
		log_info(ptrLog,
				"El archivo de configuracion no contiene la clave MARCOS");
		return 0;
	}

	if (config_has_property(config, "MARCOS_SIZE")) {
		marcosSize = config_get_int_value(config, "MARCOS_SIZE");
	} else {
		log_info(ptrLog,
				"El archivo de configuracion no contiene la clave MARCOS_SIZE");
		return 0;
	}

	if (config_has_property(config, "MARCO_X_PROC")) {
		marcoXProc = config_get_int_value(config, "MARCO_X_PROC");
	} else {
		log_info(ptrLog,
				"El archivo de configuracion no contiene la clave MARCO_X_PROC");
		return 0;
	}

	if (config_has_property(config, "ENTRADAS_TLB")) {
		entradasTLB = config_get_int_value(config, "ENTRADAS_TLB");
	} else {
		log_info(ptrLog,
				"El archivo de configuracion no contiene la clave ENTRADAS_TLB");
		return 0;
	}

	if (config_has_property(config, "RETARDO")) {
		retardo = config_get_int_value(config, "RETARDO");
	} else {
		log_info(ptrLog,
				"El archivo de configuracion no contiene la clave RETARDO");
		return 0;
	}

	if (config_has_property(config, "TLB_HABILITADA")) {
		tlbHabilitada = config_get_int_value(config, "TLB_HABILITADA");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave TLB_HABILITADA");
			return 0;
		}

	return 1;
}

int cargarValoresDeConfig() {
	t_config* config;
	config = config_create(getenv("UMC_CONFIG"));

	if (config) {
		if (config_has_property(config, "PUERTO_CPU")) {
			puertoTCPRecibirConexionesCPU = config_get_int_value(config, "PUERTO_CPU");
		} else {
			log_info(ptrLog, "El archivo de configuracion no contiene la clave PUERTO_CPU");
			return 0;
		}

		if (config_has_property(config, "PUERTO_NUCLE0")) {
			puertoTCPRecibirConexionesNucleo = config_get_int_value(config, "PUERTO_NUCLE0");
		} else {
			log_info(ptrLog, "El archivo de configuracion no contiene la clave PUERTO_NUCLE0");
			return 0;
		}

		if (config_has_property(config, "IP_SWAP")) {
			ipSwap = config_get_string_value(config, "IP_SWAP");
		} else {
			log_info(ptrLog, "El archivo de configuracion no contiene la clave IP_SWAP");
			return 0;
		}

		if (config_has_property(config, "PUERTO_SWAP")) {
			puertoReceptorSwap = config_get_int_value(config, "PUERTO_SWAP");
		} else {
			log_info(ptrLog, "El archivo de configuracion no contiene la clave PUERTO_SWAP");
			return 0;
		}

		return iniciarUMC(config);
	} else {
		return 0;
	}
}

int init() {
	if (crearLog()) {
		return cargarValoresDeConfig();
	} else {
		return 0;
	}
}
//Fin Metodos para Iniciar valores de la UMC

void enviarMensajeASwap(char *mensajeSwap) {
	log_info(ptrLog, "Envio mensaje a Swap: %s", mensajeSwap);
	uint32_t id= 5;
	uint32_t longitud = strlen(mensajeSwap);
	uint32_t operacion = 1;
	int sendBytes = enviarDatos(socketSwap, mensajeSwap, longitud, operacion, id);
}

void enviarMensajeACPU(int socketCPU, char* buffer) {
	char *mensajeCpu = "Le contesto a Cpu, soy UMC\0";
	log_info(ptrLog, "Envio mensaje a CPU: %s", mensajeCpu);
	uint32_t id= 5;
	uint32_t longitud = strlen(mensajeCpu);
	uint32_t operacion = 1;
	int sendBytes = enviarDatos(socketCPU, mensajeCpu, longitud, operacion, id);
}

void enviarMensajeANucleo(int socketNucleo, char* buffer) {
	char *mensajeNucleo = "Le contesto a Nucleo, soy UMC\0";
	log_info(ptrLog, "Envio mensaje a Nucleo: %s", mensajeNucleo);
	uint32_t id= 5;
	uint32_t longitud = strlen(mensajeNucleo);
	uint32_t operacion = 1;
	int sendBytes = enviarDatos(socketNucleo, buffer, longitud, operacion, id);
}
void datosEnSocketReceptorNucleoCPU(int socketNuevaConexion) {
	char *buffer;
	uint32_t id;
	uint32_t operacion;
	int bytesRecibidos = recibirDatos(socketNuevaConexion, &buffer, &operacion, &id);

	if(bytesRecibidos < 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos en un Socket Nucleo o CPU");
	} else if(bytesRecibidos == 0) {
		log_info(ptrLog, "No se recibieron datos en el Socket Nucleo o CPU");
	} else {
		log_info(ptrLog, "Bytes recibidos desde Nucleo o CPU: %s", buffer);
		enviarMensajeASwap("Necesito que me reserves paginas, Soy la umc");
	}
}

int datosEnSocketSwap() {
	char* buffer;
	uint32_t id;
	uint32_t operacion;
	int bytesRecibidos = recibirDatos(socketSwap, &buffer, &operacion, &id);

	if(bytesRecibidos < 0) {
		log_info(ptrLog, "Ocurrio un error al recibir datos de Swap");
		return -1;
	} else if(bytesRecibidos == 0) {
		log_info(ptrLog, "No se recibieron datos de Swap. Se cierra la conexion");
		finalizarConexion(socketSwap);
		return -1;
	} else {
		log_info(ptrLog, "Bytes recibidos desde Swap: %s", buffer);
	}

	return 0;
}

int obtenerSocketMaximoInicial() {
	int socketMaximoInicial = 0;

	if(socketReceptorCPU > socketReceptorNucleo) {
		if(socketReceptorCPU > socketSwap) {
			socketMaximoInicial = socketReceptorCPU;
		}else if(socketSwap > socketReceptorNucleo) {
			socketMaximoInicial = socketSwap;
		}
	}else if(socketReceptorNucleo > socketSwap) {
		socketMaximoInicial = socketReceptorNucleo;
	} else {
		socketMaximoInicial = socketSwap;
	}

	return socketMaximoInicial;
}

void manejarConexionesRecibidas() {
	//int handshakeNucleo = 0;
	fd_set sockets, tempSockets;
	//int resultadoSelect;
	int socketMaximo = obtenerSocketMaximoInicial();

	int socketCPUPosta = -121211;

	FD_SET(socketReceptorNucleo, &sockets);
	FD_SET(socketReceptorCPU, &sockets);
	FD_SET(socketSwap, &sockets);

	while(1) {
		FD_ZERO(&tempSockets);
		log_info(ptrLog, "Esperando conexiones");
		tempSockets = sockets;
		int resultadoSelect = select(socketMaximo+1, &tempSockets, NULL, NULL, NULL);
		if (resultadoSelect == 0) {
			log_info(ptrLog, "Time out. Volviendo a esperar conexiones");
		} else if (resultadoSelect < 0) {
			log_info(ptrLog, "Ocurrio un error");
		} else {

			if (FD_ISSET(socketReceptorNucleo, &tempSockets)) {

				int nuevoSocketConexion = AceptarConexionCliente(socketReceptorNucleo);
				if (nuevoSocketConexion < 0) {
					log_info(ptrLog, "Ocurrio un error al intentar aceptar una conexion de CPU o Nucleo");
				} else {
					FD_SET(nuevoSocketConexion, &sockets);
					FD_SET(nuevoSocketConexion, &tempSockets);
					socketMaximo = (socketMaximo < nuevoSocketConexion) ? nuevoSocketConexion : socketMaximo;
				}
				//datosEnSocketReceptorNucleoCPU(nuevoSocketConexion);
				FD_CLR(socketReceptorNucleo, &tempSockets);
				enviarMensajeANucleo(nuevoSocketConexion, "Nucleo no me rompas las pelotas\0");
				//me fijo si el mensaje no es el handshake y si no es se lo tengo que pasar al swap


			} else if (FD_ISSET(socketReceptorCPU, &tempSockets)) {

				int nuevoSocketConexion = AceptarConexionCliente(socketReceptorCPU);
				if (nuevoSocketConexion < 0) {
					log_info(ptrLog, "Ocurrio un error al intentar aceptar una conexion de CPU");
				} else {
					FD_SET(nuevoSocketConexion, &sockets);
					FD_SET(nuevoSocketConexion, &tempSockets);
					socketMaximo = (socketMaximo < nuevoSocketConexion) ? nuevoSocketConexion : socketMaximo;
				}
				//aca deberia ir un mensaje hacia swap diciendo que cpu me esta pidiendo espacio, = que con el nucleo usar una lista para cpus!!!
				//agrego las cpus a una nueva lista
				listaCpus[i] = nuevoSocketConexion;
				socketCPUPosta = nuevoSocketConexion;
				i++;
				FD_CLR(socketReceptorCPU, &tempSockets);
				enviarMensajeACPU(nuevoSocketConexion, "CPU no me rompas las pelotas\0");

			}
			else if(FD_ISSET(socketSwap, &tempSockets)) {

				//Ver que hacer aca, no se acepta conexion, se recibe algo de Swap.
				int returnDeSwap = datosEnSocketSwap();
				if(returnDeSwap == -1) {
					//Aca matamos Socket SWAP
					FD_CLR(socketSwap, &sockets);
				}

			} else {

				//Ver que hacer aca, se esta recibiendo algo de un socket en particular
				//recibirDatos(&tempSockets, &sockets, socketMaximo);
				char* buffer[MAX_BUFFER_SIZE];
					uint32_t id;
					uint32_t operacion;
					int bytesRecibidos;
					int socketFor;
					for (socketFor = 0; socketFor < (socketMaximo + 1); socketFor++) {
						if (FD_ISSET(socketFor, &tempSockets)) {
							bytesRecibidos = recibirDatos(socketFor,  &buffer, &operacion, &id);

							if (bytesRecibidos > 0) {
								buffer[bytesRecibidos] = 0;
								log_info(ptrLog, buffer);
								//ACA ME DEBERIA FIJAR QUIEN ES EL QUE ME LO MANDO MEDIANTE EL ID DEL PROCESO QUE ESTA GUARDADO EN EL BUFFER
								//Recibimos algo
							} else if (bytesRecibidos == 0) {
								//Aca estamos matando el Socket que esta fallando
								//Ver que hay que hacer porque puede venir de CPU o de Nucleo
								finalizarConexion(socketFor);
								FD_CLR(socketFor, &tempSockets);
								FD_CLR(socketFor, &sockets);
								log_info(ptrLog, "No se recibio ningun byte de un socket que solicito conexion.");
							}else{
								finalizarConexion(socketFor);
								FD_CLR(socketFor, &sockets);
								log_info(ptrLog, "Ocurrio un error al recibir los bytes de un socket");
							}
						}
					}
				if(socketCPUPosta>0) {
					enviarMensajeASwap("Se abrio un nuevo programa por favor reservame memoria");
				}
			}
		}
	}
}

int iniciarTablas(void){
	int i=0;
	int fallo=0;
	if(tlbHabilitada==1)	{

		TLB=malloc(sizeof(tlb)*(entradasTLB));
		if(TLB==NULL) fallo=-1; //si devuelve -1 es pq fallo
		for(i=0;i<entradasTLB;i++){
			TLB[i].pid=-1;
			TLB[i].indice=-1;
			TLB[i].nPag=0;
			TLB[i].numMarco=-1;
		}
	} else {
		TLB=NULL;
	}
	tMarcos=malloc(sizeof(tMarco)*(marcos));
	if(tMarcos!=NULL){
		//Inicia los marcos con el tamanio de cada uno.
		for(i=0;i<(marcos);i++){
			tMarcos[i].indice=-1; //Inicializamos todos los marcos como libres
			tMarcos[i].pid=-1;
		}
	} else {
		fallo=-1;
	}

	memoria=malloc((sizeof(char)*(marcosSize))*marcos);
	if(memoria==NULL) fallo =-1;
	raizTP=NULL;
	if(fallo==0) //Tablas iniciadas
	return fallo;
}

int marcosOcupadosMP(){
	int cuenta=0;
	int i;
	for(i=0;i<marcos;i++){
		if(tMarcos[i].indice!=-1) cuenta++;
	}
	return cuenta;
}

int marcosLibres(void){
	int i=0;
	int c=0;
	for(i=0;i<marcos;i++){
		if(tMarcos[i].indice==-1) c++;
	}
	return c;
}

int entradaTLBAReemplazar(void){ //Devuelve que entrada hay que reemplazar, si devuelve -1 es porqeu no hay tlb.
	int i=0;
	int posMenor=0;
	if(TLB!=NULL){
		for(i=0;i<entradasTLB;i++){
			if(TLB[i].indice==-1) return i;
			if(TLB[i].indice<=TLB[posMenor].indice) posMenor=i;
		}
		return posMenor;
	}
	return -1;
}

void tlbFlush(void){
	int i=0;
	if(tlbHabilitada==1){
		//pthread_mutex_lock(&MUTEXTLB);
	for(i=0;i<entradasTLB;i++){
		TLB[i].pid=-1;
		TLB[i].indice=-1;
		TLB[i].nPag=0;
		TLB[i].numMarco=-1;
		}
	//pthread_mutex_unlock(&MUTEXTLB);
	}
	printf("TLB BORRADA\n");
	return;
}

int estaEnTLB(int pid, int numPag){//Devuelve -1 si no esta,sino devuelve la posicion en la tlb en la que esta
	int i;
	if(TLB!=NULL && tlbHabilitada==1){
		for(i=0;i<entradasTLB;i++){
			if((TLB[i].pid)==pid && (TLB[i]).nPag==numPag){
				return i;
			}
		}
		return -1;
	}
	return -1;
}

void finalizarTablas(void){
	int i=0;

	if(TLB!=NULL) free(TLB);
	TLB=NULL;

	if(tMarcos!=NULL){
		free(tMarcos);
		tMarcos=NULL;
	}

	if(memoria!=NULL){
		free(memoria);
		memoria=NULL;
	}

	//if(raizTP!=NULL) finalizarListaTP();
	//printf("Tablas finalizadas\n");
}

void agregarATLB(int pid,int pagina,int marco){
	int aAgregar;
	aAgregar=entradaTLBAReemplazar();
	TLB[aAgregar].nPag=pagina;
	TLB[aAgregar].numMarco=marco;
	TLB[aAgregar].pid=pid;
	TLB[aAgregar].indice=indiceTLB;
	indiceTLB++;
}

int main() {
	if (init()) {
			listaCpus = (int *)malloc (10*sizeof(int));
			socketSwap = AbrirConexion(ipSwap, puertoReceptorSwap);
			if (socketSwap < 0) {
				log_info(ptrLog, "No pudo conectarse con Swap");
				return -1;
			}
			log_info(ptrLog, "Se conecto con Swap");

			socketReceptorNucleo = AbrirSocketServidor(puertoTCPRecibirConexionesNucleo);
			if (socketReceptorNucleo < 0) {
				log_info(ptrLog, "No se pudo abrir el Socket Servidor Nucleo de UMC");
				return -1;
			}
			log_info(ptrLog, "Se abrio el socket Servidor Nucleo de UMC");

			socketReceptorCPU = AbrirSocketServidor(puertoTCPRecibirConexionesCPU);
			if (socketReceptorCPU < 0) {
				log_info(ptrLog, "No se pudo abrir el Socket Servidor Nucleo de CPU");
				return -1;
			}
			log_info(ptrLog, "Se abrio el socket Servidor Nucleo de CPU");

			enviarMensajeASwap("Abrimos bien, soy la umc");
			manejarConexionesRecibidas();

		} else {
			log_info(ptrLog, "La UMC no pudo inicializarse correctamente");
			return -1;
		}

		return EXIT_SUCCESS;
}
