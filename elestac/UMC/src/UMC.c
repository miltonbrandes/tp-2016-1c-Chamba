/*
 * UMC.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <netinet/in.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <sockets/ServidorFunciones.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>
#include <pthread.h>
#include <semaphore.h>

#define SLEEP 1000000

typedef struct{
 int operacion;
 int PID;
 int pagina_proceso;
 int tamanio_msj;
} t_headerCPU;

// ENTRADAS A LA TLB //
typedef struct{
 int pid;
 int pagina;
 char * direccion_fisica;
 int marco;
} t_tlb;

// STRUCT TABLA PARA CADA PROCESO QUE LLEGA //
typedef struct{
   int pag; // Contiene el numero de pagina del proceso
   char * direccion_fisica; //Contiene la direccion de memoria de la pagina que se esta referenciando
   int marco; // numero de marco (si tiene) en donde esta guardada la pagina
   int accessed;
   int dirty;
   int puntero;
}process_pag;

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
typedef struct{
	int numCpu;
	int socket;
	bool enUso;
} t_cpu;

sem_t semEsperaCPU;

//struct para conexiones

struct Conexiones {
	int socket_escucha;					// Socket de conexiones entrantes
	struct sockaddr_in direccion;
	// Datos de la direccion del servidor
	socklen_t tamanio_direccion;		// Tamaño de la direccion
	//t_cpu CPUS[miContexto.cantHilosCpus];						// Sockets de conexiones ACEPTADAS
	//hago el vector de arriba de forma dinamica
	t_cpu *CPUS; //apunta al primer elemento del vector dinamico
} conexiones;

//TODO: falta que el umc reciba cuando hay un programa nuevo de cpu!!!!

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
int marcos, marcosSize, marcoXProc, entradasTLB, retardo, tlbHabilitada, hilosCpus;

char *ipSwap;
//int *listaCpus;
int i = 0;

//Variables frames, tlb
t_list * framesOcupados;
t_list * framesVacios;

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

//	if (config_has_property(config, "CANT_HILOS_CPU")) {
//			hilosCpus = config_get_int_value(config, "CANT_HILOS_CPU");
//			} else {
//				log_info(ptrLog,
//						"El archivo de configuracion no contiene la clave CANT_HILOS_CPU");
//				return 0;
//			}

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

void *escuchar(struct Conexiones* conexion) {
		int i = 1;
		semEsperaCPU.__align = 0; // inicializa semaforo

		//conexion para el comando cpu
		conexion->CPUS[0].socket = accept(conexion->socket_escucha,
				(struct sockaddr *) &conexion->direccion,
				&conexion->tamanio_direccion);

		while (i <= hilosCpus) //hasta q recorra todos los hilos de cpus habilitados
		{
			//guarda las nuevas conexiones para acceder a ellas desde cualquier parte del codigo
			conexion->CPUS[i].socket = accept(conexion->socket_escucha,
					(struct sockaddr *) &conexion->direccion,
					&conexion->tamanio_direccion);
			if (conexion->CPUS[i].socket == -1) {
				perror("ACCEPT");	//control error
			}
			conexiones.CPUS[i].numCpu = i;
			conexion->CPUS[i].enUso = false;
			//ACA ME FIJO QUE OPERACION ES
			sem_post(&semEsperaCPU); //avisa que hay 1 CPU disponible
			//puts("NUEVO HILO ESCUCHA!\n");
			//log_info(logger, "CPU %d conectado", i);
			i++;
		}

		return NULL;
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
				conexiones.CPUS[i].socket = nuevoSocketConexion;
				conexiones.CPUS[i].numCpu = i++;
				conexiones.CPUS[i].enUso = false;
				i++;
				socketCPUPosta = nuevoSocketConexion;

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

				//alojo memoria dinamicamente en tiempo de ejecucion
				conexiones.CPUS = (t_cpu*) malloc(sizeof(t_cpu) * ((hilosCpus) + 1));
				if (conexiones.CPUS == NULL)
					puts("ERROR MALLOC 1");
				pthread_t hilo_conexiones;
				if (pthread_create(&hilo_conexiones, NULL, (void*) escuchar, &conexiones) < 0)
					perror("Error HILO ESCUCHAS!");
				conexiones.CPUS[0].enUso = true;

				puts("UMC ESPERANDO CONEXIONES....\n\n\n");
				sem_wait(&semEsperaCPU); //semaforo espera conexiones

				//Ver que hacer aca, se esta recibiendo algo de un socket en particular
				//recibirDatos(&tempSockets, &sockets, socketMaximo);
				char* buffer;
				uint32_t id;
				uint32_t operacion;
				int bytesRecibidos;
				int socketFor;
				for (socketFor = 0; socketFor < (socketMaximo + 1); socketFor++) {
					if (FD_ISSET(socketFor, &tempSockets)) {
						bytesRecibidos = recibirDatos(socketFor, &buffer,&operacion, &id);

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
						} else {
							finalizarConexion(socketFor);
							FD_CLR(socketFor, &sockets);
							log_info(ptrLog,"Ocurrio un error al recibir los bytes de un socket");
						}
					}
				}
				if (socketCPUPosta > 0) {
					enviarMensajeASwap("Se abrio un nuevo programa por favor reservame memoria");
				}
			}
		}
	}
}

char * reservarMemoria(int cantidadMarcos, int tamanioMarco){
	// calloc me la llena de \0
	char * memoria = calloc(cantidadMarcos, tamanioMarco);
	return memoria;
}

void enviarMensajeASwap(char *mensajeSwap) {
	log_info(ptrLog, "Envio mensaje a Swap: %s", mensajeSwap);
	uint32_t id= 5;
	uint32_t longitud = strlen(mensajeSwap);
	uint32_t operacion = 1;
	int sendBytes = enviarDatos(socketSwap, mensajeSwap, longitud, operacion, id);
}

//int leerDesdeTlb(int socketCPU, t_list * TLB, t_headerCPU * proc, t_list* tablaAccesos, t_list* tabla_adm) {
//	bool _numeroDePid(void * p) {
//		return (*(int *) p == proc->PID);
//	}
//	bool _numeroDePagina(void * p) {
//		return (*(int *) p == proc->pagina_proceso);
//	}
//
//	int * posicion = malloc(sizeof(int));
//	t_tlb * registro_tlb = buscarEntradaProcesoEnTlb(TLB, proc, posicion);
//
//	// SI LA ENCONTRO LA LEO Y LE ENVIO EL FLAG TODO JOYA AL CPU
//	if (registro_tlb != NULL) {
//		log_info(ptrLog,
//				"TLB HIT pagina: %d en el marco numero: %d y dice: \"%s\"",
//				registro_tlb->pagina, registro_tlb->marco,
//				registro_tlb->direccion_fisica);
//		// SEGUN ISSUE 71, SI LA ENCUENTRA EN TLB HACE UN RETARDO SOLO, CUANDO OPERA CON LA PÁGINA (LA LEE)
//		usleep(retardo * SLEEP);
//
//		upPaginasAccedidas(tablaAccesos, registro_tlb->pid);
//
//		int tamanioMsj = strlen(registro_tlb->direccion_fisica) + 1;
//
//		send(socketCPU, &tamanioMsj, sizeof(uint32_t), 0);
//		if (tamanioMsj > 0)
//			send(socketCPU, registro_tlb->direccion_fisica, tamanioMsj, 0); //MODIFICAR FUNCIONES ENVIAR Y RECIBIR
//		log_info(ptrLog, "Se informa al CPU confirmacion de lectura");
//
//		t_list * tabla_proc = obtenerTablaProceso(tabla_adm, proc->PID);
//		// SI ENCONTRO UN REGISTRO CON ESE PID
//		if (tabla_proc != NULL) {	// TRAIGO LA PAGINA BUSCADA
//			process_pag * pagina_proc = obtenerPaginaProceso(tabla_proc,
//					proc->pagina_proceso);
//			actualizoTablaProceso(tabla_proc, NULL, proc);
//		} else {
//			log_error(ptrLog, "No se encontro la tabla del proceso");
//		}
//		free(posicion);
//		return 1;
//	}
//	// SI LA TLB NO ESTA HABILITADA ENTONCES TENGO QUE VERIFICAR EN LA TABLA DE TABLAS
//	//  SI YA ESTA CARGADA EN MEMORIA O SI ESTA EN SWAP
//	free(posicion);
//	return 0;
//}
//
//void iniciarProceso(t_list* tabla_adm, t_headerCPU * proceso,
//		t_list* tablaAccesos) {
//	// PRIMERO CREO LA TABLA DEL PROCESO Y LA AGREGO A LA LISTA DE LISTAS DE PROCESOS JUNTO CON EL PID
//	t_list * lista_proceso = crearListaProceso();
//
//	list_add(tablaAccesos, versus_create(proceso->PID, 0, 0));
//
//	// AGREGO UN NODO PARA CADA PAGINA A INICIALIZAR, OBVIAMENTE APUNTANDO A NULL PORQUE NO ESTAN EN MEMORIA TODAVIA
//	int x = 0;
//
//	// MIENTRAS FALTEN PAGINAS PARA INICIAR //
//	while (x < proceso->pagina_proceso) {
//		list_add(lista_proceso, pag_proc_create(x, NULL, -1, 0, 0, 0));
//		//cuando la creo el marco lo pongo en -1
//		x++;
//	}
//
//	list_add(tabla_adm, tabla_adm_create(proceso->PID, lista_proceso));
//}
//
//int leerEnMemReal(t_list * tabla_adm, t_list * TLB, t_headerCPU * package,
//		int serverSocket, int socketCliente, t_list* tablaAccesos) {
//	int * flag = malloc(sizeof(uint32_t));
//	usleep(retardo * SLEEP); // SLEEP PORQUE LA MEMORIA BUSCA EN SUS ESTRUCTURAS
//	t_list * tabla_proc = obtenerTablaProceso(tabla_adm, package->PID);
//
//	// SI ENCONTRO UN REGISTRO CON ESE PID
//	if (tabla_proc != NULL) {	// TRAIGO LA PAGINA BUSCADA
//		process_pag * pagina_proc = obtenerPaginaProceso(tabla_proc,package->pagina_proceso);
//		// SI LA DIRECCION ES NULL ES PORQUE ESTA EN SWAP, SINO YA LA ENCONTRE EN MEMORIA
//		if (pagina_proc->direccion_fisica == NULL) {
//			log_info(ptrLog,
//					"Se encontro la pagina para leer en swap, se hace el pedido de lectura");
//			if (marcosProcesoLlenos(tabla_proc)) {
//				int verific = swapeando(tabla_proc, tabla_adm, TLB, NULL,
//						serverSocket, package, tablaAccesos, socketCliente);
//			}
//			/* SI TENGO ESPACIO PARA TRAERLA (CANT MAX DE MARCOS PARA ESE PROCESO
//			 *NO FUE ALCANZADA TODAVÍA), SI ME QUEDA MEMORIA (MARCOS) LA TRAIGO(MENTIRA)
//			 */
//			else {
//				if (framesVacios->elements_count != 0) {
//					char * contenido = malloc(marcosSize);
//					envioAlSwap(package, serverSocket, contenido, flag);
//					//SI TODO SALIO BIEN, EL SWAP CARGO LA PAGINA A LEER EN "CONTENIDO"
//					if (*flag) {
//						asignarMarcosYTablas(contenido, package, tabla_proc,
//								TLB);
//						upPaginasAccedidas(tablaAccesos, package->PID);
//
//						//log_info(logger, "Se hizo conexion con swap, se envio paquete a leer y este fue recibido correctamente");
//
//						// Como la transferencia con el swap fue exitosa, le envio la pagina al CPU
//						int tamanioMsj = strlen(contenido) + 1;
//						send(socketCliente, &tamanioMsj, sizeof(int), 0);
//						if (tamanioMsj > 0)
//							send(socketCliente, contenido, tamanioMsj, 0);
//						log_info(ptrLog,
//								"Se informa al CPU confirmacion de lectura");
//
//						upFallosPagina(tablaAccesos, package->PID);
//					} else {
//						int recibi = -1;
//						send(socketCliente, &recibi, sizeof(int), 0);
//						log_error(ptrLog,
//								"Hubo un problema con la conexion/envio al swap. Se informa al CPU");
//					}
//					free(contenido);
//				} else {
//					mostrarVersus(tablaAccesos, package->PID);
//					matarProceso(package, tabla_adm, TLB, tablaAccesos);
//					int recibi = -1;
//					send(socketCliente, &recibi, sizeof(int), 0);
//					log_info(ptrLog,
//							"Ya no tengo mas marcos disponibles en la memoria, rechazo pedido e informo al CPU");
//				}
//
//			}
//		} else // SI NO ESTA EN SWAP, YA CONOZCO LA DIRECCION DE SU MARCO //
//		{
//			log_info(ptrLog, "Se encontro la pagina a leer en memoria");
//			usleep(retardo * SLEEP); // SLEEP PORQUE OPERO CON LA PAGINA SEGUN ISSUE 71
//
//
//			//puedo llamar a actualizar proceso, en teoria si esta en FIFO o CLOCK no hace nada
//
////			if (!strcmp(algoritmoReemplazo, "LRU")) { //VER TEMA DE ALGORITMO
////				actualizarTablaProcesoLru(tabla_proc, package->pagina_proceso,
////						pagina_proc->direccion_fisica, pagina_proc->marco);
////			}
////			if (!strcmp(tlbHabilitada, "SI"))
////				actualizarTlb(package->PID, package->pagina_proceso,
////						pagina_proc->direccion_fisica, TLB, pagina_proc->marco);
////
////			upPaginasAccedidas(tablaAccesos, package->PID);
////			int tamanioMsj = strlen(pagina_proc->direccion_fisica) + 1;
////			send(socketCliente, &tamanioMsj, sizeof(int), 0);
////
////			log_info(ptrLog, "La pagina contiene: %s. Se envia al CPU",
////					pagina_proc->direccion_fisica);
////
////			if (tamanioMsj > 0)
////				send(socketCliente, pagina_proc->direccion_fisica, tamanioMsj,
////						0);
//		}
//	} else {
//		log_info(ptrLog,
//				"Se esta queriendo leer una pagina de un proceso que no esta iniciado, informo al CPU");
//		int recibi = -1;
//		send(socketCliente, &recibi, sizeof(int), 0);
//	}
//	free(flag);
//}
//
//void ejecutarOperaciones(t_headerCPU * header, char * mensaje, char * memoria_real,
//		t_list * TLB, t_list * tablaMemoria, int socketCPU, int socketSwap,
//		t_list* tablaAccesos) {
//
//	int * flag = malloc(sizeof(uint32_t));
//
//	// ME FIJO QUE TENGO QUE HACER
//	switch (header->operacion) {
//	case 0:
//		/* LA INICIALIZACION SE MANDA DIRECO AL SWAP PARA QUE RESERVE ESPACIO,
//		 EL FLAG = 1 ME AVISA QUE RECIBIO OK */
////		enviarMensajeASwap(header, socketSwap, NULL, flag);
////		bool recibi;
//		if (*flag == 1) {
//			//creo todas las estructuras porque el swap ya inicializo
//			iniciarProceso(tablaMemoria, header, tablaAccesos);
//			log_info(ptrLog,
//					"Proceso mProc creado, numero de PID: %d y cantidad de paginas: %d",
//					header->PID, header->pagina_proceso);
//			// VER COMO MANDAR LA VALIDACION AL CPU QUE NO LE ESTA LLEGANDO BIEN
////			recibi = true;
////			send(socketCPU, &recibi, sizeof(bool), 0);
//			log_info(ptrLog, "Se envia al CPU confimacion de inicializacion");
//		} else {
////			recibi = false;
////			send(socketCPU, &recibi, sizeof(bool), 0);
//			log_info(ptrLog, "Hubo un problema con la conexion/envio al swap");
//		}
//		break;
//	case 1:
//		log_info(ptrLog,
//				"Solicitud de lectura recibida del PID: %d y pagina: %d",
//				header->PID, header->pagina_proceso);
//		/* CUANDO SE RECIBE UNA INSTRUCCION DE LECTURA, PRIMERO SE VERIFICA LA TLB A VER SI YA FUE CARGADA
//		 * RECIENTEMENTE, EN CASO CONTRARIO SE ENVIA AL SWAP, ESTE ME VA A DEVOLVER LA PAGINA A LEER,
//		 * LA MEMORIA LA ALMACENA EN UN MARCO DISPONIBLE PARA EL PROCESO DE LA PAGINA Y ACTUALIZA SUS TABLAS
//		 */
//		// PRIMERO VERIFICO QUE LA TLB ESTE HABILITADA*/
//		if (!strcmp(tlbHabilitada, "SI")) {
//			int lectura = leerDesdeTlb(socketCPU, TLB, header, tablaAccesos,
//					tablaMemoria);
//			// SI NO ESTABA EN TLB, ME FIJO EN MEMORIA
//			if (!lectura) {
//				leerEnMemReal(tablaMemoria, TLB, header, socketSwap,
//						socketCPU, tablaAccesos);
//			}
//		} else {
//			leerEnMemReal(tablaMemoria, TLB, header, socketSwap, socketCPU,
//					tablaAccesos);
//		}
//		break;
//	case 2:
//		log_info(ptrLog,
//				"Solicitud de escritura recibida del PID: %d y Pagina: %d, el mensaje es: \"%s\"",
//				header->PID, header->pagina_proceso, mensaje);
//		// DECLARO UN FLAG PARA SABER SI ESTABA EN LA TLB Y SE ESCRIBIO, O SI NO ESTABA
//		if (!strcmp(tlbHabilitada, "SI")) {
//			int okTlb = escribirDesdeTlb(TLB, header->tamanio_msj, mensaje,
//					header, tablaAccesos, tablaMemoria, socketCPU);
//			// SI ESTABA EN LA TLB, YA LA FUNCION ESCRIBIO Y LISTO
//			if (!okTlb) {
//				escribirEnMemReal(tablaMemoria, TLB, header, socketSwap,
//						socketCPU, mensaje, tablaAccesos);
//			}
//		}
//		// SI NO ESTABA EN LA TLB, AHORA ME FIJO SI ESTA EN LA TABLA DE TABLAS
//		else {
//			escribirEnMemReal(tablaMemoria, TLB, header, socketSwap,
//					socketCPU, mensaje, tablaAccesos);
//		}
//		break;
//	case 3:
//		log_info(ptrLog, TLB, tablaAccesos);
//		envioAlSwap(header,socketSwap, NULL, flag);
//
//		if (flag) {
//			log_info(ptrLog,
//					"Se hizo conexion con swap, se envio proceso a matar y este fue recibido correctamente");
//			bool recibi = true;
//			send(socketCPU, &recibi, sizeof(bool), 0);
//			log_info(ptrLog, "Se informa al CPU confirmacion de finalizacion");
//		} else {
//			bool recibi = false;
//			send(socketCPU, &recibi, sizeof(bool), 0);
//			log_error(ptrLog, "Hubo un problema con la conexion/envio al swap");
//		}
//		break;
//	default:
//		log_error(ptrLog, "El tipo de ejecucion recibido no es valido");
//		break;
//	}
//	free(flag);
//}

int main() {
	if (init()) {
			conexiones.CPUS = (uint32_t *)malloc (10*sizeof(uint32_t));
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
