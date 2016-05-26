/*
 * Swap.h
 *
 *  Created on: 25/5/2016
 *      Author: utnso
 */

#ifndef SRC_SWAP_H_
#define SRC_SWAP_H_

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

#endif /* SRC_SWAP_H_ */
