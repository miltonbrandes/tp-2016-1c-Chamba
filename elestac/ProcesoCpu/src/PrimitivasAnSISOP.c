/*
 * PrimitivasAnSISOP.c

 *
 *  Created on: 17/5/2016
 *      Author: utnso
 */
#include "PrimitivasAnSISOP.h"
#include <sockets/EscrituraLectura.h>
#include <sockets/OpsUtiles.h>
#include "ProcesoCpu.h"
#define TAMANIO_VARIABLE 4
extern t_log* ptrLog;
t_pcb* pcb;

void setPCB(t_pcb * pcbDeCPU) {
	pcb = pcbDeCPU;
}

void freePCBDePrimitivas() {
	int a, b;

	if(pcb->ind_etiq != NULL) {
		free(pcb->ind_etiq);
	}

	for(a = 0; a < list_size(pcb->ind_codigo); a ++) {
		t_indice_codigo * indice = list_get(pcb->ind_codigo, a);
		list_remove(pcb->ind_codigo, a);
		free(indice);
	}
	free(pcb->ind_codigo);

	for(a = 0; a < list_size(pcb->ind_stack); a ++) {
		t_stack* linea = list_get(pcb->ind_stack, a);
		uint32_t cantidadArgumentos = list_size(linea->argumentos);

		for(b = 0; b < cantidadArgumentos; b++) {
			t_argumento *argumento = list_get(linea->argumentos, b);
			list_remove(linea->argumentos, b);
			free(argumento);
		}
		free(linea->argumentos);

		int32_t cantidadVariables = list_size(linea->variables);

		for(b = 0; b < cantidadVariables; b++) {
			t_variable *variable = list_get(linea->variables, b);
			list_remove(linea->variables, b);
			free(variable->idVariable);
			free(variable);
		}
	}
	free(pcb->ind_stack);

	free(pcb);
}

bool esArgumento(t_nombre_variable identificador_variable){
	if(isdigit(identificador_variable)){
		return true;
	}else{
		return false;
	}
}

