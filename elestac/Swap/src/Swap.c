/*
 * Swap.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <sockets/ServidorFunciones.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <sockets/EscrituraLectura.h>

#define MAX_BUFFER_SIZE 4096
//Socket que recibe conexiones de Nucleo y CPU
int socketReceptorUMC;

//Archivo de Log
t_log* ptrLog;

//estructuras para manejar el espacio de memoria
typedef struct espacioLibre_t
{
	struct espacioLibre_t* sgte;
	struct espacioLibre_t* ant;
	uint32_t posicion_inicial;
	uint32_t cantidad_paginas;
}espacioLibre;

typedef struct espacioOcupado_t
{
	struct espacioOcupado_t* sgte;
	struct espacioOcupado_t* ant;
	uint32_t posicion_inicial;
	uint32_t cantidad_paginas;
	uint32_t pid;
	uint32_t leido;
	uint32_t escrito;
}espacioOcupado;

espacioLibre* libre;
espacioOcupado* ocupado;
FILE* archivo_control;

//Variables de configuracion
int puertoTCPRecibirConexiones;
int retardoCompactacion;
char* nombreSwap;
int tamanoPagina;
int cantidadPaginas;

//Metodos para Iniciar valores de la UMC
int crearLog() {
	ptrLog = log_create(getenv("SWAP_LOG"), "Swap", 1, 0);
	if (ptrLog) {
		return 1;
	}
	else {
		log_info(ptrLog,"Error al crear Log");
		return 0;
	}
}
int cargarValoresDeConfig() {
	t_config* config;
	config = config_create(getenv("SWAP_CONFIG"));
	//cuando trato de debuggear me tira un error en la ubicacion de la libreria o algo asi. Igual en consola, que si anda, me tira el mismo error. raro.

	//int tamanoPagina, cantidadPaginas;

	if (config) {
		if (config_has_property(config, "PUERTO_ESCUCHA")) {
			puertoTCPRecibirConexiones = config_get_int_value(config,
					"PUERTO_ESCUCHA");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave PUERTO_ESCUCHA");
			return 0;
		}

		if (config_has_property(config, "NOMBRE_SWAP")) {
			nombreSwap = config_get_string_value(config, "NOMBRE_SWAP");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave NOMBRE_SWAP");
			return 0;
		}

		if (config_has_property(config, "CANTIDAD_PAGINAS")) {
			cantidadPaginas = config_get_int_value(config,
					"CANTIDAD_PAGINAS");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave CANTIDAD_PAGINAS");
			return 0;
		}

		if (config_has_property(config, "TAMANO_PAGINA")) {
			tamanoPagina = config_get_int_value(config, "TAMANO_PAGINA");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave TAMANO_PAGINA");
			return 0;
		}

		if (config_has_property(config, "RETARDO_COMPACTACION")) {
			retardoCompactacion = config_get_int_value(config,
					"RETARDO_COMPACTACION");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave RETARDO_COMPACTACION");
			return 0;
		}
	} else {
		log_info(ptrLog,
				"Error al crear Archivo de Configuracion");
		return 0;
	}

	return 1;
}

void eliminarListas(void)
{//liberamos la memoria de las listas de ocupados y de libres
	if(libre)//si hay nodos libres
	{
		espacioLibre* siguiente= libre->sgte;
		while(siguiente)
		{
			free(libre);
			libre= siguiente;
			siguiente= siguiente->sgte;
		}
		free(libre);
		libre= NULL;
	}
	if(ocupado)//si hay nodos ocupados
	{
		espacioOcupado* siguiente= ocupado->sgte;
		while(siguiente)
		{
			free(ocupado);
			ocupado= siguiente;
			siguiente= siguiente->sgte;
		}
		free(ocupado);
		ocupado= NULL;
	}
	return;
}

int init() {
	if (crearLog()) {
		return cargarValoresDeConfig();
	} else {
		return 0;
	}
}
//Fin Metodos para Iniciar valores de la UMC

void manejarConexionesRecibidas(int socketUMC) {
	do {
		log_info(ptrLog, "Esperando recibir alguna peticion");
		char* buffer;
		int* id;
		int bytesRecibidos = leer(socketUMC, &id, &buffer);

		if(bytesRecibidos < 0) {
			log_info(ptrLog, "Ocurrio un error al Leer datos de UMC\0");
			finalizarConexion(socketUMC);
			return;
		}else if(bytesRecibidos == 0) {
			log_info(ptrLog, "No se recibio ningun byte de UMC");
			//Aca matamos Socket UMC
			finalizarConexion(socketUMC);
			return;
		}else{
			log_info(ptrLog, buffer);
			//Envio algo a UMC esperando con un retardo de compactacion
			log_info(ptrLog, "Esperando Compactacion");
			sleep(retardoCompactacion);
			char *mensajeUMC = "Toma la pagina que te pidio nucleo\0";
			int id = 4;
			int longitud = strlen(mensajeUMC);
			int operacion = 1;
			int sendBytes = escribir(socketUMC,id,longitud,operacion, mensajeUMC);
			log_info(ptrLog, "Pagina enviada");
		}

	}while(1);
}

int crearArchivoControlMemoria(){
	char nombre_archivo [30];
		strcpy(nombre_archivo,nombreSwap);
		archivo_control= fopen(nombre_archivo,"w+");
		if(archivo_control==0)
		{
			printf("No se pudo crear el archivo en swap.c \n");
			log_error(ptrLog, "Fallo al crear el archivo de swap");
			cerrarSwap();
		}
	    inicializarArchivo();
		libre=malloc(sizeof(espacioLibre));//creamos el primero nodo libre (archivo entero)
		if(!libre)
		{
			printf("fallo el malloc para la lista de libres en swap.c \n");
			log_error(ptrLog, "Fallo el malloc para la lista libres en swap");
			cerrarSwap();
		}
		libre->sgte=NULL;
		libre->ant=NULL;
		libre->posicion_inicial=0;
		libre->cantidad_paginas=cantidadPaginas;
		return 1;
}

void cerrarSwap(void)//no cierra los sockets
{//cerramos el swap ante un error
	log_info(ptrLog, "Proceso SWAP finalizado abruptamente. \n");
	fclose(archivo_control);
	eliminarListas();
	log_destroy(ptrLog);
	exit(-1);
}

void inicializarArchivo(void)
{//lo llenamos con el caracter correspondiente
    int tamanoArchivo=(cantidadPaginas)*(tamanoPagina);
    char* s = string_repeat('\0', tamanoArchivo);
    fprintf(archivo_control,"%s", s);
    free(s);
    return;
}

int main() {
	if (init()) {
		libre=NULL;
		ocupado=NULL;
		socketReceptorUMC = AbrirSocketServidor(puertoTCPRecibirConexiones);
		if (socketReceptorUMC < 0) {
			log_info(ptrLog, "No se pudo abrir el Socket Servidor Swap");
			return -1;
		}
		log_info(ptrLog, "Se abrio el socket Servidor Swap");

		do {
			log_info(ptrLog, "Esperando conexiones");
			int socketUMC = AceptarConexionCliente(socketReceptorUMC);
			log_info(ptrLog, "Conexion de UMC aceptada");
			int exito_creacion_archivo=crearArchivoControlMemoria();
			if(exito_creacion_archivo)
				{
					//printf("se creo el archivo correctamente\n");
				}
			else
			{
				printf("fallo al crear el archivo \n");
				log_error(ptrLog,"Fallo al crear el archivo, cerrando swap.");
				cerrarSwap();
			}
			manejarConexionesRecibidas(socketUMC);
		}while(1);


	} else {
		log_info(ptrLog, "El SWAP no pudo inicializarse correctamente");
		return -1;
	}
	printf("Proceso SWAP finalizado.\n\n");
	log_info(ptrLog, "Proceso SWAP finalizado.\n");
	//finalizarConexion(socketUMC);
	fclose(archivo_control);
	eliminarListas();
	//log_destroy(ptrLog);
	return EXIT_SUCCESS;
}
