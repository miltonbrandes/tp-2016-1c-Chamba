/*
 * PrimitivasAnSISOP.c
 *
 *  Created on: 17/5/2016
 *      Author: utnso
 */
#include "PrimitivasAnSISOP.h"
#include <sockets/EscrituraLectura.h>
#include <sockets/OpsUtiles.h>

#define TAMANIO_VARIABLE 5
extern t_pcb* pcb;
extern int socketNucleo;
extern t_log* ptrLog;

t_posicion definirVariable(t_nombre_variable identificador_variable) {
	log_debug(ptrLog, "Llamada a definirVariable");
	return NULL;
}

t_posicion obtenerPosicionVariable(t_nombre_variable identificador_variable) {
	log_debug(ptrLog, "Llamada a obtenerPosicionVariable");
	return NULL;
}

t_valor_variable dereferenciar(t_posicion direccion_variable) {
	log_debug(ptrLog, "Llamada a dereferenciar");
	return NULL;
}

void asignar(t_posicion direccion_variable, t_valor_variable valor) {
	log_debug(ptrLog, "Llamada a asignar");
	return;
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable) {
	char* buffer;
	uint32_t id;
	t_valor_variable valor = 0;
	log_debug(ptrLog, "Obteniendo el valor de la variable compartida '%s'",
			variable);
	if (enviarDatos(socketNucleo, &variable, (uint32_t)(strlen(variable) + 1),	(uint32_t)LEER_VAR_COMPARTIDA, (uint32_t)CPU) < 0)
	return -1;
	buffer = recibirDatos(socketNucleo, NULL, &id);
	if (strlen(buffer) < 0)
		return -1;

	if (strcmp("ERROR", buffer) == 0) {

	} else {
		memcpy(&valor, buffer, sizeof(t_valor_variable));
		log_debug(ptrLog, "El valor de '%s' es %d", variable, valor);
		free(buffer);
		return valor;
	}
}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable,
		t_valor_variable valor) {
	int longitud = strlen(variable) + 1 + 8;
	char* buffer;
	log_debug(ptrLog, "Asignando el valor %d a la variable compartida '%s'",
			valor, variable);
	t_op_varCompartida* varCompartida = malloc(sizeof(t_op_varCompartida));
	varCompartida->longNombre = strlen(variable) + 1;
	varCompartida->nombre = malloc(strlen(variable) + 1);
	strcpy(varCompartida->nombre, variable);
	varCompartida->valor = valor;
	buffer = serializar_opVarCompartida(varCompartida);

	if (enviarDatos(socketNucleo, &buffer, (uint32_t)longitud, (uint32_t)ASIG_VAR_COMPARTIDA, (uint32_t)CPU) < 0)
	return -1;
	free(buffer);
	free(varCompartida->nombre);
	free(varCompartida);
	log_debug(ptrLog, "Valor asignado");
	return valor;
}

void irAlLabel(t_nombre_etiqueta etiqueta) {
	log_debug(ptrLog, "Llamada a irAlLabel");
}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_posicion donde_retornar) {
	log_debug(ptrLog, "Llamada a llamarFuncion");
}

void retornar(t_valor_variable retorno) {
	log_debug(ptrLog, "Llamada a retornar");
}

void imprimir(t_valor_variable valor_mostrar) {
	log_debug(ptrLog,
			"Enviando al kernel el valor %d que se mostrara por pantalla",
			valor_mostrar);
	char* buffer = malloc(sizeof(t_valor_variable));
	memcpy(buffer, &valor_mostrar, sizeof(t_valor_variable));
	int bytesEnviados = enviarDatos(socketNucleo, &buffer, (uint32_t)sizeof(t_valor_variable), (uint32_t)IMPRIMIR_VALOR, (uint32_t)CPU);
	log_debug(ptrLog, "Valor enviado");
	free(buffer);
//		return bytesEnviados;
}

void imprimirTexto(char* texto) {
	log_debug(ptrLog,
			"Enviando al kernel una cadena de texto que se mostrara por pantalla");
	texto = _string_trim(texto);
	log_trace(ptrLog, "La cadena es:\n%s", texto);
	int bytesEnviados = enviarDatos(socketNucleo, &texto, (uint32_t)(strlen(texto) + 1), (uint32_t)IMPRIMIR_TEXTO, (uint32_t)CPU);
	log_debug(ptrLog, "Cadena enviada");
//	return bytesEnviados;
}

void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo) {
	char* buffer;
	uint32_t operacion = IO;
	log_debug(ptrLog, "Se efectua la operacion de Entrada/Salida");
	t_dispositivo_io* op_IO = malloc(sizeof(t_dispositivo_io));
	op_IO->nombre = malloc(strlen(dispositivo) + 1);
	strcpy(op_IO->nombre, dispositivo);
	op_IO->tiempo = tiempo;
//	buffer = serializar_opIO(op_IO);
	int bytesEnviados = enviarDatos(socketNucleo, &buffer, (uint32_t)(8 + strlen(dispositivo) + 1), operacion, (uint32_t)CPU);
	log_debug(ptrLog, "Dispositivo '%s' con un tiempo de %d enviados al kernel",
			dispositivo, tiempo);
	free(buffer);
	free(op_IO->nombre);
	free(op_IO);
//	return bytesEnviados;
}

void wait(t_nombre_semaforo identificador_semaforo) {
	char* buffer;
	char op;
	uint32_t operacion;
	uint32_t id;
	log_debug(ptrLog, "Enviado al kernel funcion WAIT para el semaforo '%s'",
			identificador_semaforo);
	enviarDatos(socketNucleo, &identificador_semaforo, (uint32_t)(strlen(identificador_semaforo) + 1), WAIT, CPU);
	log_debug(ptrLog, "Esperando respuesta del kernel");
	buffer = recibirDatos(socketNucleo, NULL, &id);
	if (op == WAIT)
	log_debug(ptrLog,
			"El proceso queda bloqueado hasta que se haga un SIGNAL a '%s'",
			identificador_semaforo);
	free(buffer);
//	return bytesRecibidos;

}

void signal(t_nombre_semaforo identificador_semaforo) {
	log_debug(ptrLog, "Enviado al kernel funcion SIGNAL para el semaforo '%s'",
			identificador_semaforo);
	int bytesEnviados = enviarDatos(socketNucleo, &identificador_semaforo, (uint32_t)(strlen(identificador_semaforo) + 1), (uint32_t)SIGNAL, (uint32_t)CPU);
//	return bytesEnviados;
}

