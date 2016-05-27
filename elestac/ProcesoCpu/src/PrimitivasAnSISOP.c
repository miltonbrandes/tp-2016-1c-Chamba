/*
 * PrimitivasAnSISOP.c
 *
 *  Created on: 17/5/2016
 *      Author: utnso
 */
#include "PrimitivasAnSISOP.h"
#include <sockets/EscrituraLectura.h>
#include <sockets/OpsUtiles.h>

#define TAMANIO_VARIABLE 4
extern int socketNucleo;
extern t_log* ptrLog;
extern t_pcb* pcb;
extern int socketNucleo;
extern int socketUMC;


t_puntero definirVariable(t_nombre_variable identificador_variable) {
	//ver bien que devuelvo....
	t_posicion_stack posicionDevolver;
	log_debug(ptrLog, "Llamada a definirVariable de la variable, %c", identificador_variable);
	t_variable* nuevaVar = malloc(sizeof(t_variable));
	t_stack* lineaStack = list_get(pcb->ind_stack, pcb->numeroContextoEjecucionActualStack);
	//me fijo si el offset de la ultima + el tamaño superan o son iguales el tamaño de la pagina, si esto sucede, tengo que pasar a una pagina nueva
	if(pcb->stackPointer + TAMANIO_VARIABLE > tamanioPagina){
		nuevaVar->idVariable = identificador_variable;
		pcb->paginaStackActual++;
		nuevaVar->pagina = pcb->paginaStackActual;
		nuevaVar->size = TAMANIO_VARIABLE;
		nuevaVar->offset = 0;
		pcb->stackPointer += TAMANIO_VARIABLE;
		list_add(lineaStack->variables, nuevaVar);
		//devuelvo el offset desde el inicio de la nueva pagina
		posicionDevolver.pagina = nuevaVar->pagina;
		posicionDevolver.offset = nuevaVar->offset;
		posicionDevolver.size = nuevaVar->size;
	}else{//8
		nuevaVar->idVariable = identificador_variable;
		nuevaVar->pagina = pcb->paginaStackActual;
		nuevaVar->size = TAMANIO_VARIABLE;
		nuevaVar->offset = pcb->stackPointer;
		pcb->stackPointer+= TAMANIO_VARIABLE;
		list_add(lineaStack->variables, nuevaVar);
		posicionDevolver.pagina = nuevaVar->pagina;
		posicionDevolver.offset = nuevaVar->offset;
		posicionDevolver.size = nuevaVar->size;
	}
	//calculo el desplazamiento desde la primer pagina del stack hasta donde arranca mi nueva variable
	uint32_t posicionRet = ((posicionDevolver.pagina-pcb->primerPaginaStack)*tamanioPagina)+(posicionDevolver.pagina*tamanioPagina)+posicionDevolver.offset;
	free(nuevaVar);
	return posicionRet;
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable) {
	//ver bien que deberia devolver aca, como calcular el stack pointer...
	t_variable* variable = malloc(sizeof(t_variable));
	int i = 0;
	char* nom = malloc(2);
	nom[0] = identificador_variable;
	nom[1] = '\0';
	log_debug(ptrLog, "Se obtiene la posicion de '%c'", identificador_variable);
	//obtengo la linea del stack del contexto de ejecucion actual...
	t_stack* lineaActualStack = list_get(pcb->ind_stack, pcb->numeroContextoEjecucionActualStack);
	//me posiciono al inicio de esta linea del stack
	//me fijo cual variable de la lista coincide con el nombre que me estan pidiendo
	for(i = 0; i<list_size(lineaActualStack->variables); i++){

		variable = list_get(lineaActualStack->variables, i);
		if(variable->idVariable == identificador_variable ){
			log_debug(ptrLog, "La posicion de '%c' es %u", variable, variable->offset);
			//tengo que devolver la pag offset y size de la variable
			t_posicion_stack pos;
			pos.pagina = variable->pagina;
			pos.offset = variable->offset;
			pos.size = variable->size;
			//deberia devolver una estructura con las 3 cosas
			//ver bien aca que deberia devolver
			free(variable);
			free(nom);
			//calculo el desplazamiento desde la primer pagina del stack hasta donde arranca la variable a buscar
			uint32_t posicionRet = ((pos.pagina-pcb->primerPaginaStack)*tamanioPagina)+(pos.pagina*tamanioPagina)+pos.offset;
			return pos;
			log_debug(ptrLog, "Llamada a obtenerPosicionVariable");
		}
	}
	//devuelvo -1
	uint32_t posicionRetError = -1;
	free(variable);
	free(nom);
	return posicionRetError;
}

