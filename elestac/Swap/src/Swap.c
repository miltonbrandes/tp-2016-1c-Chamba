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
typedef struct espacioLibre_t {
	struct espacioLibre_t* sgte;
	struct espacioLibre_t* ant;
	uint32_t posicion_inicial;
	uint32_t cantidad_paginas;
} espacioLibre;

typedef struct espacioOcupado_t {
	struct espacioOcupado_t* sgte;
	struct espacioOcupado_t* ant;
	uint32_t posicion_inicial;
	uint32_t cantidad_paginas;
	uint32_t pid;
	uint32_t leido;
	uint32_t escrito;
} espacioOcupado;

espacioLibre* libre;
espacioOcupado* ocupado;
FILE* archivo_control;

//Variables de configuracion
int puertoTCPRecibirConexiones;
int retardoCompactacion;
char* nombreSwap;
int tamanoPagina;
int cantidadPaginas;

typedef struct mensaje_UMC_SWAP_t {
	instruccion_t instruccion;
	uint32_t pid;
	uint32_t parametro; // cantidad de paginas a crear, pagina a leer o a escribir.
	char*contenidoPagina; //Suponemos que el tamanio de la pagina ya lo sabe el swap y que se manda de a una pagina.
} mensaje_UMC_SWAP;

//Metodos para Iniciar valores de la UMC
int crearLog() {
	ptrLog = log_create(getenv("SWAP_LOG"), "Swap", 1, 0);
	if (ptrLog) {
		return 1;
	} else {
		log_info(ptrLog, "Error al crear Log");
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
			cantidadPaginas = config_get_int_value(config, "CANTIDAD_PAGINAS");
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
		log_info(ptrLog, "Error al crear Archivo de Configuracion");
		return 0;
	}

	return 1;
}

void eliminarListas(void) { //liberamos la memoria de las listas de ocupados y de libres
	if (libre) //si hay nodos libres
	{
		espacioLibre* siguiente = libre->sgte;
		while (siguiente) {
			free(libre);
			libre = siguiente;
			siguiente = siguiente->sgte;
		}
		free(libre);
		libre = NULL;
	}
	if (ocupado) //si hay nodos ocupados
	{
		espacioOcupado* siguiente = ocupado->sgte;
		while (siguiente) {
			free(ocupado);
			ocupado = siguiente;
			siguiente = siguiente->sgte;
		}
		free(ocupado);
		ocupado = NULL;
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
		uint32_t id;
		uint32_t operacion;
		int bytesRecibidos = recibirDatos(socketUMC, &buffer, &operacion, &id);

		if (bytesRecibidos < 0) {
			log_info(ptrLog, "Ocurrio un error al Leer datos de UMC\0");
			finalizarConexion(socketUMC);
			return;
		} else if (bytesRecibidos == 0) {
			log_info(ptrLog, "No se recibio ningun byte de UMC");
			//Aca matamos Socket UMC
			finalizarConexion(socketUMC);
			return;
		} else {
			log_info(ptrLog, buffer);
			//Envio algo a UMC esperando con un retardo de compactacion
			log_info(ptrLog, "Esperando Compactacion");
			sleep(retardoCompactacion);
			char *mensajeUMC = "Toma la pagina que te pidio nucleo\0";
			uint32_t id = 4;
			uint32_t longitud = strlen(mensajeUMC);
			uint32_t operacion = 1;
			int sendBytes = enviarDatos(socketUMC, mensajeUMC, longitud,
					operacion, id);
			log_info(ptrLog, "Pagina enviada");
		}

	} while (1);
}

int crearArchivoControlMemoria() {
	char nombre_archivo[30];
	strcpy(nombre_archivo, nombreSwap);
	archivo_control = fopen(nombre_archivo, "w+");
	if (archivo_control == 0) {
		printf("No se pudo crear el archivo en swap.c \n");
		log_error(ptrLog, "Fallo al crear el archivo de swap");
		cerrarSwap();
	}
	inicializarArchivo();
	libre = malloc(sizeof(espacioLibre)); //creamos el primero nodo libre (archivo entero)
	if (!libre) {
		printf("fallo el malloc para la lista de libres en swap.c \n");
		log_error(ptrLog, "Fallo el malloc para la lista libres en swap");
		cerrarSwap();
	}
	libre->sgte = NULL;
	libre->ant = NULL;
	libre->posicion_inicial = 0;
	libre->cantidad_paginas = cantidadPaginas;
	return 1;
}

void cerrarSwap(void) //no cierra los sockets
{ //cerramos el swap ante un error
	log_info(ptrLog, "Proceso SWAP finalizado abruptamente. \n");
	fclose(archivo_control);
	eliminarListas();
	log_destroy(ptrLog);
	exit(-1);
}

