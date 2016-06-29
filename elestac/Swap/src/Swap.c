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
#include "Swap.h"
//Socket que recibe conexiones de Nucleo y CPU
int socketReceptorUMC;
//Archivo de Log
t_log* ptrLog;

espacioLibre* libre;
espacioOcupado* ocupado;
FILE* archivo_control;
t_config* config;
t_list* listaOcupados;
t_list* listaLibres;
char * holes;

//Variables de configuracion
int puertoTCPRecibirConexiones;
int retardoCompactacion, retardoAcceso;
char* nombre_swap;
int tamanoPagina;
int cantidadPaginas;

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
			log_info(ptrLog,
					"Se creo correctamente el archivo de control de memoria");
		} else {
			log_error(ptrLog, "Fallo al crear el archivo, cerrando swap.");
			cerrarSwap();
		}

		log_info(ptrLog, "Esperando conexiones");
		int socketUMC = AceptarConexionCliente(socketReceptorUMC);
		log_info(ptrLog, "Conexion de UMC aceptada");

		manejarConexionesRecibidas(socketUMC, listaDeLibres, listaDeOcupados);

	} else {
		log_info(ptrLog, "El SWAP no pudo inicializarse correctamente");
		return -1;
	}
	log_info(ptrLog, "Proceso SWAP finalizado.");
	free(config);
	free(listaOcupados);
	free(listaLibres);
	free(holes);
	finalizarConexion(socketReceptorUMC);
	fclose(archivo_control);
	eliminarListas();
	//log_destroy(ptrLog);
	return EXIT_SUCCESS;
}