t_puntero definirVariable(t_nombre_variable identificador_variable) {
	if(!esArgumento(identificador_variable)){//si entra a este if es porque es una variable, si no entra es porque es un argumento, me tengo que fijar si es del 0 al 9, no solo del 0
		log_debug(ptrLog, "Definir variable %c", identificador_variable);
		t_variable* nuevaVar = malloc(sizeof(t_variable));
		t_stack* lineaStack = list_get(pcb->ind_stack, pcb->numeroContextoEjecucionActualStack);
		if(pcb->stackPointer + 4 > tamanioPagina && pcb->paginaStackActual - pcb->primerPaginaStack == tamanioStack -1){
			if(!huboStackOver){
				log_error(ptrLog, "StackOverflow. Se finaliza el proceso");
				huboStackOver = true;
			}
			return -1;
		}else{
			if(lineaStack == NULL){
				//el tamaño de la linea del stack seria delos 4 ints mas
				uint32_t tamLineaStack = 7*sizeof(uint32_t)+1;
				lineaStack = malloc(tamLineaStack);
				lineaStack->retVar = NULL;
				lineaStack->direcretorno = 0;
				lineaStack->argumentos = list_create();
				lineaStack->variables = list_create();
				//lineaStack->variables = malloc((sizeof(uint32_t)*3)+1);
				//pcb->ind_stack = malloc(tamLineaStack);
				list_add(pcb->ind_stack, lineaStack);
			}
			//me fijo si el offset de la ultima + el tamaño superan o son iguales el tamaño de la pagina, si esto sucede, tengo que pasar a una pagina nueva
			if(pcb->stackPointer + TAMANIO_VARIABLE > tamanioPagina){
				nuevaVar->idVariable = identificador_variable;
				pcb->paginaStackActual++;
				nuevaVar->pagina = pcb->paginaStackActual;
				nuevaVar->size = TAMANIO_VARIABLE;
				nuevaVar->offset = 0;
				pcb->stackPointer = TAMANIO_VARIABLE;
				list_add(lineaStack->variables, nuevaVar);
			}else{
				nuevaVar->idVariable = identificador_variable;
				nuevaVar->pagina = pcb->paginaStackActual;
				nuevaVar->size = TAMANIO_VARIABLE;
				nuevaVar->offset = pcb->stackPointer;
				pcb->stackPointer+= TAMANIO_VARIABLE;
				list_add(lineaStack->variables, nuevaVar);
			}
			//calculo el desplazamiento desde la primer pagina del stack hasta donde arranca mi nueva variable
			uint32_t posicionRet = (nuevaVar->pagina * tamanioPagina) + nuevaVar->offset;
			log_debug(ptrLog, "%c %i %i %i", nuevaVar->idVariable, nuevaVar->pagina, nuevaVar->offset, nuevaVar->size);
			return posicionRet;
		}
	}else{
		//en este caso es un argumento, realizar toda la logica aca y tambien en obtener posicion variable, asignar imprimir y retornar
		log_debug(ptrLog, "Definir variable - argumento %c", identificador_variable);
		t_argumento* nuevoArg = malloc(sizeof(t_argumento));
		t_stack* lineaStack = list_get(pcb->ind_stack, pcb->numeroContextoEjecucionActualStack);
		if(pcb->stackPointer + 4 > tamanioPagina && pcb->paginaStackActual - pcb->primerPaginaStack == tamanioStack -1){
			if(!huboStackOver){
				log_error(ptrLog, "StackOverflow. Se finaliza el proceso");
				huboStackOver = true;
			}
			return -1;
		}else{
			//me fijo si el offset de la ultima + el tamaño superan o son iguales el tamaño de la pagina, si esto sucede, tengo que pasar a una pagina nueva
			if(pcb->stackPointer + TAMANIO_VARIABLE > tamanioPagina){
				pcb->paginaStackActual++;
				nuevoArg->pagina = pcb->paginaStackActual;
				nuevoArg->size = TAMANIO_VARIABLE;
				nuevoArg->offset = 0;
				pcb->stackPointer = TAMANIO_VARIABLE;
				list_add(lineaStack->argumentos, nuevoArg);
			}else{
				nuevoArg->pagina = pcb->paginaStackActual;
				nuevoArg->size = TAMANIO_VARIABLE;
				nuevoArg->offset = pcb->stackPointer;
				pcb->stackPointer += TAMANIO_VARIABLE;
				list_add(lineaStack->argumentos, nuevoArg);
			}
			//calculo el desplazamiento desde la primer pagina del stack hasta donde arranca mi nueva variable
			uint32_t posicionRet = (nuevoArg->pagina * tamanioPagina) + nuevoArg->offset;
			log_debug(ptrLog, "%c %i %i %i", identificador_variable, nuevoArg->pagina, nuevoArg->offset, nuevoArg->size);
			return posicionRet;
		}
	}
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable) {
	if(!esArgumento(identificador_variable)){
		t_variable* variable = malloc(sizeof(t_variable));
		int i = 0;
		char* nom = calloc(1, 2);
		nom[0] = identificador_variable;
		log_debug(ptrLog, "Obtener posicion variable '%c'", identificador_variable);
		//obtengo la linea del stack del contexto de ejecucion actual...
		t_stack* lineaActualStack = list_get(pcb->ind_stack, pcb->numeroContextoEjecucionActualStack);
		//me posiciono al inicio de esta linea del stack
		//me fijo cual variable de la lista coincide con el nombre que me estan pidiendo
		if(list_size((t_list*)lineaActualStack->variables) > 0){
			for(i = 0; i<list_size(lineaActualStack->variables); i++){

				variable = list_get(lineaActualStack->variables, i);
				if(variable->idVariable == identificador_variable ){
					free(nom);
					uint32_t posicionRet = (variable->pagina * tamanioPagina) + variable->offset;
					return posicionRet;
				}
			}
		}
		//devuelvo -1
		uint32_t posicionRetError = -1;
		free(variable);
		free(nom);
		return posicionRetError;
	}else{
		t_argumento* arg = malloc(sizeof(t_argumento));
		int i = 0;
		char* nom = calloc(1, 2);
		nom[0] = identificador_variable;
		log_debug(ptrLog, "Obtener posicion variable '%c'", identificador_variable);
		//obtengo la linea del stack del contexto de ejecucion actual...
		t_stack* lineaActualStack = list_get(pcb->ind_stack, pcb->numeroContextoEjecucionActualStack);
		//me posiciono al inicio de esta linea del stack
		//me fijo cual variable de la lista coincide con el nombre que me estan pidiendo
		if(list_size((t_list*)lineaActualStack->argumentos) > 0){
			int lineaArg = identificador_variable - '0';
			arg = list_get(lineaActualStack->argumentos, lineaArg);
			free(nom);
			uint32_t posicionRet = (arg->pagina * tamanioPagina) + arg->offset;
			return posicionRet;
		}
		//devuelvo -1
		uint32_t posicionRetError = -1;
		free(arg);
		free(nom);
		return posicionRetError;
	}
}