void inicializarArchivo(void) { //lo llenamos con el caracter correspondiente
	int tamanoArchivo = (cantidadPaginas) * (tamanoPagina);
	char* s = string_repeat('\0', tamanoArchivo);
	fprintf(archivo_control, "%s", s);
	free(s);
	return;
}

int asignarMemoria( uint32_t pid, uint32_t cantPag)
{//le damos memoria al proceso nuevo
	int inicio;
	inicio= hayEspacio(cantPag);
	if(!inicio)
	{
		if(alcanzanPaginas(cantPag)) //si las paginas libres totales alcanzan, que desfragmente
		{
			log_info(ptrLog, "Se inicia la compactacion del archivo");
			printf("se compacta el archivo... \n");
			sleep(retardoCompactacion);//antes de compactar hacemos el sleep
			desfragmentar();
			log_info(ptrLog, "Finaliza la compactacion del archivo");
			printf("finalizada la compactacion del archivo.\n");
			inicio = hayEspacio(cantPag);
		}
		else
		{
			log_info(ptrLog, "Se rechaza el proceso de pid %u por no haber espacio suficiente", pid);
			//printf("No hay espacio suficiente para el proceso de pid: %u\n", pid);
			return 0;
		}
	}
	int exito= agregarOcupado(pid, cantPag, inicio);
	log_info(ptrLog, "Se asignan %u bytes de memoria al proceso de pid %u desde el byte %u",
			cantPag*tamanoPagina, pid, (inicio-1)*tamanoPagina); //CANTIDAD DE BYTES ES NPAG*TMANIOPAGINA
	return exito;
}


int hayEspacio(int espacio)//espacio esta en paginas
{//te dice si hay un nodo libre con es el espacio requerido
	int posicion;
	espacioLibre* raiz=libre;//para no cambiar al puntero
	while(raiz)
	{
		if(raiz->cantidad_paginas >= espacio)
		{
			posicion= raiz->posicion_inicial;//el comienzo minimo es 1
			ocupar(posicion, espacio);//ocupamos el espacio requerido por el proceso
			return posicion;//devolvemos la posicion inicial del espacio
		}
		raiz= raiz->sgte;
	}
	return 0;
}


void ocupar(int posicion, int espacio)
{//cambia un espacio libre a ocupado
	espacioLibre* aux=libre;
	while(aux->posicion_inicial != posicion)//vamos al nodo a ocupar
	{
		aux= aux->sgte;
	}
	aux->posicion_inicial= aux->posicion_inicial + espacio;
	aux->cantidad_paginas= aux->cantidad_paginas - espacio;
	if(aux->cantidad_paginas == 0)//si nos piden el nodo entero
	{
		if(aux->ant)
		{
			aux->ant->sgte= aux->sgte;
		}
		if(aux->sgte)
		{
			aux->sgte->ant= aux->ant;
		}

		if(aux == libre)
		{
			libre= libre->sgte;
			if(libre)
			{
				libre->ant= NULL;
			}
		}
		free(aux);
	}
    int tamano=(espacio)*(tamanoPagina);
    char* s = string_repeat('\0', tamano);
    fseek(archivo_control, (posicion -1) * tamanoPagina, SEEK_SET);//vamos al inicio de ocupado
    fwrite(s, sizeof(char), (espacio * tamanoPagina) , archivo_control);
    free(s);
	return;
}

int alcanzanPaginas(int pagsNecesarias)//0 no alcanzan, 1 alcanzan
{//nos dice si vale la pena desfragmentar (si hay paginas libres suficientes)
	espacioLibre* auxL= libre;
	int pagsTotales= 0;
	while (auxL)
	{
		pagsTotales= pagsTotales + auxL->cantidad_paginas;//sumamos todas las paginas libres en pagsTotales
		auxL= auxL->sgte;
	}
	if(pagsNecesarias > pagsTotales) //se necesitan mas de las que hay
	{
		return 0;
	}
	return 1;
}

void desfragmentar(void)
{//movemos los nodos ocupados y juntamos los espacios libres en uno solo
	int sizeLibres= 0;
	espacioLibre* auxLibre= libre;
	espacioOcupado* auxO= ocupado;
	while(auxLibre)//contamos cuantos espacios libres hay
    {
        auxLibre=auxLibre->sgte;
        sizeLibres++;
	}
    while (sizeLibres>1)
    {
    	auxO= ocupado;//el nodo siguiente puede reprecentar una pagina anterior, no estan en orden, por eso inicializamos en el comienzo en cada iteracion
    	while(auxO && auxO->posicion_inicial != libre->posicion_inicial + libre->cantidad_paginas)
    	{//vamos al nodo ocupado a la derecha del nodo libre, si existe
    		auxO=auxO->sgte;
    	}
    	if(!auxO) return;//si no hay nodos a la derecha nos vamos
    	moverInformacion(auxO->posicion_inicial, auxO->cantidad_paginas, libre->posicion_inicial);
    	auxO->posicion_inicial= libre->posicion_inicial;//lo colocamos al principio de los libres
    	libre->posicion_inicial= libre->posicion_inicial + auxO->cantidad_paginas;//movemos la raiz al final del nodo ocupado
    	if(libre->sgte->posicion_inicial == libre->posicion_inicial + libre->cantidad_paginas)
    	{//si la pagina siguiente tmb esta libre los unimos en un solo nodo
    		unirBloquesLibres();
    		sizeLibres--;
    	}
    }
	return;
}

