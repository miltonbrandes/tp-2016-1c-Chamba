#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>
#include <sockets/OpsUtiles.h>
#include <stdbool.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

t_log* ptrLog;
t_config* config;

char *direccion;
int puerto;
int bytesRecibidos = 0;
int socketConexionNucleo;

int crearLog() {
	ptrLog = log_create(getenv("CONSOLA_LOG"), "Consola", 1, 0);
	if (ptrLog) {
		return 1;

	} else {
		return 0;
	}
}

int iniciarConsola() {
	config = config_create(getenv("CONSOLA_CONFIG"));
	if (config) {
		if (config_has_property(config, "PUERTO")) {
			puerto = config_get_int_value(config, "PUERTO");
		} else {
			log_info(ptrLog, "El archivo de configuracion no tiene puerto");
			return 0;

		}
		if (config_has_property(config, "IP")) {
			direccion = config_get_string_value(config, "IP");
		} else {
			log_info(ptrLog, "El archivo de configuracion no tiene IP");
			return 0;
		}
	}
	return 1;
}

int enviarScriptAlNucleo(char *script) {

	uint32_t id = 1;
	uint32_t operacion;
	uint32_t longitud = strlen(script) + 1;
	char *buff = malloc(longitud);
	buff = script;
	int bytesRecibidos = 0;
	socketConexionNucleo = AbrirConexion(direccion, puerto);
	if (socketConexionNucleo < 0) {
		log_info(ptrLog, "Error en la conexion con el nucleo");
		finalizarConexion(socketConexionNucleo);
		return -1;
	}
	log_info(ptrLog, "Se conecto con el nucleo");
	operacion = 1;
	bytesRecibidos = enviarDatos(socketConexionNucleo, buff, longitud,
			operacion, id);
	if (bytesRecibidos < 0) {
		finalizarConexion(socketConexionNucleo);
		return -1;
	}
	log_info(ptrLog, "Mensaje Enviado al nucleo");
	return 0;

}

//creo la funcion leer archivo para poder asignarle al script ansisop lo que dice el archivo
char* leerArchivo(FILE *archivo) {
	//busco el final del archivo
	fseek(archivo, 0, SEEK_END);
	//veo que tan grande es
	long fsize = ftell(archivo);
	//voy al principio nuevamente para leerlo
	fseek(archivo, 0, SEEK_SET);
	//reservo memoria para poder usarlo
	char *script = malloc(fsize + 1);
	//leo el archivo
	size_t resp = fread(script, fsize, 1, archivo);
	script[fsize] = '\0';
	//muestro el script por el log para ver si es correcto
	log_info(ptrLog, script);
	return script;

}


int main(int argc, char **argv) {
	//creo la variable que me va a leer el script mediante el archivo que me llegue.
	char* script;
	FILE *programa;
	//leo del archivo de configuracion el puerto y el ip
	//creo el log
	if (crearLog()) {
		if (iniciarConsola() == 1) {
			//cuando reciba por linea de comandos la ruta para abrir un programa lo tengo que abrir
			log_info(ptrLog, "Inicio del Programa");
			log_debug(ptrLog, "Abriendo el script..");
			if ((programa =
					fopen(
							"/home/utnso/tp-2016-1c-Chamba/Ejemplo con AnSISOP Parser/Prueba/ansisop/facil.ansisop",
							"r")) == NULL) {
				log_error(ptrLog,
						"No se ha podido abrir el script. Favor, verificar si existe.");
				return EXIT_FAILURE;
			}
			script = leerArchivo(programa);
			fclose(programa);
			log_debug(ptrLog, "Se abrio el script con exito");

			enviarScriptAlNucleo(script);
			char* server_reply;
			uint32_t operacion;
			uint32_t id;
			int valor = 0;
			bool salir = false;
			while (1) {
				server_reply = recibirDatos(socketConexionNucleo, &operacion, &id);
				if (strlen(server_reply) < 0) {
					log_error(ptrLog, "Error al recibir datos del servior.");
					free(server_reply);
					return EXIT_FAILURE;
				} else if (strcmp("ERROR", server_reply) == 0) {
					log_error(ptrLog, "No se recibio nada de Nucleo. Cierro la conexion");
					break;
				} else {
					switch (operacion) {
					case IMPRIMIR_TEXTO:
						printf("%s\n", server_reply);
						break;
					case IMPRIMIR_VALOR:
						valor = deserializar(server_reply);
						//memcpy(&valor, server_reply, 4);
						printf("%i\n", valor);
						break;
					default:
						printf("%s\n", server_reply);
						salir = true;
						break;
					}

					free(server_reply);
					if (salir)
						break;
				}
			}
		} else {
			log_info(ptrLog,
					"Hubo un error al abrir el archivo de configuracion de la consola.");
		}
	} else {
		log_info(ptrLog, "La consola no pudo iniciarse");
		return -1;
	}

	free(script);
	log_destroy(ptrLog);
	config_destroy(config);
	return EXIT_SUCCESS;
}