t_valor_variable dereferenciar(t_puntero direccion_variable) {
	//calculo el la posicion de la variable en el stack mediante el desplazamiento
	t_posicion_stack posicionRet;
	posicionRet.pagina = (direccion_variable/tamanioPagina);
	posicionRet.offset = direccion_variable%tamanioPagina;
	posicionRet.size = TAMANIO_VARIABLE;
	t_valor_variable valor = 0;
	log_debug(ptrLog, "Dereferenciar %d", direccion_variable);
	t_solicitarBytes* solicitar = malloc(sizeof(t_solicitarBytes));
	solicitar->pagina = posicionRet.pagina;
	solicitar->offset = TAMANIO_VARIABLE;
	solicitar->start = posicionRet.offset;
	char* buffer = enviarOperacion(LEER_VALOR_VARIABLE, solicitar, socketUMC);
	if(buffer != NULL) {
		t_instruccion * instruccion = deserializarInstruccion(buffer);
		if(strcmp(instruccion, "FINALIZAR") == 0) {
			log_error(ptrLog, "La variable no pudo dereferenciarse. Se finaliza el Proceso.");
			finalizarProcesoPorErrorEnUMC();
			return 0;
		}else{
			int valueAsInt = atoi(instruccion->instruccion);
			memcpy(&valor, &valueAsInt, sizeof(t_valor_variable));
			log_debug(ptrLog, "Variable dereferenciada. Valor: %d", valor);
			free(buffer);
		}
	}
	free(solicitar);
	return valor;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor) {
	log_debug(ptrLog, "Asignar. Posicion %d - Valor %d", direccion_variable, valor);
	//calculo el la posicion de la variable en el stack mediante el desplazamiento

	t_enviarBytes* enviar = malloc(sizeof(uint32_t) * 4);
	enviar->pagina = (direccion_variable / tamanioPagina);
	enviar->offset = direccion_variable % tamanioPagina;
	enviar->tamanio = TAMANIO_VARIABLE;
	enviar->pid = pcb->pcb_id;
	enviar->buffer = malloc(sizeof(uint32_t));
	sprintf(enviar->buffer, "%d", valor);

	char* resp = enviarOperacion(ESCRIBIR, enviar, socketUMC);
	if(resp != NULL && resp[0] == -1){
		operacion = ERROR;
		free(enviar->buffer);
		free(enviar);
		return;
	}else{
		uint32_t result = deserializarUint32(resp);
		if(result == SUCCESS) {
			log_info(ptrLog, "Variable asignada");
		}else if(result == ERROR){
			log_error(ptrLog, "La variable no pudo asignarse. Se finaliza el Proceso.");
			finalizarProcesoPorErrorEnUMC();
		}
	}
	free(resp);
	free(enviar->buffer);
	free(enviar);
	return;
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable) {
	char* buffer;
	uint32_t id = CPU;
	t_valor_variable valor = 0;
	uint32_t lon = strlen(variable)+1;
	operacion = LEER_VAR_COMPARTIDA;
	log_debug(ptrLog, "Obtener valor compartida '%s'", variable);
	if (enviarDatos(socketNucleo, variable, lon, operacion, id) < 0)
	return -1;
	buffer = recibirDatos(socketNucleo, &operacion, &id);

	if (strcmp("ERROR", buffer) == 0) {
		free(buffer);
		return -1;
	} else {
		memcpy(&valor, buffer, sizeof(t_valor_variable));
		log_debug(ptrLog, "Valor compartida '%s' obtenido. Valor: %d", variable, valor);
		free(buffer);
		return valor;
	}
}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor) {
	uint32_t op = ASIG_VAR_COMPARTIDA;
	uint32_t id = CPU;
	log_debug(ptrLog, "Asignar valor compartida. Variable compartida: '%s' - Valor: %d",
			variable, valor);
	t_op_varCompartida* varCompartida = malloc(sizeof(t_op_varCompartida));
	varCompartida->longNombre = strlen(variable) + 1;
	varCompartida->nombre = malloc(strlen(variable) + 1);
	strcpy(varCompartida->nombre, variable);
	varCompartida->valor = valor;
	t_buffer_tamanio * tamanio_buffer = serializar_opVarCompartida(varCompartida);

	if (enviarDatos(socketNucleo, tamanio_buffer->buffer, tamanio_buffer->tamanioBuffer, op, id) < 0)
	return -1;
	free(tamanio_buffer->buffer);
	free(tamanio_buffer);
	free(varCompartida->nombre);
	free(varCompartida);
	log_debug(ptrLog, "Valor compartida '%s' asignada", variable);
	return valor;
}

