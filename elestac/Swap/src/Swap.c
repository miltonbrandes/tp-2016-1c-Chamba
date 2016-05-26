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
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

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

typedef struct frame_t{
	uint32_t pid;
	uint32_t numero_pagina;
	uint32_t numero_frame;
} frame;
espacioLibre* libre;
espacioOcupado* ocupado;
FILE* archivo_control;

//Variables de configuracion
int puertoTCPRecibirConexiones;
int retardoCompactacion;
char* nombreSwap;
int tamanoPagina;
int cantidadPaginas;
char* nombre_archivo;

uint32_t paginaAEnviar;
espacioLibre* encontrarHueco(t_list* listaDeLibres, int pagsRequeridas);
uint32_t ocupar(espacioLibre* hueco, t_list* listaDeLibres, t_list* listaDeOcupados, uint32_t pidRecibido,  int pagsRequeridas);
void liberarMemoria(t_list* listaDeLibres, t_list* listaDeOcupados, uint32_t pidRecibido);
espacioOcupado* encontrarLugarAsignadoAProceso(t_list* listaDeOcupados, uint32_t pidRecibido);
void desfragmentar(t_list* listaDeOcupados, t_list* listaDeLibres);
int asignarMemoria(uint32_t pid, uint32_t cantidad_paginas, t_list* listaDeLibres, t_list* listaDeOcupados);
void inicializarArchivo(void);
char* leerProceso(uint32_t pagina, uint32_t pid, t_list* listaDeOcupados);
void escribirProceso(int paginaProceso, char* info , t_list* listaDeOcupados, uint32_t pid);
void cerrarSwap(void);
int interpretarMensajeRecibido(char* buffer,int op, int socket, t_list* listaDeLibres, t_list* listaDeOcupados);
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
		if (config_has_property(config, "CANTIDAD_PAGINAS")) {
			nombre_archivo = config_get_string_value(config, "NOMBRE_ARCHIVO");
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

void manejarConexionesRecibidas(int socketUMC, t_list* listaDeLibres, t_list* listaDeOcupados) {
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
				int interpretado=interpretarMensajeRecibido(buffer, operacion, socketUMC, listaDeLibres, listaDeOcupados);
				if(!interpretado){
					return;
				}
			}
		}

	} while (1);
}