////FUNCIONES SWAP////

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
			nombre_swap = config_get_string_value(config, "NOMBRE_SWAP");
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

		if (config_has_property(config, "TAMANIO_PAGINA")) {
			tamanoPagina = config_get_int_value(config, "TAMANIO_PAGINA");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave TAMANIO_PAGINA");
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

		if (config_has_property(config, "RETARDO_ACCESO")) {
			retardoAcceso = config_get_int_value(config, "RETARDO_ACCESO");
		} else {
			log_info(ptrLog,
					"El archivo de configuracion no contiene la clave RETARDO_ACCESO");
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

void manejarConexionesRecibidas(int socketUMC, t_list* listaDeLibres,
		t_list* listaDeOcupados) {
	do {
		log_info(ptrLog, "Esperando recibir alguna peticion de UMC");
		uint32_t id;
		uint32_t operacion;

		char * buffer = recibirDatos(socketUMC, &operacion, &id);

		if (strcmp("ERROR", buffer) == 0) {
			log_info(ptrLog,
					"Ocurrio un error al Leer datos de UMC. Finalizo la conexion");
			finalizarConexion(socketUMC);
			return;
		} else {
			int interpretado = interpretarMensajeRecibido(buffer, operacion,
					socketUMC, listaDeLibres, listaDeOcupados);
			//if(!interpretado){
			//	return;
			//}
		}

	} while (1);
}

int crearArchivoControlMemoria() {
	int tamanoArchivo = (cantidadPaginas) * (tamanoPagina);
	holes = malloc(tamanoArchivo);
	holes = string_repeat('\0', tamanoArchivo);

	int fd = open(nombre_swap, O_WRONLY | O_CREAT | O_TRUNC, 0666);

	write(fd, holes, tamanoArchivo);
	close(fd);

//	archivo_control = fopen(nombre_swap, "w+");
//	if (archivo_control == 0) {
//		printf("No se pudo crear el archivo en swap.c \n");

//		log_error(ptrLog, "Fallo al crear el archivo de swap");
//		cerrarSwap();
//	}
//	inicializarArchivo();
	libre = malloc(sizeof(espacioLibre)); //creamos el primero nodo libre (archivo entero)
	if (!libre) {
		log_error(ptrLog, "Fallo el malloc para la lista libres en Swap");
		cerrarSwap();
	}
	libre->sgte = NULL;
	libre->ant = NULL;
	libre->posicion_inicial = 0;
	libre->cantidad_paginas = cantidadPaginas;

//	fclose(archivo_control);

	return 1;
}

void cerrarSwap(void) //no cierra los sockets
{ //cerramos el swap ante un error
	log_info(ptrLog, "Proceso SWAP finalizado abruptamente.");
	fclose(archivo_control);
	eliminarListas();
	log_destroy(ptrLog);
	exit(-1);
}

int interpretarMensajeRecibido(char* buffer, int op, int socketUMC,
		t_list* listaDeLibres, t_list* listaDeOcupados) {
	uint32_t resultado;

	t_buffer_tamanio * buffer_tamanio;
	t_check_espacio * checkEspacio;
	t_solicitud_pagina * solicitudPagina;
	t_pagina_de_swap * paginaDeSwap;
	t_escribir_en_swap * escritura;
	t_finalizar_programa * finalizarPrograma;

	switch (op) {
	case NUEVOPROGRAMA:
		checkEspacio = deserializarCheckEspacio(buffer);
		log_info(ptrLog,
				"UMC Solicita la creacion de espacio para el Proceso con PID %d",
				checkEspacio->pid);
		resultado = asignarMemoria(checkEspacio->pid,
				checkEspacio->cantidadDePaginas, listaDeLibres,
				listaDeOcupados);
		if (!resultado) {
			buffer_tamanio = serializarUint32(ERROR);
			int enviado = enviarDatos(socketUMC, buffer_tamanio->buffer,
					buffer_tamanio->tamanioBuffer, RECHAZAR_PROCESO_A_UMC,
					SWAP);
			if (enviado <= 0) {
				log_error(ptrLog,
						"Ocurrio un error al Notificar a UMC que se rechazo el Proceso con PID %d.",
						checkEspacio->pid);
			}
			free(buffer_tamanio->buffer);
			free(buffer_tamanio);
			free(checkEspacio);
			free(buffer);
			return 0;
		} else {
			log_info(ptrLog,
					"Proceso con PID %d puede almacenarse",
					checkEspacio->pid);
			buffer_tamanio = serializarUint32(SUCCESS);
			int bytesEnviados = enviarDatos(socketUMC, buffer_tamanio->buffer,
					buffer_tamanio->tamanioBuffer, ACEPTAR_PROCESO_A_UMC, SWAP);
			if (bytesEnviados <= 0) {
				log_error(ptrLog,
						"Ocurrio un error al Notificar a UMC que se acepto el Proceso con PID %d.",
						checkEspacio->pid);
			}
			free(buffer_tamanio->buffer);
			free(buffer_tamanio);
			free(checkEspacio);
			free(buffer);
			return 1;
		}
		break;
	case FINALIZARPROGRAMA:
		usleep(retardoAcceso * 1000);
		finalizarPrograma = deserializarFinalizarPrograma(buffer);
		log_info(ptrLog, "UMC Solicita la finalizacion del Proceso con PID %i",
				finalizarPrograma->programID);
		liberarMemoria(listaDeLibres, listaDeOcupados,
				finalizarPrograma->programID);

		buffer_tamanio = serializarUint32(SUCCESS);
		int bytesEnviadosFinalizar = enviarDatos(socketUMC,
				buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer,
				FINALIZARPROGRAMA, SWAP);
		if (bytesEnviadosFinalizar <= 0) {
			log_error(ptrLog,
					"Ocurrio un error al Notificar a UMC que se finalizo el Proceso con PID %d.",
					finalizarPrograma->programID);
		}
		free(buffer);
		free(finalizarPrograma);
		free(buffer_tamanio->buffer);
		free(buffer_tamanio);
		break;
	case LEER:
		usleep(retardoAcceso * 1000);
		solicitudPagina = deserializarSolicitudPagina(buffer);
		log_info(ptrLog,
				"UMC Solicita la lectura de la Pagina %d del Proceso con PID %d",
				solicitudPagina->paginaProceso, solicitudPagina->pid);

		char* leido = leerProceso(solicitudPagina->paginaProceso,
				solicitudPagina->pid, listaDeOcupados);
		paginaDeSwap = malloc(tamanoPagina);
		paginaDeSwap->paginaSolicitada = leido;

		free(solicitudPagina);

		buffer_tamanio = serializarPaginaDeSwap(paginaDeSwap, tamanoPagina);
		int bytesEnviados = enviarDatos(socketUMC, buffer_tamanio->buffer,
				buffer_tamanio->tamanioBuffer, ENVIAR_PAGINA_A_UMC, SWAP);
		if (bytesEnviados <= 0) {
			log_error(ptrLog,
					"Ocurrio un error al enviar a UMC la Pagina %d del Proceso con PID %d.",
					solicitudPagina->paginaProceso, solicitudPagina->pid);
		}
		free(buffer);
		free(paginaDeSwap);
		free(buffer_tamanio->buffer);
		free(buffer_tamanio);
		free(leido);
		break;
	case ESCRIBIR:
		usleep(retardoAcceso * 1000);
		escritura = deserializarEscribirEnSwap(buffer);
		log_info(ptrLog,
				"UMC Solicita la escritura de la Pagina %d del Proceso con PID %d",
				escritura->paginaProceso, escritura->pid);

		escribirProceso(escritura->paginaProceso, escritura->contenido,
				listaDeOcupados, escritura->pid);
		free(escritura->contenido);
		free(escritura);

		buffer_tamanio = serializarUint32(SUCCESS);
		int bytesEnviadosEscritura = enviarDatos(socketUMC,
				buffer_tamanio->buffer, buffer_tamanio->tamanioBuffer,
				ESCRITURA_OK_UMC, SWAP);
		if (bytesEnviadosEscritura <= 0) {
			log_error(ptrLog,
					"Ocurrio un error al Notificar a UMC que se escribio la Pagina %d del Proceso con PID %d.",
					escritura->paginaProceso, escritura->pid);
		}
		free(buffer_tamanio->buffer);
		free(buffer_tamanio);
		free(buffer);
		break;
		break;
	default:
		log_error(ptrLog, "Mensaje de UMC no comprendido");
		free(buffer);
		return 0;
		break;
	}

	return 1;
}

char* leerProceso(uint32_t pagina, uint32_t pid, t_list* listaDeOcupados) //pagALeer tiene como 0 a la pos inicial
{ //leemos una pagina del archivo de swap
	espacioOcupado* espacioDelProceso = encontrarLugarAsignadoAProceso(
			listaDeOcupados, pid);
	FILE* swap = fopen(nombre_swap, "r");
	fseek(swap, tamanoPagina * (pagina + espacioDelProceso->posicion_inicial),
			SEEK_SET);
	char* contLeido = calloc(1, tamanoPagina);
	int readed = fread(contLeido, tamanoPagina, 1, swap);
	if (!readed) {
		return 0;
	}
	espacioDelProceso->leido++;
	log_info(ptrLog,
			"Se leyeron %d bytes en la página %d (número de byte inicial: %d) del proceso %d. Su contenido es: %s",
			tamanoPagina, pagina, (pagina * tamanoPagina), pid, contLeido);
	fclose(swap);
	return contLeido;
}

void escribirProceso(int paginaProceso, char* info, t_list* listaDeOcupados,
		uint32_t pid) // 0 mal 1 bien. pagAEscribir comienza en 0
{ //escribimos en el archivo de swap
	int fd = open(nombre_swap, O_RDWR);
	char* data = mmap((caddr_t) 0, cantidadPaginas * tamanoPagina,
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	espacioOcupado* espacioDelProceso = encontrarLugarAsignadoAProceso(
			listaDeOcupados, pid);
	memset(
			data
					+ ((espacioDelProceso->posicion_inicial + paginaProceso)
							* tamanoPagina), '\0', tamanoPagina);
	memcpy(
			data
					+ ((espacioDelProceso->posicion_inicial + paginaProceso)
							* tamanoPagina), info, tamanoPagina);
	if (msync(data, cantidadPaginas * tamanoPagina, MS_SYNC) == -1) {
		log_info(ptrLog, "No se pudo acceder al archivo en disco");
	}
	munmap(data, cantidadPaginas * tamanoPagina);
	close(fd);
	espacioDelProceso->escrito++;

	char * escrito = malloc(tamanoPagina + 1);
	memcpy(escrito, info, tamanoPagina);
	log_info(ptrLog,
			"Se escribieron %d bytes en la página %d (número de byte inicial: %d) del proceso %d. Su contenido es: %s.",
			tamanoPagina, paginaProceso, (paginaProceso * tamanoPagina), pid,
			escrito);
	free(escrito);
}

//funciones para asignar memoria
int asignarMemoria(uint32_t pid, uint32_t cantidad_paginas,
		t_list* listaDeLibres, t_list* listaDeOcupados) { //le damos memoria al proceso nuevo
	log_info(ptrLog, "Comienzo a chequear si hay espacio para el Proceso %d",
			pid);
	if (!hayEspacio(cantidad_paginas, listaDeLibres)) {
		log_info(ptrLog, "No hay espacio para el Proceso %d", pid);
		return 0;
	}

	log_info(ptrLog, "Hay espacio para el Proceso %d", pid);
	espacioLibre* hueco = encontrarHueco(listaDeLibres, cantidad_paginas);

	if (hueco == NULL) {
		log_info(ptrLog,
				"Necesito desfragmentar / compactar memoria para alojar el Proceso %d",
				pid);
		desfragmentar(listaDeOcupados, listaDeLibres);
		usleep(retardoCompactacion * 1000);
		hueco = encontrarHueco(listaDeLibres, cantidad_paginas);
	}

	log_info(ptrLog, "Comienzo a ocupar el espacio para el Proceso %d", pid);
	uint32_t comienzoProceso = ocupar(hueco, listaDeLibres, listaDeOcupados,
			pid, cantidad_paginas);
	log_info(ptrLog,
			"Se ubicó correctamente el proceso %d partiendo del byte %d y ocupando %d bytes.",
			pid, (comienzoProceso * tamanoPagina),
			(cantidad_paginas * tamanoPagina));
	return 1;
}

espacioLibre* encontrarHueco(t_list* listaDeLibres, int pagsRequeridas) {
	bool condicionBloqueEncontrado(espacioLibre* hueco) {
		return (hueco->cantidad_paginas >= pagsRequeridas);
	}

	espacioLibre* hueco;
	hueco = list_find(listaDeLibres, (void*) condicionBloqueEncontrado);
	if (hueco == NULL) {
		log_info(ptrLog, "Fallé al encontrar hueco.");

	}
	return hueco;
}

int espacioLibrePorHueco(espacioLibre* listaLibre) {
	return listaLibre->cantidad_paginas;
}

void iniciarProceso(int comienzo, int pags) {
	int fd = open(nombre_swap, O_RDWR);

	char* data = mmap((caddr_t) 0, cantidadPaginas * tamanoPagina,
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data != NULL) {
		memset(data + (comienzo * tamanoPagina), '\0', pags * tamanoPagina);
		if (msync(data, (cantidadPaginas * tamanoPagina), MS_SYNC) == -1) {
			log_info(ptrLog, "No se pudo acceder al archivo de disco");
		} else {
			munmap(data, cantidadPaginas * tamanoPagina);
			close(fd);
		}
	} else {
//		log_info(ptrLog, "Data NULL o = -1 ?????");
	}

}

uint32_t ocupar(espacioLibre* hueco, t_list* listaDeLibres,
		t_list* listaDeOcupados, uint32_t pidRecibido, int pagsRequeridas) { //cambia un espacio libre a ocupado
	//VARIABLES PARA LA FUNCION BOOL ES ESTE HUECO Y PARA GUARDAR LOS VALORES DEL HUECO

	int comienzoAux = hueco->posicion_inicial;
	int cantAux = hueco->cantidad_paginas;

	iniciarProceso(comienzoAux, pagsRequeridas);
	espacioOcupado* procesoNuevo = malloc(sizeof(espacioOcupado));
	procesoNuevo->pid = pidRecibido;
	procesoNuevo->posicion_inicial = comienzoAux;
	procesoNuevo->cantidad_paginas = pagsRequeridas;
	procesoNuevo->leido = 0;
	procesoNuevo->escrito = 0;
	list_add(listaDeOcupados, procesoNuevo);

	bool esEsteHueco(espacioLibre* huecos) {

		return (huecos->cantidad_paginas == cantAux
				&& huecos->posicion_inicial == comienzoAux);
	}

	if (pagsRequeridas == hueco->cantidad_paginas) {
		list_remove_by_condition(listaDeLibres, (void*) esEsteHueco);
	} else {
		hueco->posicion_inicial = comienzoAux + pagsRequeridas;
		hueco->cantidad_paginas = cantAux - pagsRequeridas;
	}
	return procesoNuevo->posicion_inicial;
}

void ordenarLista(t_list* listaDeLibres) {
	bool compararComienzos(espacioLibre* hueco1, espacioLibre* hueco2) {
		return (hueco1->posicion_inicial < hueco2->posicion_inicial);
	}
	list_sort(listaDeLibres, (void*) compararComienzos);
}

int hayEspacio(int cantidadPaginasRequeridas, t_list* listaDeLibres) //espacio esta en paginas
{
	t_list* libreMapeado = list_map(listaDeLibres,
			(void*) *espacioLibrePorHueco);
	int auxSuma = 0;
	while (libreMapeado->head != NULL) {
		auxSuma += (int) libreMapeado->head->data;
		libreMapeado->head = libreMapeado->head->next;

	}
	list_destroy(libreMapeado);
	return cantidadPaginasRequeridas <= auxSuma;
}

void desfragmentar(t_list* listaDeOcupados, t_list* listaDeLibres) { //movemos los nodos ocupados y juntamos los espacios libres en uno solo
	ordenarLista(listaDeOcupados);
	int fd = open(nombre_swap, O_RDWR);
	char* data = mmap((caddr_t) 0, cantidadPaginas * tamanoPagina,
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	t_link_element* aux = listaDeOcupados->head;

	if (!list_is_empty(listaDeOcupados)) {
		int paginasOcupadas = 0;
		espacioOcupado* actual = aux->data;
		memcpy(data, data + (tamanoPagina * (actual->posicion_inicial)),
				tamanoPagina * (actual->cantidad_paginas)); //COPIAR EL PRIMERO AL PRINCIPIO
		actual->posicion_inicial = 0;
		int paginasUltimo = actual->cantidad_paginas;
		paginasOcupadas += paginasUltimo;
		aux = aux->next;

		while (aux != NULL)

		{
			actual = aux->data;
			memcpy(data + (tamanoPagina * paginasOcupadas),
					data + (tamanoPagina * (actual->posicion_inicial)),
					tamanoPagina * (actual->cantidad_paginas)); //COPIAR EL PRIMERO AL PRINCIPIO
			actual->posicion_inicial = paginasOcupadas;
			paginasUltimo = actual->cantidad_paginas;
			paginasOcupadas += paginasUltimo;
			aux = aux->next;
		}
		list_clean(listaDeLibres);
		espacioLibre* huecoLibre = malloc(sizeof(espacioLibre));
		huecoLibre->posicion_inicial = paginasOcupadas;
		huecoLibre->cantidad_paginas = cantidadPaginas - paginasOcupadas;
		list_add(listaDeLibres, huecoLibre);

		memset(data + (huecoLibre->posicion_inicial * tamanoPagina), '\0',
				tamanoPagina * huecoLibre->cantidad_paginas); // RELLENAR CON \0

		if (msync(data, tamanoPagina * paginasOcupadas, MS_SYNC) == -1) {
			log_info(ptrLog, "No se pudo acceder al archvo en disco");
		}
		munmap(data, tamanoPagina * paginasOcupadas);
		close(fd);
	}

	log_info(ptrLog, "La compactación finalizó.");
}

void moverInformacion(int inicioDe, int cantidad_paginass, int inicioA) // puse unos -1 alguien que me confime que tiene sentido
{ //intercambia lo escrito en el swap para cuando movemos un nodo ocupado al desfragmentar
	char buffer[cantidad_paginass * tamanoPagina]; //creamos el buffer
	fseek(archivo_control, (inicioDe - 1) * tamanoPagina, SEEK_SET); //vamos al inicio de ocupado
	int readed = fread(buffer, sizeof(char), (cantidad_paginass * tamanoPagina),
			archivo_control); //leemos
	if (!readed) {
		return;
	}
	fseek(archivo_control, (inicioA - 1) * tamanoPagina, SEEK_SET); //vamos a libre
	fwrite(buffer, sizeof(char), (cantidad_paginass * tamanoPagina),
			archivo_control); //escribimos
	return; //en el "nuevo" libre ahora hay basura
}

//funciones para liberar memoria
espacioOcupado* encontrarLugarAsignadoAProceso(t_list* listaDeOcupados,
		uint32_t pidRecibido) {
	bool esEsteElProceso(espacioOcupado* proceso) {
		return (proceso->pid == pidRecibido);
	}

	return list_find(listaDeOcupados, (void*) esEsteElProceso);
}

void consolidarHueco(t_list* listaDeLibres) {
	ordenarLista(listaDeLibres);
	t_link_element* aux = listaDeLibres->head;
	while (aux != NULL) {
		espacioLibre* actual = aux->data;
		int comienzoActual = actual->posicion_inicial;
		int paginasActual = actual->cantidad_paginas;
		int terminaActual/*+1*/= comienzoActual + paginasActual;
		if (aux->next != NULL) {
			espacioLibre* siguiente = aux->next->data;
			int comienzoSig = siguiente->posicion_inicial;
			if (terminaActual == comienzoSig) {
				actual->cantidad_paginas = paginasActual
						+ siguiente->cantidad_paginas;

				t_link_element* temp = aux->next;
				aux->next = temp->next;
				free(temp);
				listaDeLibres->elements_count--;
			}
		}
		aux = aux->next;

	}
}

void sacarDelArchivo(int comienzoProceso, int paginas) {

	int archivo = open(nombre_swap, O_RDWR);
	char* data = mmap((caddr_t) 0, cantidadPaginas * tamanoPagina,
			PROT_READ | PROT_WRITE, MAP_SHARED, archivo, 0);
	memset(data + (tamanoPagina * comienzoProceso), '\0',
			paginas * tamanoPagina);

	if (msync(data, paginas * tamanoPagina, MS_SYNC) == -1) {
		log_info(ptrLog, "No se pudo acceder al archivo en disco");
	}
	munmap(data, paginas * tamanoPagina);
	close(archivo);
}

void liberarMemoria(t_list* listaDeLibres, t_list* listaDeOcupados,
		uint32_t pidRecibido) { //se va un proceso y borramos su nodo ocupado y agregamos un libre
	espacioOcupado* proceso = encontrarLugarAsignadoAProceso(listaDeOcupados,
			pidRecibido);
	espacioLibre* huecoProceso = malloc(sizeof(espacioLibre));
	int comienzoAux = proceso->posicion_inicial;
	int paginasAux = proceso->cantidad_paginas;
	huecoProceso->posicion_inicial = comienzoAux;
	huecoProceso->cantidad_paginas = paginasAux;
	list_add(listaDeLibres, huecoProceso);
	consolidarHueco(listaDeLibres);

	bool esEsteElProceso(espacioOcupado* proceso) {
		return (proceso->pid == pidRecibido);
	}

	sacarDelArchivo(comienzoAux, paginasAux);
	list_remove_by_condition(listaDeOcupados, (void*) esEsteElProceso);
	log_info(ptrLog,
			"Se quitó el proceso %d, que comenzaba en el byte %d. %d bytes liberados.",
			pidRecibido, (comienzoAux * tamanoPagina),
			(paginasAux * tamanoPagina));
}

t_list* crearListaLibre(int cantPaginas) {
	listaLibres = list_create();
	espacioLibre* particionCompleta = malloc(sizeof(espacioLibre));
	particionCompleta->posicion_inicial = 0;
	particionCompleta->cantidad_paginas = cantPaginas;
	list_add(listaLibres, particionCompleta);
	return listaLibres;
}

t_list* crearListaOcupados() {
	listaOcupados = list_create();
	return listaOcupados;
}