t_valor_variable dereferenciar(t_puntero direccion_variable) {

	//uint32_t operacion;
	//calculo el la posicion de la variable en el stack mediante el desplazamiento
	t_posicion_stack posicionRet;
	posicionRet.pagina = (direccion_variable/tamanioPagina)+pcb->primerPaginaStack;
	posicionRet.offset = direccion_variable%tamanioPagina;
	posicionRet.size = TAMANIO_VARIABLE;
			//((posicionDevolver.pagina-pcb->primerPaginaStack)*tamanioPagina)+(posicionDevolver.pagina*tamanioPagina)+posicionDevolver.offset;
	t_valor_variable valor = 0;
	log_debug(ptrLog, "Llamada a dereferenciar %d", direccion_variable);
	t_solicitarBytes* solicitar = malloc(sizeof(t_solicitarBytes));
	solicitar->pagina = posicionRet.pagina;
	solicitar->offset = TAMANIO_VARIABLE;
	solicitar->start = posicionRet.offset;
	char* buffer = enviarOperacion(LEER, solicitar, socketUMC);
	if(buffer[0] == -1){
		//operacion = ERROR;
		free(solicitar);
		return -1;
	}
	memcpy(&valor, buffer + 1, sizeof(t_valor_variable));
	log_debug(ptrLog, "El valor es %d", valor);
	free(solicitar);
	return valor;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor) {
	//corroborar que esto este bien
	log_debug(ptrLog, "Llamada a asignar en posicion %d y valor %d", direccion_variable, valor);
	t_enviarBytes* enviar = malloc(sizeof(t_enviarBytes));
	uint32_t operacion;
	//calculo el la posicion de la variable en el stack mediante el desplazamiento
	t_posicion_stack posicionRet;
	posicionRet.pagina = (direccion_variable/tamanioPagina)+pcb->primerPaginaStack;
	posicionRet.offset = direccion_variable%tamanioPagina;
	posicionRet.size = TAMANIO_VARIABLE;
	enviar->pagina = posicionRet.pagina;
	enviar->offset = posicionRet.offset;
	enviar->tamanio = TAMANIO_VARIABLE;
	enviar->pid = pcb->pcb_id;
	uint32_t valorVar = (uint32_t)valor;
	enviar->buffer = malloc(sizeof(uint32_t));
	memcpy(enviar->buffer, &valorVar, sizeof(uint32_t));
	char* resp = enviarOperacion(ESCRIBIR, enviar, socketUMC);
	if(resp[0] == -1){
		operacion = ERROR;
		free(enviar->buffer);
		free(enviar);
		return;
	}
	log_debug(ptrLog, "Valor asignado");
	free(enviar->buffer);
	free(enviar);
	return;
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable) {
	char* buffer;
	uint32_t id;
	t_valor_variable valor = 0;
	log_debug(ptrLog, "Obteniendo el valor de la variable compartida '%s'", variable);
	if (enviarDatos(socketNucleo, &variable, (uint32_t)(strlen(variable) + 1),	(uint32_t)LEER_VAR_COMPARTIDA, (uint32_t)CPU) < 0)
	return -1;
	buffer = recibirDatos(socketNucleo, NULL, &id);
	if (strlen(buffer) < 0)
		return -1;

	if (strcmp("ERROR", buffer) == 0) {
		return -1;
	} else {
		memcpy(&valor, buffer, sizeof(t_valor_variable));
		log_debug(ptrLog, "El valor de '%s' es %d", variable, valor);
		free(buffer);
		return valor;
	}


}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor) {
	int longitud = strlen(variable) + 1 + 8;
	log_debug(ptrLog, "Asignando el valor %d a la variable compartida '%s'",
			valor, variable);
	t_op_varCompartida* varCompartida = malloc(sizeof(t_op_varCompartida));
	varCompartida->longNombre = strlen(variable) + 1;
	varCompartida->nombre = malloc(strlen(variable) + 1);
	strcpy(varCompartida->nombre, variable);
	varCompartida->valor = valor;
	t_buffer_tamanio * tamanio_buffer = serializar_opVarCompartida(varCompartida);

	if (enviarDatos(socketNucleo, tamanio_buffer->buffer, tamanio_buffer->tamanioBuffer, (uint32_t)ASIG_VAR_COMPARTIDA, (uint32_t)CPU) < 0)
	return -1;
	free(tamanio_buffer);
	free(varCompartida->nombre);
	free(varCompartida);
	log_debug(ptrLog, "Valor asignado");
	return valor;
}