int crearArchivoControlMemoria() {
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

int interpretarMensajeRecibido(char* buffer,int op, int socket, t_list* listaDeLibres, t_list* listaDeOcupados){
	/*espacioOcupado* aBorrar;
	espacioOcupado*aEscribir;
	espacioOcupado* aLeer;*/
	uint32_t paginaSolicitada;
	//uint32_t cantidadPaginasAEscribir;
	uint32_t resultado=paginaSolicitada;
	uint32_t  pid=0;

	t_buffer_tamanio * buffer_tamanio;

	switch(op){
		case NUEVOPROGRAMA:
			resultado=asignarMemoria(pid, paginaSolicitada, listaDeLibres, listaDeOcupados);
			if (!resultado)
			{
				buffer_tamanio = serializarUint32(pid);
				int enviado=enviarDatos(socket, buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, RECHAZAR_PROCESO_A_UMC,SWAP);
				if(enviado==-1)
				{
					printf("No se pudo enviar mensaje a UMC ");
					log_error(ptrLog, "No se pudo enviar mensaje a UMC");
				}
				return 0;
			}
		break;
		case FINALIZARPROGRAMA:
			/*aBorrar=ocupado;
			while (aBorrar && aBorrar->pid != pid) aBorrar= aBorrar->sgte;
			if(!aBorrar)
			{
				printf ("El proceso %d no se encuentra en el swap \n",pid);
				log_error(ptrLog, "El proceso %d no se encuentra en el swap",pid);
				buffer_tamanio = serializarUint32(pid);
				int enviado=enviarDatos(socket,buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, RECHAZAR_FINALIZAR_PROCESO_A_UMC,SWAP);
				if(enviado==-1)
				{
					printf("No se pudo enviar mensaje al ADM\n");
					log_error(ptrLog, "No se pudo enviar mensaje al ADM");
				}
				return 0;
			}
			//printf("Se libero la memoria del proceso de pid %u \n", aBorrar->pid);*/
			liberarMemoria(listaDeLibres, listaDeOcupados, pid);
		break;
		case LEER:
			sleep(retardoCompactacion);
			/*aLeer=ocupado;
			while(aLeer && aLeer->pid !=pid) aLeer=aLeer->sgte;
			if(!aLeer)
			{
				printf ("El proceso %d no se encuentra en el swap \n",pid);
				buffer_tamanio = serializarUint32(pid);
				int enviado=enviarDatos(socket,buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, RECHAZAR_LEER_PROCESO_A_UMC,SWAP);
				if(enviado==-1)
				{
					printf("No se pudo enviar mensaje a UMC\n");
					log_error(ptrLog, "No se pudo enviar mensaje a UMC");
				}
				return 0;
			}*/
			char* leido= leerProceso(paginaSolicitada, pid, listaDeOcupados);
			enviarDatos(socket, leido, sizeof(leido),ENVIAR_PAGINA_A_UMC,SWAP);
			free(leido);
		break;
		case ESCRIBIR:
			sleep(retardoCompactacion);
			/*aEscribir=ocupado;
			while(aEscribir && aEscribir->pid != pid) aEscribir=aEscribir->sgte;
			if(!aEscribir)
			{
				printf ("El proceso %d no se encuentra en el swap \n",pid);
				log_error(ptrLog, "El proceso %d no se encuentra en el swap \n",pid);
				buffer_tamanio = serializarUint32(pid);
				int enviado=enviarDatos(socket,buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer, RECHAZAR_ESCRIBIR_PROCESO_A_UMC,SWAP);
				if(enviado==-1)
				{
					printf("No se pudo enviar mensaje al UMC\n");
					log_error(ptrLog, "No se pudo enviar mensaje al UMC");
					cerrarSwap();
				}
				return 0;
			}*/
			escribirProceso(paginaSolicitada,buffer, listaDeOcupados,pid);
			buffer=NULL;
			break;

		break;
		default:
			printf("Mensaje de UMC no comprendido\n");
			log_error(ptrLog, "Mensaje de UMC no comprendido");
			//cerrarSwap();
			break;
	}
	//si llegó hasta acá es porque esta OK
	int i=-1;
	t_buffer_tamanio * bufferPagina = serializarUint32(resultado);
	i=enviarDatos(socket, bufferPagina->buffer, bufferPagina->tamanioBuffer, SUCCESS, SWAP);
	if(i==-1)
	{
		printf("No se pudo enviar mensaje de confirmacion a UMC\n");
		log_error(ptrLog, "No se pudo enviar mensaje de confirmacion a UMC");
		cerrarSwap();
	}
	return 1;
}

char* leerProceso(uint32_t pagina, uint32_t pid, t_list* listaDeOcupados)//pagALeer tiene como 0 a la pos inicial
{//leemos una pagina del archivo de swap
	/*char* buffer=NULL;
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
*/

	espacioOcupado* espacioDelProceso = encontrarLugarAsignadoAProceso(listaDeOcupados, pid);
	FILE* swap = fopen(nombre_archivo,"r");
	fseek(swap,tamanoPagina* (pagina+ espacioDelProceso->posicion_inicial),SEEK_SET);
	char* contLeido = malloc(tamanoPagina);
	int readed=fread(contLeido,tamanoPagina,1,swap);
	if(!readed){
		return 0;
	}
	espacioDelProceso->leido++;
	sleep(retardoCompactacion);
	log_info(ptrLog, "Se leyeron %d bytes en la página %d (número de byte inicial: %d) del proceso %d. Su contenido es: %s", strlen(contLeido), pagina, (pagina*tamanoPagina), pid, contLeido);
	fclose(swap);
	return contLeido;
}
void escribirProceso(int paginaProceso, char* info , t_list* listaDeOcupados, uint32_t pid)// 0 mal 1 bien. pagAEscribir comienza en 0
{//escribimos en el archivo de swap
	/*fseek(archivo_control,((aEscribir->posicion_inicial -1) * tamanoPagina) +  (pagAEscribir * tamanoPagina), SEEK_SET);
	fwrite(texto, sizeof(char), tamanoPagina, archivo_control);
	aEscribir->escrito= aEscribir->escrito +1;//el proceso lee una pagina y lo documentamos
	char bufferLogueo[tamanoPagina +1];//esto va a ser para meterle un \0 por las dudas
	strcpy(bufferLogueo, texto);
	bufferLogueo[tamanoPagina]='\0';
	log_info(ptrLog, "El proceso de pid %u escribe %u bytes comenzando en el byte %u y escribe: %s",
			aEscribir->pid, strlen(texto)*sizeof(char), ((aEscribir->posicion_inicial -1) * tamanoPagina) +
			(pagAEscribir * tamanoPagina), bufferLogueo);
	return;*/
	int fd = open(nombre_archivo,O_RDWR);
	char* data =mmap((caddr_t)0, cantidadPaginas*tamanoPagina, PROT_READ|PROT_WRITE ,MAP_SHARED, fd, 0);
	espacioOcupado* espacioDelProceso = encontrarLugarAsignadoAProceso(listaDeOcupados, pid);
	memset(data+((espacioDelProceso->posicion_inicial+paginaProceso)* tamanoPagina), '\0', tamanoPagina);
	memcpy(data+((espacioDelProceso->posicion_inicial+paginaProceso)* tamanoPagina), info, strlen(info) );
	if (msync(data, cantidadPaginas*tamanoPagina, MS_SYNC) == -1)
	{
		log_info(ptrLog,"No se pudo acceder al archivo en disco");
	}
	munmap(data, cantidadPaginas*tamanoPagina);
	close(fd);
	espacioDelProceso->escrito++;
	sleep(retardoCompactacion);
	log_info(ptrLog,
			"Se escribieron %d bytes en la página %d (número de byte inicial: %d) del proceso %d. Su contenido es: %s.",
			strlen(info),
			paginaProceso,
			(paginaProceso*tamanoPagina),
			pid,
			info);
}
void inicializarArchivo(void) { //lo llenamos con el caracter correspondiente
	int tamanoArchivo = (cantidadPaginas) * (tamanoPagina);
	char* s = string_repeat('\0', tamanoArchivo);
	fprintf(archivo_control, "%s", s);
	free(s);
	return;
}

//funciones para asignar memoria
int asignarMemoria(uint32_t pid, uint32_t cantidad_paginas, t_list* listaDeLibres, t_list* listaDeOcupados) { //le damos memoria al proceso nuevo
	/*int inicio;
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
	uint32_t posicion = agregarOcupado(pid, cantidad_paginas, inicio);
	log_info(ptrLog,
			"Se asignan %u bytes de memoria al proceso de pid %u desde el byte %u",
			cantidad_paginas * tamanoPagina, pid, (inicio - 1) * tamanoPagina); //CANTIDAD DE BYTES ES NPAG*TMANIOPAGINA
	return posicion;
	*/

	if(!hayEspacio(cantidad_paginas,listaDeLibres))
	{
		return 0;
	}
	espacioLibre* hueco = encontrarHueco(listaDeLibres, cantidad_paginas);

	if(hueco == NULL)
	{
		desfragmentar(listaDeOcupados, listaDeLibres);
		hueco = encontrarHueco(listaDeLibres, cantidad_paginas);
	}

	uint32_t comienzoProceso = ocupar(hueco,listaDeLibres, listaDeOcupados, pid, cantidad_paginas);
	log_info(ptrLog,
			"Se ubicó correctamente el proceso %d partiendo del byte %d y ocupando %d bytes.",
			pid,
			(comienzoProceso*tamanoPagina), (cantidad_paginas*tamanoPagina));
	return 1;
}
espacioLibre* encontrarHueco(t_list* listaDeLibres, int pagsRequeridas)
{
	bool condicionBloqueEncontrado(espacioLibre* hueco) {
			return (hueco->cantidad_paginas >= pagsRequeridas);
	}

	espacioLibre* hueco;
	hueco=list_find( listaDeLibres, (void*) condicionBloqueEncontrado);
	if(hueco == NULL)
	{
		log_info(ptrLog,"Fallé al encontrar hueco.");

	}
	return hueco;
}
int espacioLibrePorHueco(espacioLibre* listaLibre)
{
	return listaLibre->cantidad_paginas;
}

void iniciarProceso(int comienzo, int pags)
{		int fd = open(nombre_archivo,O_RDWR);
		char* data =mmap((caddr_t)0, cantidadPaginas*tamanoPagina, PROT_READ|PROT_WRITE ,MAP_SHARED, fd, 0);
		memset(data+(comienzo*tamanoPagina),'\0', pags*tamanoPagina);
		if (msync(data, cantidadPaginas*tamanoPagina, MS_SYNC) == -1)
		{
		 log_info(ptrLog,"No se pudo acceder al archivo de disco");
		}
		munmap(data, cantidadPaginas*tamanoPagina);
		close(fd);
}
uint32_t ocupar(espacioLibre* hueco, t_list* listaDeLibres, t_list* listaDeOcupados, uint32_t pidRecibido,  int pagsRequeridas) { //cambia un espacio libre a ocupado
	/*espacioLibre* aux = libre;
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
	return;*/
		//VARIABLES PARA LA FUNCION BOOL ES ESTE HUECO Y PARA GUARDAR LOS VALORES DEL HUECO
		int comienzoAux= hueco->posicion_inicial;
		int cantAux = hueco ->cantidad_paginas;

		iniciarProceso(comienzoAux, pagsRequeridas);
		espacioOcupado* procesoNuevo=malloc(sizeof(espacioOcupado));
		procesoNuevo-> pid = pidRecibido;
		procesoNuevo-> posicion_inicial = comienzoAux;
		procesoNuevo-> cantidad_paginas = pagsRequeridas;
		procesoNuevo-> leido = 0;
		procesoNuevo-> escrito = 0;
		list_add(listaDeOcupados, procesoNuevo);

		bool esEsteHueco (espacioLibre* huecos)
		{

			return (huecos->cantidad_paginas== cantAux && huecos->posicion_inicial == comienzoAux);
		}

		if(pagsRequeridas == hueco->cantidad_paginas)
		{
			list_remove_by_condition(listaDeLibres,(void*) esEsteHueco);
		}
		else
		{
			hueco->posicion_inicial = comienzoAux +pagsRequeridas;
			hueco->cantidad_paginas = cantAux-pagsRequeridas;
		}
		return procesoNuevo->posicion_inicial;
}

/*int alcanzanPaginas(int pagsNecesarias) //0 no alcanzan, 1 alcanzan
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
}*/

void ordenarLista(t_list* listaDeLibres)
{
	bool compararComienzos(espacioLibre* hueco1, espacioLibre* hueco2)
	{
		return(hueco1->posicion_inicial < hueco2->posicion_inicial);
	}
	list_sort(listaDeLibres, (void*) compararComienzos);
}
int hayEspacio(/*int espacio*/ int cantidadPaginasRequeridas, t_list* listaDeLibres) //espacio esta en paginas
{
	/*//te dice si hay un nodo libre con es el espacio requerido
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
	*/

	t_list* libreMapeado= list_map(listaDeLibres, (void*) *espacioLibrePorHueco);
	int auxSuma=0;
	while(libreMapeado->head!=NULL)
	{
		auxSuma +=(int)libreMapeado->head->data;
		libreMapeado->head= libreMapeado->head->next;

	}
	list_destroy(libreMapeado);
	return cantidadPaginasRequeridas <= auxSuma;
}
void desfragmentar(t_list* listaDeOcupados, t_list* listaDeLibres) { //movemos los nodos ocupados y juntamos los espacios libres en uno solo
	/*int sizeLibres = 0;
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
	return;*/
	ordenarLista(listaDeOcupados);
	int fd = open(nombre_archivo,O_RDWR);
	char* data =mmap((caddr_t)0, cantidadPaginas*tamanoPagina, PROT_READ|PROT_WRITE ,MAP_SHARED, fd, 0);
	t_link_element* aux = listaDeOcupados->head;

	if(!list_is_empty(listaDeOcupados))
	{
		int paginasOcupadas = 0;
		espacioOcupado* actual= aux->data;
		memcpy(data,data+(tamanoPagina*(actual->posicion_inicial)), tamanoPagina*(actual->cantidad_paginas)); //COPIAR EL PRIMERO AL PRINCIPIO
		actual->posicion_inicial=0;
		int paginasUltimo= actual->cantidad_paginas;
		paginasOcupadas+=paginasUltimo;
		aux= aux->next;

		while(aux!=NULL)

		{ actual=aux->data;
		  memcpy(data+(tamanoPagina*paginasOcupadas),data+(tamanoPagina*(actual->posicion_inicial)), tamanoPagina*(actual->cantidad_paginas)); //COPIAR EL PRIMERO AL PRINCIPIO
		  actual->posicion_inicial= paginasOcupadas;
		  paginasUltimo= actual->cantidad_paginas;
		  paginasOcupadas+=paginasUltimo;
		  aux = aux->next;
		}
		list_clean(listaDeLibres);
		espacioLibre* huecoLibre = malloc(sizeof(espacioLibre));
		huecoLibre->posicion_inicial = paginasOcupadas;
		huecoLibre->cantidad_paginas = cantidadPaginas-paginasOcupadas;
		list_add(listaDeLibres, huecoLibre);

		memset(data+(huecoLibre->posicion_inicial*tamanoPagina),'\0', tamanoPagina*huecoLibre->cantidad_paginas);// RELLENAR CON \0

		if (msync(data, tamanoPagina*paginasOcupadas, MS_SYNC) == -1)
		{log_info(ptrLog,"No se pudo acceder al archvo en disco");
		}
		munmap(data, tamanoPagina*paginasOcupadas);
		close(fd);
		}

	sleep(retardoCompactacion);

	log_info(ptrLog, "La compactación finalizó.");
}

/*void unirBloquesLibres(void) { //une dos nodos libres en uno solo mas grande
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
}*/

void moverInformacion(int inicioDe, int cantidad_paginass, int inicioA) // puse unos -1 alguien que me confime que tiene sentido
{ //intercambia lo escrito en el swap para cuando movemos un nodo ocupado al desfragmentar
	char buffer[cantidad_paginass * tamanoPagina]; //creamos el buffer
	fseek(archivo_control, (inicioDe - 1) * tamanoPagina, SEEK_SET); //vamos al inicio de ocupado
	int readed=fread(buffer, sizeof(char), (cantidad_paginass * tamanoPagina),
			archivo_control);//leemos
	if(!readed){
		return;
	}
	fseek(archivo_control, (inicioA - 1) * tamanoPagina, SEEK_SET); //vamos a libre
	fwrite(buffer, sizeof(char), (cantidad_paginass * tamanoPagina),
			archivo_control); //escribimos
	return; //en el "nuevo" libre ahora hay basura
}

/*int agregarOcupado(uint32_t pid, uint32_t cantidad_paginas,int posicion_inicial) //LOS NODOS OCUPADOS SE APILAN SIN ORDEN
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
	return nuevo->posicion_inicial;
}
*/
//funciones para liberar memoria
espacioOcupado* encontrarLugarAsignadoAProceso(t_list* listaDeOcupados, uint32_t pidRecibido)
{
	bool esEsteElProceso(espacioOcupado* proceso)
	{
		return(proceso->pid== pidRecibido);
	}

	return list_find(listaDeOcupados, (void*) esEsteElProceso);
}
void consolidarHueco(t_list* listaDeLibres)
{
	ordenarLista(listaDeLibres);
	t_link_element* aux = listaDeLibres->head;
	while(aux!=NULL)
		{
		espacioLibre* actual= aux->data;
		int comienzoActual= actual->posicion_inicial;
		int paginasActual= actual->cantidad_paginas;
		int terminaActual/*+1*/= comienzoActual+ paginasActual;
		if(aux->next!=NULL){
		espacioLibre* siguiente= aux->next->data;
		int comienzoSig= siguiente->posicion_inicial;
			if(terminaActual==comienzoSig)
			{
				actual->cantidad_paginas= paginasActual+siguiente->cantidad_paginas;

				t_link_element* temp= aux->next;
				aux->next=temp->next;
				free(temp);
				listaDeLibres->elements_count--;
			}
		}
		aux=aux->next;

		}
}
void sacarDelArchivo(int comienzoProceso, int paginas){

 int archivo = open(nombre_archivo, O_RDWR);
 char* data =mmap((caddr_t)0, cantidadPaginas*tamanoPagina , PROT_READ|PROT_WRITE ,MAP_SHARED, archivo, 0);
 memset(data+(tamanoPagina*comienzoProceso),'\0', paginas*tamanoPagina);

 if (msync(data, paginas*tamanoPagina, MS_SYNC) == -1)
 {
  log_info(ptrLog,"No se pudo acceder al archivo en disco");
 }
 munmap(data, paginas*tamanoPagina);
 close(archivo);
}
void liberarMemoria(t_list* listaDeLibres, t_list* listaDeOcupados, uint32_t pidRecibido) { //se va un proceso y borramos su nodo ocupado y agregamos un libre
	/*int anteriorVar = anterior(aBorrar);
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
	return 0;*/
	espacioOcupado* proceso= encontrarLugarAsignadoAProceso(listaDeOcupados, pidRecibido);
	espacioLibre* huecoProceso=malloc(sizeof(espacioLibre));
	int comienzoAux = proceso->posicion_inicial;
	int paginasAux = proceso->cantidad_paginas;
	huecoProceso->posicion_inicial= comienzoAux;
	huecoProceso->cantidad_paginas= paginasAux;
	list_add(listaDeLibres,huecoProceso);
	consolidarHueco(listaDeLibres);

	bool esEsteElProceso(espacioOcupado* proceso)
		{
			return(proceso->pid== pidRecibido);
		}

	sacarDelArchivo(comienzoAux, paginasAux);
	list_remove_by_condition(listaDeOcupados,  (void*) esEsteElProceso);
	log_info(ptrLog, "Se quitó el proceso %d, que comenzaba en el byte %d. %d bytes liberados.", pidRecibido, (comienzoAux*tamanoPagina), (paginasAux*tamanoPagina));
}

/*void borrarNodoOcupado(espacioOcupado* aBorrar)
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

/*int anterior(espacioOcupado* nodo) //0 no hay nada 1 libre 2 ocupado
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
}*/

t_list* crearListaLibre(int cantPaginas)
{
 t_list* listaLibres = list_create();
 espacioLibre* particionCompleta=malloc(sizeof(espacioLibre));
 particionCompleta->posicion_inicial = 0;
 particionCompleta->cantidad_paginas = cantPaginas;
 list_add(listaLibres, particionCompleta);
 return listaLibres;
}

t_list* crearListaOcupados()
{
	 t_list* listaOcupados = list_create();
	 return listaOcupados;
}

int main() {
	if (init()) {
		t_list* listaDeOcupados = crearListaOcupados();
		t_list* listaDeLibres = crearListaLibre(cantidadPaginas);
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
			manejarConexionesRecibidas(socketUMC,listaDeLibres, listaDeOcupados);


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