void moverInformacion(int inicioDe, int cantPags, int inicioA)// puse unos -1 alguien que me confime que tiene sentido
{//intercambia lo escrito en el swap para cuando movemos un nodo ocupado al desfragmentar
	char buffer[cantPags * tamanoPagina];//creamos el buffer
	fseek(archivo_control, (inicioDe -1) * tamanoPagina, SEEK_SET);//vamos al inicio de ocupado
	fread(buffer, sizeof(char), (cantPags * tamanoPagina) , archivo_control);//leemos
	fseek(archivo_control, (inicioA -1) * tamanoPagina, SEEK_SET);//vamos a libre
	fwrite(buffer, sizeof(char), (cantPags * tamanoPagina) , archivo_control);//escribimos
	return;//en el "nuevo" libre ahora hay basura
}

int agregarOcupado(uint32_t pid, uint32_t cantPag, int comienzo)//LOS NODOS OCUPADOS SE APILAN SIN ORDEN
{//llega un proceso nuevo y le asignamos un nodo correspondiente
	espacioOcupado* aux= ocupado;
	while(aux && aux->sgte)//vamos al ultimo nodo si hay una lista
	{
		aux= aux->sgte;
	}
	espacioOcupado* nuevo= malloc(sizeof(espacioOcupado));
	if(!nuevo)
	{
		printf("fallo el malloc para la lista de ocupados \n");
		log_error(ptrLog, "Fallo el malloc para la lista de ocupados");
		cerrarSwap();
	}
	nuevo->pid= pid;
	nuevo->cantidad_paginas= cantPag;
	nuevo->posicion_inicial= comienzo;
	nuevo->sgte= NULL;
	nuevo->ant= aux;
	nuevo->escrito= 0;
	nuevo->leido= 0;
	if(!aux)//si no habia nodos
	{
		ocupado= nuevo;
		return 1;
	}
	aux->sgte= nuevo;//si habia
	return 1;
}

int main() {
	if (init()) {
		libre = NULL;
		ocupado = NULL;
		//MENSAJE
		socketReceptorUMC = AbrirSocketServidor(puertoTCPRecibirConexiones);
		if (socketReceptorUMC < 0) {
			log_info(ptrLog, "No se pudo abrir el Socket Servidor Swap");
			return -1;
		}
		log_info(ptrLog, "Se abrio el socket Servidor Swap");
		int exito_creacion_archivo = crearArchivoControlMemoria();
		if (exito_creacion_archivo) {
			printf("se creo el archivo correctamente\n");
			log_info(ptrLog,
					"Se creo correctamente el archivo de control de memoria");
		} else {
			printf("fallo al crear el archivo \n");
			log_error(ptrLog, "Fallo al crear el archivo, cerrando swap.");
			cerrarSwap();
		}

		do {
			log_info(ptrLog, "Esperando conexiones");
			int socketUMC = AceptarConexionCliente(socketReceptorUMC);
			log_info(ptrLog, "Conexion de UMC aceptada");
			manejarConexionesRecibidas(socketUMC);
		} while (1);

	} else {
		log_info(ptrLog, "El SWAP no pudo inicializarse correctamente");
		return -1;
	}
	/*MENSAJE!?!?!?
	 *
	 *
	 * int status;		// Estructura que maneja el status de los receive.
	 status = recibirPaginaDeADM(socketUMC, &mensaje, tamanoPagina);
	 while (status) {
	 if (status == -1) {
	 printf("Error en recibir pagina de ADM\n");
	 log_error(log, "Error en recibir pagina de ADM");
	 cerrarSwap();
	 }
	 //printf("Recibi de ADM: Pid: %d Inst: %d Parametro: %d\n",mensaje.pid,mensaje.instruccion,mensaje.parametro);
	 interpretarMensaje(mensaje, socketUMC);
	 status = recibirPaginaDeADM(socketADM, &mensaje,
	 configuracion.TAMANIO_PAGINA);
	 }*/
	printf("Proceso SWAP finalizado.\n\n");
	log_info(ptrLog, "Proceso SWAP finalizado.\n");
	finalizarConexion(socketReceptorUMC);
	fclose(archivo_control);
	eliminarListas();
	//log_destroy(ptrLog);
	return EXIT_SUCCESS;
}
