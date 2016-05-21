/*
 * Swap.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <sockets/ServidorFunciones.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <sockets/EscrituraLectura.h>
#include <sockets/OpsUtiles.h>

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

		buffer = recibirDatos(socketUMC, &operacion, &id);
		int bytesRecibidos = strlen(buffer);

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
			if (strcmp("ERROR", buffer) == 0) {
				return;
			} else {
				interpretarMensajeRecibido(buffer, socketUMC, id);
			}
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

int interpretarMensajeRecibido(char* buffer, int socket, uint32_t ** pid){
	int resultado;
	espacioOcupado* aBorrar;
	espacioOcupado*aEscribir;
	espacioOcupado* aLeer;
	int cantidadPaginasALeer;
	int cantidadPaginasAEscribir;
	switch(buffer){
		case NUEVOPROGRAMA:
			resultado=asignarMemoria(pid, cantidadPaginasALeer);
			if (!resultado)
			{
				char* paqueteAEnviar= serializarUint32(*pid);
				int enviado=enviarDatos(socket,paqueteAEnviar,sizeof(uint32_t),RECHAZAR_PROCESO_A_UMC,SWAP);
				if(enviado==-1)
				{
					printf("No se pudo enviar mensaje a UMC ");
					log_error(ptrLog, "No se pudo enviar mensaje a UMC");
				}
				return 0;
			}
		break;
		case FINALIZARPROGRAMA:
			aBorrar=ocupado;
			while (aBorrar && aBorrar->pid != pid) aBorrar= aBorrar->sgte;
			if(!aBorrar)
			{
				printf ("El proceso %d no se encuentra en el swap \n",pid);
				log_error(ptrLog, "El proceso %d no se encuentra en el swap",pid);
				char* paqueteAEnviar= serializarUint32(*pid);
				int enviado=enviarDatos(socket,paqueteAEnviar,sizeof(uint32_t), RECHAZAR_FINALIZAR_PROCESO_A_UMC);
				if(enviado==-1)
				{
					printf("No se pudo enviar mensaje al ADM\n");
					log_error(ptrLog, "No se pudo enviar mensaje al ADM");
				}
				return 0;
			}
			//printf("Se libero la memoria del proceso de pid %u \n", aBorrar->pid);
			liberarMemoria(aBorrar);
		break;
		case LEER:
			sleep(retardoCompactacion);
			aLeer=ocupado;
			while(aLeer && aLeer->pid !=pid) aLeer=aLeer->sgte;
			if(!aLeer)
			{
				printf ("El proceso %d no se encuentra en el swap \n",pid);
				char* paqueteAEnviar= serializarUint32(*pid);
				int enviado=enviarDatos(socket,paqueteAEnviar,sizeof(uint32_t), RECHAZAR_LEER_PROCESO_A_UMC);
				if(enviado==-1)
				{
					printf("No se pudo enviar mensaje a UMC\n");
					log_error(ptrLog, "No se pudo enviar mensaje a UMC");
				}
				return 0;
			}
			char* leido= leerProceso(aLeer,cantidadPaginasALeer);
			enviarDatos(socket, leido,sizeof(leido),ENVIAR_PAGINA_A_UMC);
			free(leido);
		break;
		case ESCRIBIR:
			sleep(retardoCompactacion);
			aEscribir=ocupado;
			while(aEscribir && aEscribir->pid != pid) aEscribir=aEscribir->sgte;
			if(!aEscribir)
			{
				printf ("El proceso %d no se encuentra en el swap \n",pid);
				log_error(ptrLog, "El proceso %d no se encuentra en el swap \n",pid);
				char* paqueteAEnviar= serializarUint32(*pid);
				int enviado=enviarDatos(socket,paqueteAEnviar,sizeof(uint32_t), RECHAZAR_ESCRIBIR_PROCESO_A_UMC);
				if(enviado==-1)
				{
					printf("No se pudo enviar mensaje al ADM\n");
					log_error(ptrLog, "No se pudo enviar mensaje al ADM");
					cerrarSwap();
				}
				return 0;
			}
			escribirProceso(aEscribir, cantidadPaginasAEscribir, buffer);
			buffer=NULL;
			break;

		break;
		default:
			printf("Mensaje de UMC no comprendido\n");
			log_error(ptrLog, "Mensaje de UMC no comprendido");
			cerrarSwap();
			break;
	}
	//si llegó hasta acá es porque esta OK
	int i=-1;
	i=enviarDatos(socket, "Mensaje comprendido",sizeof(uint32_t),SUCCESS);
	if(i==-1)
	{
		printf("No se pudo enviar mensaje de confirmacion a UMC\n");
		log_error(ptrLog, "No se pudo enviar mensaje de confirmacion a UMC");
		cerrarSwap();
	}
	return 1;
}

char* leerProceso(espacioOcupado* aLeer, uint32_t pagALeer)//pagALeer tiene como 0 a la pos inicial
{//leemos una pagina del archivo de swap
	char* buffer=NULL;
	buffer= calloc(tamanoPagina, 1);//es como el malloc pero inicializa en \0
	if(!buffer)
	{
		printf("Fallo la creacion del buffer en el swap funcion leer \n");
		log_error(ptrLog, "Fallo la creacion del buffer en el swap funcion leer");
		return buffer;
	}
	fseek(archivo_control,((aLeer->posicion_inicial -1) * tamanoPagina) + (pagALeer * tamanoPagina), SEEK_SET);//vamos la pagina a leer (sin el menos uno la pasamos)
	fread(buffer, sizeof(char), tamanoPagina, archivo_control);//leemos
	aLeer->leido= aLeer->leido +1;//aumentamos la cantidad de paginas leidas por el proceso
	char bufferLogueo[tamanoPagina +1];//esto va a ser para meterle un \0 por las dudas
	strcpy(bufferLogueo, buffer);
	bufferLogueo[tamanoPagina]='\0';
	log_info(ptrLog, "El proceso de pid %u lee %u bytes comenzando en el byte %u y leyo: %s", aLeer->pid, strlen(buffer)*sizeof(char), (aLeer->posicion_inicial -1) * tamanoPagina +  pagALeer * tamanoPagina, bufferLogueo); //EN EL BYTE DESDE QUE COMIENZ AGREGO EL NUM PAG
	return buffer;
}

void inicializarArchivo(void) { //lo llenamos con el caracter correspondiente
	int tamanoArchivo = (cantidadPaginas) * (tamanoPagina);
	char* s = string_repeat('\0', tamanoArchivo);
	fprintf(archivo_control, "%s", s);
	free(s);
	return;
}

//funciones para asignar memoria
int asignarMemoria(uint32_t pid, uint32_t cantidad_paginas) { //le damos memoria al proceso nuevo
	int inicio;
	inicio = hayEspacio(cantidad_paginas);
	if (!inicio) {
		if (alcanzanPaginas(cantidad_paginas)) //si las paginas libres totales alcanzan, que desfragmente
			{
			log_info(ptrLog, "Se inicia la compactacion del archivo");
			printf("se compacta el archivo... \n");
			sleep(retardoCompactacion); //antes de compactar hacemos el sleep
			desfragmentar();
			log_info(ptrLog, "Finaliza la compactacion del archivo");
			printf("finalizada la compactacion del archivo.\n");
			inicio = hayEspacio(cantidad_paginas);
		} else {
			log_info(ptrLog,"Se rechaza el proceso de pid %u por no haber espacio suficiente",pid);
			//printf("No hay espacio suficiente para el proceso de pid: %u\n", pid);
			return 0;
		}
	}
	int exito = agregarOcupado(pid, cantidad_paginas, inicio);
	log_info(ptrLog,
			"Se asignan %u bytes de memoria al proceso de pid %u desde el byte %u",
			cantidad_paginas * tamanoPagina, pid, (inicio - 1) * tamanoPagina); //CANTIDAD DE BYTES ES NPAG*TMANIOPAGINA
	return exito;
}

int hayEspacio(int espacio) //espacio esta en paginas
{ //te dice si hay un nodo libre con es el espacio requerido
	int posicion;
	espacioLibre* raiz = libre; //para no cambiar al puntero
	while (raiz) {
		if (raiz->cantidad_paginas >= espacio) {
			posicion = raiz->posicion_inicial; //el posicion_inicial minimo es 1
			ocupar(posicion, espacio); //ocupamos el espacio requerido por el proceso
			return posicion; //devolvemos la posicion inicial del espacio
		}
		raiz = raiz->sgte;
	}
	return 0;
}

void ocupar(int posicion, int espacio) { //cambia un espacio libre a ocupado
	espacioLibre* aux = libre;
	while (aux->posicion_inicial != posicion) //vamos al nodo a ocupar
	{
		aux = aux->sgte;
	}
	aux->posicion_inicial = aux->posicion_inicial + espacio;
	aux->cantidad_paginas = aux->cantidad_paginas - espacio;
	if (aux->cantidad_paginas == 0) //si nos piden el nodo entero
			{
		if (aux->ant) {
			aux->ant->sgte = aux->sgte;
		}
		if (aux->sgte) {
			aux->sgte->ant = aux->ant;
		}

		if (aux == libre) {
			libre = libre->sgte;
			if (libre) {
				libre->ant = NULL;
			}
		}
		free(aux);
	}
	int tamano = (espacio) * (tamanoPagina);
	char* s = string_repeat('\0', tamano);
	fseek(archivo_control, (posicion - 1) * tamanoPagina, SEEK_SET); //vamos al inicio de ocupado
	fwrite(s, sizeof(char), (espacio * tamanoPagina), archivo_control);
	free(s);
	return;
}

int alcanzanPaginas(int pagsNecesarias) //0 no alcanzan, 1 alcanzan
{ //nos dice si vale la pena desfragmentar (si hay paginas libres suficientes)
	espacioLibre* auxL = libre;
	int pagsTotales = 0;
	while (auxL) {
		pagsTotales = pagsTotales + auxL->cantidad_paginas; //sumamos todas las paginas libres en pagsTotales
		auxL = auxL->sgte;
	}
	if (pagsNecesarias > pagsTotales) //se necesitan mas de las que hay
			{
		return 0;
	}
	return 1;
}

void desfragmentar(void) { //movemos los nodos ocupados y juntamos los espacios libres en uno solo
	int sizeLibres = 0;
	espacioLibre* auxLibre = libre;
	espacioOcupado* auxO = ocupado;
	while (auxLibre) //contamos cuantos espacios libres hay
	{
		auxLibre = auxLibre->sgte;
		sizeLibres++;
	}
	while (sizeLibres > 1) {
		auxO = ocupado; //el nodo siguiente puede reprecentar una pagina anterior, no estan en orden, por eso inicializamos en el posicion_inicial en cada iteracion
		while (auxO
				&& auxO->posicion_inicial
						!= libre->posicion_inicial + libre->cantidad_paginas) { //vamos al nodo ocupado a la derecha del nodo libre, si existe
			auxO = auxO->sgte;
		}
		if (!auxO)
			return; //si no hay nodos a la derecha nos vamos
		moverInformacion(auxO->posicion_inicial, auxO->cantidad_paginas,
				libre->posicion_inicial);
		auxO->posicion_inicial = libre->posicion_inicial; //lo colocamos al principio de los libres
		libre->posicion_inicial = libre->posicion_inicial
				+ auxO->cantidad_paginas; //movemos la raiz al final del nodo ocupado
		if (libre->sgte->posicion_inicial
				== libre->posicion_inicial + libre->cantidad_paginas) { //si la pagina siguiente tmb esta libre los unimos en un solo nodo
			unirBloquesLibres();
			sizeLibres--;
		}
	}
	return;
}

void unirBloquesLibres(void) { //une dos nodos libres en uno solo mas grande
	espacioLibre* nodoABorrar = libre->sgte;
	libre->cantidad_paginas = libre->cantidad_paginas
			+ nodoABorrar->cantidad_paginas;
	if (!(nodoABorrar->sgte)) //solo son dos nodos
	{
		libre->sgte = NULL;
		free(nodoABorrar);
		return;
	}
	nodoABorrar->sgte->ant = libre;
	libre->sgte = nodoABorrar->sgte;
	free(nodoABorrar);
	return;
}

void moverInformacion(int inicioDe, int cantidad_paginass, int inicioA) // puse unos -1 alguien que me confime que tiene sentido
{ //intercambia lo escrito en el swap para cuando movemos un nodo ocupado al desfragmentar
	char buffer[cantidad_paginass * tamanoPagina]; //creamos el buffer
	fseek(archivo_control, (inicioDe - 1) * tamanoPagina, SEEK_SET); //vamos al inicio de ocupado
	fread(buffer, sizeof(char), (cantidad_paginass * tamanoPagina),
			archivo_control); //leemos
	fseek(archivo_control, (inicioA - 1) * tamanoPagina, SEEK_SET); //vamos a libre
	fwrite(buffer, sizeof(char), (cantidad_paginass * tamanoPagina),
			archivo_control); //escribimos
	return; //en el "nuevo" libre ahora hay basura
}

int agregarOcupado(uint32_t pid, uint32_t cantidad_paginas,int posicion_inicial) //LOS NODOS OCUPADOS SE APILAN SIN ORDEN
{ //llega un proceso nuevo y le asignamos un nodo correspondiente
	espacioOcupado* aux = ocupado;
	while (aux && aux->sgte) //vamos al ultimo nodo si hay una lista
	{
		aux = aux->sgte;
	}
	espacioOcupado* nuevo = malloc(sizeof(espacioOcupado));
	if (!nuevo) {
		printf("fallo el malloc para la lista de ocupados \n");
		log_error(ptrLog, "Fallo el malloc para la lista de ocupados");
		cerrarSwap();
	}
	nuevo->pid = pid;
	nuevo->cantidad_paginas = cantidad_paginas;
	nuevo->posicion_inicial = posicion_inicial;
	nuevo->sgte = NULL;
	nuevo->ant = aux;
	nuevo->escrito = 0;
	nuevo->leido = 0;
	if (!aux) //si no habia nodos
	{
		ocupado = nuevo;
		return 1;
	}
	aux->sgte = nuevo; //si habia
	return 1;
}

//funciones para liberar memoria
int liberarMemoria(espacioOcupado* aBorrar) { //se va un proceso y borramos su nodo ocupado y agregamos un libre
	int anteriorVar = anterior(aBorrar);
	int posteriorVar = posterior(aBorrar);
	log_info(ptrLog,
			"Se liberan %u bytes de memoria del proceso de pid %u desde el byte %u. El proceso leyo %u paginas y escribio %u.",
			(aBorrar->cantidad_paginas) * tamanoPagina, aBorrar->pid,
			(aBorrar->posicion_inicial - 1) * tamanoPagina, aBorrar->leido,
			aBorrar->escrito);
	if (anteriorVar == 0 && posteriorVar == 0) //el proceso ocupa el archivo entero
			{
		inicializarArchivo();
		libre = malloc(sizeof(espacioLibre)); //creamos el primero nodo libre (que esTodo el archivo)
		if (!libre) {
			printf("fallo el malloc para la lista de libres en swap.c \n");
			log_error(ptrLog, "Fallo el malloc para la lista de libres");
			cerrarSwap();
		}
		libre->sgte = NULL;
		libre->ant = NULL;
		libre->posicion_inicial = 1; //posicion 1 en vez de 0, mas comodo
		libre->cantidad_paginas = (cantidadPaginas);
		borrarNodoOcupado(aBorrar);
		return 1;
	} else if (anteriorVar == 0 && posteriorVar == 1) //tiene un libre posterior
			{
		libre->posicion_inicial = 1;
		libre->cantidad_paginas = libre->cantidad_paginas
				+ aBorrar->cantidad_paginas;
		borrarNodoOcupado(aBorrar);
		return 1;
	} else if (anteriorVar == 1 && posteriorVar == 0) //tiene libre anterior y nada posterior
			{
		espacioLibre* ultimo = libre;
		while (ultimo->sgte) //vamos al ultimo nodo libre
		{
			ultimo = ultimo->sgte;
		}
		ultimo->cantidad_paginas = ultimo->cantidad_paginas
				+ aBorrar->cantidad_paginas;
		borrarNodoOcupado(aBorrar);
		return 1;
	} else if (anteriorVar == 2 && posteriorVar == 2) //esta entre dos ocupados, hay varios sub casos aca
			{
		if (!libre) // estaba ocupado entero y sacamos un proceso
		{
			libre = malloc(sizeof(espacioLibre));
			if (!libre) {
				printf("fallo el malloc para la lista de libres en swap.c \n");
				log_error(ptrLog, "Fallo el malloc para la lista de libres");
				cerrarSwap();
			}
			libre->sgte = NULL;
			libre->ant = NULL;
			libre->posicion_inicial = aBorrar->posicion_inicial;
			libre->cantidad_paginas = aBorrar->cantidad_paginas;
			borrarNodoOcupado(aBorrar);
			return 1;
		} else if (libre->posicion_inicial
				> aBorrar->cantidad_paginas + aBorrar->posicion_inicial) { //la raiz esta posterior. pasamos la raiz anterior y agregamos un nodo donde estaba la raiz
			espacioLibre* nuevo;
			nuevo = malloc(sizeof(espacioLibre));
			if (!nuevo) {
				printf("fallo el malloc para la lista de libres en swap.c \n");
				log_error(ptrLog, "Fallo el malloc para la lista de libres");
				cerrarSwap();
			}
			if (libre->sgte) {
				libre->sgte->ant = nuevo;
			}
			nuevo->cantidad_paginas = libre->cantidad_paginas;
			nuevo->posicion_inicial = libre->posicion_inicial;
			nuevo->sgte = libre->sgte;
			nuevo->ant = libre;
			libre->cantidad_paginas = aBorrar->cantidad_paginas;
			libre->posicion_inicial = aBorrar->posicion_inicial;
			libre->ant = NULL;
			libre->sgte = nuevo;
			borrarNodoOcupado(aBorrar);
			return 1;
		} else //la raiz esta anterior
		{
			espacioLibre* anterior = libre;
			while (anterior->sgte
					&& anterior->sgte->posicion_inicial
							+ anterior->sgte->cantidad_paginas
							< aBorrar->posicion_inicial) {
				anterior = anterior->sgte;
			} //mientras no sea el ultimo y el nodo siguiente este por dentras de nuestro nodo avanzamos
			espacioLibre* nuevo;
			nuevo = malloc(sizeof(espacioLibre));
			if (!nuevo) {
				printf("fallo el malloc para la lista de libres en swap.c \n");
				log_error(ptrLog, "Fallo el malloc para la lista de libres");
				cerrarSwap();
			}
			nuevo->posicion_inicial = aBorrar->posicion_inicial;
			nuevo->cantidad_paginas = aBorrar->cantidad_paginas;
			nuevo->ant = anterior;
			nuevo->sgte = anterior->sgte;
			if (anterior->sgte) {
				anterior->sgte->ant = nuevo;
			}
			anterior->sgte = nuevo;
			borrarNodoOcupado(aBorrar);
			return 1;
		}
	} else if (anteriorVar == 1 && posteriorVar == 2) //esta entre un libre y un ocupado
			{
		espacioLibre* anterior = libre;
		while (anterior->posicion_inicial + anterior->cantidad_paginas
				!= aBorrar->posicion_inicial) {
			anterior = anterior->sgte;
		}
		anterior->cantidad_paginas = anterior->cantidad_paginas
				+ aBorrar->cantidad_paginas;
		borrarNodoOcupado(aBorrar);
		return 1;
	} else if (anteriorVar == 2 && posteriorVar == 1) //esta entre un ocupado y un libre
			{
		espacioLibre* siguiente = libre;
		while (siguiente->posicion_inicial
				!= aBorrar->posicion_inicial + aBorrar->cantidad_paginas) { //si a la derecha esta la raiz anda igual
			siguiente = siguiente->sgte;
		}
		siguiente->posicion_inicial = aBorrar->posicion_inicial;
		siguiente->cantidad_paginas = siguiente->cantidad_paginas
				+ aBorrar->cantidad_paginas;
		borrarNodoOcupado(aBorrar);
		return 1;
	} else if (anteriorVar == 1 && posteriorVar == 1) //esta entre dos libres
			{
		espacioLibre* anterior = libre;
		while (anterior->posicion_inicial + anterior->cantidad_paginas
				!= aBorrar->posicion_inicial) {
			anterior = anterior->sgte;
		}
		espacioLibre* aUnir = anterior->sgte; //a este nodo mas tarde lo tenemos que eliminar
		anterior->cantidad_paginas = anterior->cantidad_paginas
				+ aBorrar->cantidad_paginas + aUnir->cantidad_paginas; //unimos los tres en uno
		if (aUnir->sgte) //si hay un tercer nodo libre
		{
			aUnir->sgte->ant = anterior;
		}
		anterior->sgte = aUnir->sgte;
		borrarNodoOcupado(aBorrar);
		free(aUnir);
		return 1;
	} else if (anteriorVar == 2 && posteriorVar == 0) //entre un ocupado y el final del archivo
			{
		if (!libre) {
			libre = malloc(sizeof(espacioLibre));
			if (!libre) {
				printf("fallo el malloc para la lista de libres en swap.c \n");
				log_error(ptrLog, "Fallo el malloc para la lista de libres");
				cerrarSwap();
			}
			libre->sgte = NULL;
			libre->ant = NULL;
			libre->posicion_inicial = aBorrar->posicion_inicial;
			libre->cantidad_paginas = aBorrar->cantidad_paginas;
			borrarNodoOcupado(aBorrar);
			return 1;
		} else //hay un anterior a la izquierda
		{
			espacioLibre* anterior = libre;
			while (anterior->sgte) //vamos al ultimo nodo de los libres
			{
				anterior = anterior->sgte;
			}
			espacioLibre* nuevo;
			nuevo = malloc(sizeof(espacioLibre));
			if (!nuevo) {
				printf("fallo el malloc para la lista de libres en swap.c \n");
				log_error(ptrLog, "Fallo el malloc para la lista de libres");
				cerrarSwap();
			}
			nuevo->posicion_inicial = aBorrar->posicion_inicial;
			nuevo->cantidad_paginas = aBorrar->cantidad_paginas;
			nuevo->ant = anterior;
			nuevo->sgte = NULL;
			anterior->sgte = nuevo;
			borrarNodoOcupado(aBorrar);
			return 1;
		}
	} else if (anteriorVar == 0 && posteriorVar == 2) //entre el inicio del archivo y un ocupado
			{
		if (!libre) {
			libre = malloc(sizeof(espacioLibre));
			if (!libre) {
				printf("fallo el malloc para la lista de libres en swap.c \n");
				log_error(ptrLog, "Fallo el malloc para la lista de libres");
				cerrarSwap();
			}
			libre->sgte = NULL;
			libre->ant = NULL;
			libre->posicion_inicial = aBorrar->posicion_inicial;
			libre->cantidad_paginas = aBorrar->cantidad_paginas;
			borrarNodoOcupado(aBorrar);
			return 1;
		}
		espacioLibre* nuevo;
		nuevo = malloc(sizeof(espacioLibre));
		if (!nuevo) {
			printf("fallo el malloc para la lista de libres en swap.c \n");
			log_error(ptrLog, "Fallo el malloc para la lista de libres");
			cerrarSwap();
		}
		if (libre->sgte) //corremos la raiz al inicio y nuevo donde estaba la raiz
		{
			libre->sgte->ant = nuevo;
		}
		nuevo->cantidad_paginas = libre->cantidad_paginas;
		nuevo->posicion_inicial = libre->posicion_inicial;
		nuevo->sgte = libre->sgte;
		nuevo->ant = libre;
		libre->cantidad_paginas = aBorrar->cantidad_paginas;
		libre->posicion_inicial = aBorrar->posicion_inicial;
		libre->ant = NULL;
		libre->sgte = nuevo;
		borrarNodoOcupado(aBorrar);
		return 1;
	}
	return 0;
}

void borrarNodoOcupado(espacioOcupado* aBorrar)
{//se va un proceso y borramos su nodo
	if(aBorrar->ant)
	{
		aBorrar->ant->sgte=aBorrar->sgte;
	}
	if(aBorrar->sgte)
	{
		aBorrar->sgte->ant= aBorrar->ant;
	}
	if(aBorrar == ocupado)
	{
		ocupado= ocupado->sgte;
		if(ocupado)
		{
			ocupado->ant= NULL;
		}
	}
	free(aBorrar);
	return;
}

int anterior(espacioOcupado* nodo) //0 no hay nada 1 libre 2 ocupado
{ //recibe un nodo ocupado y nos dice si la pagina de atras es libre u ocupada
	int comienzo = nodo->posicion_inicial;
	espacioLibre* libreAux = libre;
	espacioOcupado* ocupadoAux = ocupado;
	while (libreAux) {
		if (comienzo
				== libreAux->posicion_inicial + libreAux->cantidad_paginas) {
			return 1;
		}
		libreAux = libreAux->sgte;
	}
	while (ocupadoAux) {
		if (comienzo
				== ocupadoAux->posicion_inicial
						+ ocupadoAux->cantidad_paginas) {
			return 2;
		}
		ocupadoAux = ocupadoAux->sgte;
	}
	return 0;
}

int posterior(espacioOcupado* nodo) // 0 no hay nada 1 libre 2 ocupado
{ //recibe un nodo ocupado y nos dice si la pagina de adelante es libre u ocupada
	int comienzo = nodo->posicion_inicial;
	int cantPag = nodo->cantidad_paginas;
	espacioLibre* libreAux = libre;
	espacioOcupado* ocupadoAux = ocupado;
	while (libreAux) {
		if (comienzo + cantPag == libreAux->posicion_inicial) {
			return 1;
		}
		libreAux = libreAux->sgte;
	}
	while (ocupadoAux) {
		if (comienzo + cantPag == ocupadoAux->posicion_inicial) {
			return 2;
		}
		ocupadoAux = ocupadoAux->sgte;
	}
	return 0;
}

int main() {
	if (init()) {
		libre = NULL;
		ocupado = NULL;
		char* mensaje;
		uint32_t operacion;
		uint32_t id;
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
	printf("Proceso SWAP finalizado.\n\n");
	log_info(ptrLog, "Proceso SWAP finalizado.\n");
	finalizarConexion(socketReceptorUMC);
	fclose(archivo_control);
	eliminarListas();
	//log_destroy(ptrLog);
	return EXIT_SUCCESS;
}
