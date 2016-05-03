#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <sockets/ClienteFunciones.h>
#include <sockets/EscrituraLectura.h>

#define MAX_BUFFER_SIZE 4096

t_log* ptrLog;
t_config* config;
char *direccion;
int puerto;
char buff[MAX_BUFFER_SIZE] = "Hola como estas, soy consola, nucleo";
char respuestaServidor[MAX_BUFFER_SIZE];
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
	char *buff = "Soy un nuevo programa, vas a iniciar el circuito";
	char *respuestaServidor;
	int bytesRecibidos = 0;
	int id = 1;
	int longitud = strlen(buff);
	int operacion = 1;
	socketConexionNucleo = AbrirConexion(direccion, puerto);
	if (socketConexionNucleo < 0) {
		log_info(ptrLog, "Error en la conexion con el nucleo");
		finalizarConexion(socketConexionNucleo);
		return -1;
	}
	log_info(ptrLog, "Se conecto con el nucleo");
	bytesRecibidos = escribir(socketConexionNucleo, id, longitud, operacion, buff);
	if (bytesRecibidos< 0) {
		finalizarConexion(socketConexionNucleo);
		return -1;
	}
	log_info(ptrLog, "Mensaje Enviado al nucleo");
	while (1) {
			log_info(ptrLog, "Esperando mensaje del Nucleo");
			bytesRecibidos = leer(socketConexionNucleo, &id, &buff);
			if (bytesRecibidos < 0) {
				finalizarConexion(socketConexionNucleo);
				log_info(ptrLog, "Error en la lectura del nucleo");
				return -1;
			}else if(bytesRecibidos > 0) {
				log_info(ptrLog, "Mensaje recibido de Nucleo: %s", respuestaServidor);
			} else {
				//Aca matamos a Nucleo
				finalizarConexion(socketConexionNucleo);
				log_info(ptrLog, "No se recibio nada de Nucleo. Cerramos conexion");
				break;
			}
		}
	//log_info(ptrLog, respuestaServidor);
	finalizarConexion(socketConexionNucleo);
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
		if(iniciarConsola() == 1){
		//cuando reciba por linea de comandos la ruta para abrir un programa lo tengo que abrir
		log_info(ptrLog, "Inicio del Programa");
		log_debug(ptrLog, "Abriendo el script..");
		/*if ((programa = fopen(argv[1], "r")) == NULL)
		{
				log_error(ptrLog, "No se ha podido abrir el script. Favor, verificar si existe.");
				return EXIT_FAILURE;
		}*/
		//script = leerArchivo(programa);
		//fclose(programa);
		//log_debug(ptrLog, "Se abrio el script con exito");
			char comando[100];
			while(1) {
				printf("Ingrese un comando: ");
				scanf("%s", comando);
				if(strcmp("run", comando) == 0) {
					printf("\n");
					break;
				}else{
					printf("\nComando no reconocido.\n\n");
				}
			}

			enviarScriptAlNucleo(script);
		}
		else
		{
			log_info(ptrLog, "Hubo un error al abrir el archivo de configuracion de la consola.");
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