void irAlLabel(t_nombre_etiqueta etiqueta) {
	//devuelvo la primer instruccion ejecutable de etiqueta o -1 en caso de error
	//necesito el tamanio de etiquetas, lo tendria que agregar al pcb
	//en vez de devolverla me conviene agregarla al program counter
	log_debug(ptrLog, "Ir al Label '%s'", etiqueta);
	t_puntero_instruccion numeroInstr = metadata_buscar_etiqueta(etiqueta, pcb->ind_etiq, pcb->tamanioEtiquetas);
	pcb->PC = numeroInstr - 1;

	return;
}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar) {
	log_debug(ptrLog, "Llamar con Retorno. Reservando espacio y cambiando al nuevo contexto de ejecucion");
	uint32_t tamLineaStack = 4*sizeof(uint32_t)+2*sizeof(t_list);
	t_stack * nuevaLineaStackEjecucionActual;
	t_argumento* varRetorno = malloc(sizeof(t_argumento));
	varRetorno->pagina = (donde_retornar/tamanioPagina);
	varRetorno->offset = donde_retornar%tamanioPagina;
	varRetorno->size = TAMANIO_VARIABLE;
	if(nuevaLineaStackEjecucionActual == NULL){
		//el tamaño de la linea del stack seria delos 4 ints mas
		nuevaLineaStackEjecucionActual = malloc(tamLineaStack);
		nuevaLineaStackEjecucionActual->argumentos = list_create();
		nuevaLineaStackEjecucionActual->variables = list_create();
		nuevaLineaStackEjecucionActual->retVar = varRetorno;
		nuevaLineaStackEjecucionActual->direcretorno = pcb->PC;
		list_add(pcb->ind_stack, nuevaLineaStackEjecucionActual);

		pcb->numeroContextoEjecucionActualStack++;
	}

	irAlLabel(etiqueta);
	return;
}

