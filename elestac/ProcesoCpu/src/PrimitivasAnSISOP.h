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

//Tipos de datos
typedef uint32_t t_posicion;
typedef u_int32_t t_puntero;
typedef u_int32_t t_size;
typedef u_int32_t t_puntero_instruccion;
typedef char t_nombre_variable;
typedef int t_valor_variable;
typedef t_nombre_variable* t_nombre_semaforo;
typedef t_nombre_variable* t_nombre_etiqueta;
typedef  t_nombre_variable* t_nombre_compartida;
typedef  t_nombre_variable* t_nombre_dispositivo;
t_posicion definirVariable(t_nombre_variable identificador_variable);
t_posicion obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_valor_variable dereferenciar(t_posicion direccion_variable);
void asignar(t_posicion	direccion_variable,	t_valor_variable valor);
t_valor_variable obtenerValorCompartida(t_nombre_compartida	variable);
t_valor_variable asignarValorCompartida(t_nombre_compartida	variable, t_valor_variable valor);
void irAlLabel(t_nombre_etiqueta etiqueta);
void llamarFuncion(t_nombre_etiqueta etiqueta,	t_posicion donde_retornar);
void retornar(t_valor_variable retorno);
void imprimir(t_valor_variable valor_mostrar);
void imprimirTexto(char*	texto);
void entradaSalida(t_nombre_dispositivo dispositivo, int	tiempo);
void wait(t_nombre_semaforo	identificador_semaforo);
void signal(t_nombre_semaforo identificador_semaforo);

AnSISOP_funciones functions = {
		.AnSISOP_asignar = asignar,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,
		.AnSISOP_definirVariable = definirVariable,
		.AnSISOP_dereferenciar = dereferenciar,
		.AnSISOP_entradaSalida = entradaSalida,
		.AnSISOP_imprimir = imprimir,
		.AnSISOP_imprimirTexto = imprimirTexto,
		.AnSISOP_irAlLabel = irAlLabel,
		.AnSISOP_obtenerPosicionVariable = obtenerPosicionVariable,
		.AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
		.AnSISOP_retornar = retornar,
		.AnSISOP_llamarConRetorno = llamarFuncion
};

AnSISOP_kernel kernel_functions = {
		.AnSISOP_signal = signal,
		.AnSISOP_wait = wait
};

#endif /* CPU_PRIMITIVAS_ANSISOP_H_ */
