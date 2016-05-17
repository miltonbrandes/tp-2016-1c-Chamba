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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <commons/string.h>
typedef struct op_varCompartida {
	char* nombre;
	uint32_t longNombre;
	uint32_t valor;
} t_op_varCompartida;
enum CPU {
	QUANTUM = 2,
	IO,
	EXIT,
	IMPRIMIR_VALOR,
	IMPRIMIR_TEXTO,
	LEER_VAR_COMPARTIDA,
	ASIG_VAR_COMPARTIDA,
	WAIT,
	SIGNAL,
	SIGUSR
};
typedef struct
{
	uint32_t idVariable;
	uint32_t pagina;
	uint32_t offset;
	uint32_t size;
}t_variable;
typedef struct {
	char *nombre;
	int tiempo;
} t_dispositivo_io;
typedef struct {
	char* nombre;
	int valor;
} t_semaforo;
typedef struct {
	char *nombre;
	int valor;
} t_variable_compartida;
typedef struct{
	uint32_t pagina;
	uint32_t offset;
	uint32_t size;
}t_argumento;
typedef struct stack{
	uint32_t posicion;
	t_list* argumentos;
	t_list* variables;
	uint32_t direcretorno;
	t_argumento retVar;
}t_stack;
typedef struct pcb {
	uint32_t pcb_id;
	uint32_t codigo;
	t_list* ind_codigo;
	t_list* ind_stack;
	char* ind_etiq;
	uint32_t PC;
	uint32_t posicionPrimerPaginaCodigo;
} t_pcb;
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
t_puntero_instruccion irAlLabel(t_nombre_etiqueta etiqueta);
t_puntero_instruccion llamarFuncion(t_nombre_etiqueta etiqueta,	t_posicion donde_retornar, t_puntero_instruccion linea_en_ejecuccion);
t_puntero_instruccion retornar(t_valor_variable retorno);
int	imprimir(t_valor_variable valor_mostrar);
int	imprimirTexto(char*	texto);
int	entradaSalida(t_nombre_dispositivo dispositivo, int	tiempo);
int	wait(t_nombre_semaforo	identificador_semaforo);
int signal(t_nombre_semaforo identificador_semaforo);

#endif /* CPU_PRIMITIVAS_ANSISOP_H_ */