void retornar(t_valor_variable retorno) {
	//agarro contexto actual y anterior
	int a = pcb->numeroContextoEjecucionActualStack;
	t_stack* contextoEjecucionActual = list_get(pcb->ind_stack, a);

	//Limpio el contexto actual
	int i = 0;
	for(i = 0; i < list_size(contextoEjecucionActual->argumentos); i++){
		t_argumento* arg = list_get(contextoEjecucionActual->argumentos, i);
		pcb->stackPointer=pcb->stackPointer-4;
		free(arg);
	}
	for(i = 0; i <list_size(contextoEjecucionActual->variables); i++){
		t_variable* var = list_get(contextoEjecucionActual->variables, i);
		pcb->stackPointer=pcb->stackPointer-4;
		free(var);
	}
	t_argumento* retVar = contextoEjecucionActual->retVar;
	t_puntero direcVariable = (retVar->pagina * tamanioPagina) + retVar->offset;
	//calculo la direccion a la que tengo que retornar mediante la direccion de pagina start y offset que esta en el campo retvar
	asignar(direcVariable, retorno);
	//elimino el contexto actual del indice del stack
	//Seteo el contexto de ejecucion actual en el anterior
	pcb->PC =  contextoEjecucionActual->direcretorno;
	free(contextoEjecucionActual);
	list_remove(pcb->ind_stack, pcb->numeroContextoEjecucionActualStack);
	pcb->numeroContextoEjecucionActualStack = pcb->numeroContextoEjecucionActualStack -1;
	t_stack* contextoEjecNuevo = list_get(pcb->ind_stack, pcb->numeroContextoEjecucionActualStack);
	log_debug(ptrLog, "Llamada a retornar");
	return;
}

void imprimir(t_valor_variable valor_mostrar) {
	log_debug(ptrLog,
			"Envio a Nucleo valor %d para que Consola imprima por pantalla.",
			valor_mostrar);
	t_buffer_tamanio*  buff = serializar(valor_mostrar);
	uint32_t op = IMPRIMIR_VALOR;
	uint32_t id = CPU;
	int bytesEnviados = enviarDatos(socketNucleo, buff->buffer, buff->tamanioBuffer, op, id);
	free(buff->buffer);
	free(buff);
}

void imprimirTexto(char* texto) {
	log_debug(ptrLog, "Envio la cadena '%s' a Nucleo para que Consola imprima por pantalla", texto);
	texto = _string_trim(texto);
	uint32_t op = IMPRIMIR_TEXTO;
	uint32_t id = CPU;
	uint32_t lon = strlen(texto)+1;
	int bytesEnviados = enviarDatos(socketNucleo, texto, lon, op, id);
}

void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo) {

	operacion = IO;
	uint32_t id = CPU;
	uint32_t lon = strlen(dispositivo)+1+sizeof(uint32_t) + sizeof(int);
	log_debug(ptrLog, "Entrada/Salida. Dispositivo: '%s' - Tiempo: %d", dispositivo, tiempo);
	t_dispositivo_io* op_IO = malloc(sizeof(t_dispositivo_io));
	op_IO->nombre = malloc(strlen(dispositivo)+1);
	strcpy(op_IO->nombre, dispositivo);
	op_IO->tiempo = tiempo;
	char* buffer;
	buffer = (char *)serializar_opIO(op_IO);
	enviarDatos(socketNucleo, buffer, lon, operacion, id);
	free(buffer);
	free(op_IO->nombre);
	free(op_IO);
}

void wait(t_nombre_semaforo identificador_semaforo) {
	char* buffer;
	uint32_t op = WAIT;
	uint32_t id = CPU;
	uint32_t lon = strlen(identificador_semaforo)+1;
	log_debug(ptrLog, "Wait. Semaforo: '%s'", identificador_semaforo);
	enviarDatos(socketNucleo, identificador_semaforo, lon, op, id);
	log_debug(ptrLog, "Esperando respuesta del Nucleo para Wait.");
	buffer = recibirDatos(socketNucleo, &op, &id);
	if(operacion != NOTHING){
		operacion = op;
		log_debug(ptrLog,"Proceso no queda bloqueado por Semaforo '%s'", identificador_semaforo);
	}
	if (op == WAIT){
		operacion = op;
		log_debug(ptrLog,"Proceso bloqueado por Semaforo '%s'", identificador_semaforo);
	}
	free(buffer);
}

void ansisop_signal(t_nombre_semaforo identificador_semaforo) {
	uint32_t op = SIGNAL;
	uint32_t id = CPU;
	uint32_t lon = strlen(identificador_semaforo)+1;

	log_debug(ptrLog, "Signal. Semaforo: '%s'",
			identificador_semaforo);
	enviarDatos(socketNucleo, identificador_semaforo, lon, op, id);
	return;
}