t_puntero_instruccion irAlLabel(t_nombre_etiqueta etiqueta) {
	//devuelvo la primer instruccion ejecutable de etiqueta o -1 en caso de error
	//necesito el tamanio de etiquetas, lo tendria que agregar al pcb
	//en vez de devolverla me conviene agregarla al program counter
	uint32_t numeroInstr = metadata_buscar_etiqueta(etiqueta, pcb->ind_etiq, pcb->tamanioEtiquetas);
	if(numeroInstr > -1){
		pcb->PC = numeroInstr;
	}
	log_debug(ptrLog, "Llamada a irAlLabel");
	return numeroInstr;
}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_posicion_stack donde_retornar) {
	log_debug(ptrLog, "Llamada a llamarFuncion");
	return;
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
}

void imprimirTexto(char* texto) {
	log_debug(ptrLog,
			"Enviando al kernel una cadena de texto que se mostrara por pantalla");
	texto = _string_trim(texto);
	log_trace(ptrLog, "La cadena es:\n%s", texto);
	int bytesEnviados = enviarDatos(socketNucleo, &texto, (uint32_t)(strlen(texto) + 1), (uint32_t)IMPRIMIR_TEXTO, (uint32_t)CPU);
	log_debug(ptrLog, "Cadena enviada");
}

void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo) {
	char* buffer;
	uint32_t operacion = IO;
	log_debug(ptrLog, "Se efectua la operacion de Entrada/Salida");
	t_dispositivo_io* op_IO = malloc(sizeof(t_dispositivo_io));
	op_IO->nombre = malloc(strlen(dispositivo) + 1);
	strcpy(op_IO->nombre, dispositivo);
	op_IO->tiempo = tiempo;
	buffer = serializar_opIO(op_IO);
	int bytesEnviados = enviarDatos(socketNucleo, &buffer, (uint32_t)(8 + strlen(dispositivo) + 1), operacion, (uint32_t)CPU);
	log_debug(ptrLog, "Dispositivo '%s' con un tiempo de %d enviados al kernel",
			dispositivo, tiempo);
	free(buffer);
	free(op_IO->nombre);
	free(op_IO);
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

}

void signal(t_nombre_semaforo identificador_semaforo) {
	log_debug(ptrLog, "Enviado al kernel funcion SIGNAL para el semaforo '%s'",
			identificador_semaforo);
	int bytesEnviados = enviarDatos(socketNucleo, &identificador_semaforo, (uint32_t)(strlen(identificador_semaforo) + 1), (uint32_t)SIGNAL, (uint32_t)CPU);
}
