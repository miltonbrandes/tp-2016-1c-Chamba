/*
 * PrimitivasAnSISOP.h
 *
 *  Created on: 12/5/2016
 *      Author: utnso
 */

#ifndef CPU_PRIMITIVAS_ANSISOP_H_
#define CPU_PRIMITIVAS_ANSISOP_H_

#include <parser/parser.h>
#include <parser/sintax.h>
#include <parser/metadata_program.h>
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <commons/string.h>
#include <sockets/StructsUtiles.h>
#include "ProcesoCpu.h"

//Tipos de datos
typedef uint32_t t_posicion;
typedef uint32_t t_puntero;
typedef uint32_t t_size;
typedef uint32_t t_puntero_instruccion;
typedef char t_nombre_variable;
typedef int t_valor_variable;
typedef t_nombre_variable* t_nombre_semaforo;
typedef t_nombre_variable* t_nombre_etiqueta;
typedef  t_nombre_variable* t_nombre_compartida;
typedef  t_nombre_variable* t_nombre_dispositivo;
t_puntero definirVariable(t_nombre_variable identificador_variable);
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_valor_variable dereferenciar(t_puntero direccion_variable);
void asignar(t_puntero	direccion_variable,	t_valor_variable valor);
t_valor_variable obtenerValorCompartida(t_nombre_compartida	variable);
t_valor_variable asignarValorCompartida(t_nombre_compartida	variable, t_valor_variable valor);
void irAlLabel(t_nombre_etiqueta etiqueta);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void retornar(t_valor_variable retorno);
void imprimir(t_valor_variable valor_mostrar);
void imprimirTexto(char*	texto);
void entradaSalida(t_nombre_dispositivo dispositivo, int	tiempo);
void wait(t_nombre_semaforo	identificador_semaforo);
void ansisop_signal(t_nombre_semaforo identificador_semaforo);
extern int socketNucleo;
extern int socketNucleo;
extern int socketUMC;
extern uint32_t operacion;
extern bool huboStackOver;
void setPCB(t_pcb * pcbDeCPU);
void freePCBDePrimitivas();

#endif /* CPU_PRIMITIVAS_ANSISOP_H_ */
