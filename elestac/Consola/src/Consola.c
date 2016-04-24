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
int crearLog()
{
	ptrLog = log_create("../Consola/log.txt", "Consola", 1, 0);
		 if(ptrLog)
		 {
			 return 1;
		 }else
		 {
			 return 0;
		 }
}
int iniciarConsola(t_config* config)
{
	config = config_create("../Consola/Consola.txt");
	if(config)
	{
		if(config_has_property(config, "PUERTO"))
		{
			puerto = config_get_int_value(config, "PUERTO");
		}
		else
		{
			log_info(ptrLog, "El archivo de configuracion no tiene puerto");
			return 0;

		}
		if(config_has_property(config, "IP"))
		{
			direccion = config_get_string_value(config, "IP");
		}
		else
		{
			log_info(ptrLog, "El archivo de configuracion no tiene IP");
			return 0;
		}
	}
	return 1;
}
int enviarScriptAlNucleo()
{
	//aca lo que deberia mandar es el script, no una cosa cualquiera
	char buff[MAX_BUFFER_SIZE] = "Hola como estas?";
	char* respuestaServidor="kease";
	int bytesRecibidos = 0;
	int socketConexionNucleo;
	socketConexionNucleo = AbrirConexion(direccion, puerto);
	if (socketConexionNucleo < 0)
	{
	//aca me deberia mostrar por log que hubo un error
		log_info(ptrLog, "Error en la conexion con el nucleo");
		return -1;
	}
	log_info(ptrLog, "Se conecto con el nucleo");
	//aca se conecto con el nucleo
	if (escribir(socketConexionNucleo, buff, sizeof(buff)) < 0)
	{
	//error, no encontro el servidor
		return -1;
	}
	log_info(ptrLog, "Mensaje Enviado al nucleo");
	//si pasa este if se conecta correctamente al socket servidor
	//Respuesta del socket servidor
	while(1)
	{
		bytesRecibidos = leer(socketConexionNucleo, respuestaServidor, sizeof(respuestaServidor));
		if (bytesRecibidos < 0) {
		//no pude recibir nada del nucleo
			log_info(ptrLog, "Error en la lectura del nucleo");
			return -1;
		}
		log_info(ptrLog, respuestaServidor);
		//free(respuestaServidor); ES NECESARIO???
	}
	finalizarConexion(socketConexionNucleo);
	return EXIT_SUCCESS;
}
//creo la funcion leer archivo para poder asignarle al script ansisop lo que dice el archivo
char* leerArchivo(FILE *archivo)
{
	//busco el final del archivo
	fseek(archivo, 0, SEEK_END);
	//veo que tan grande es
	long fsize = ftell(archivo);
	//voy al principio nuevamente para leerlo
	fseek(archivo, 0, SEEK_SET);
	//reservo memoria para poder usarlo
	char *script = malloc(fsize + 1);
	//leo el archivo
	fread(script, fsize, 1, archivo);
	script[fsize] = '\0';
	//muestro el script por el log para ver si es correcto
	log_info(ptrLog, script);
	return script;

}
int main(int argc, char **argv) {
	//creo la variable que me va a leer el script mediante el archivo que me llegue.
	//char* script;
	 //FILE *programa;
	 //leo del archivo de configuracion el puerto y el ip
	 //creo el log
	 if(crearLog())
	 {
		 iniciarConsola(config);
		 //cuando reciba por linea de comandos la ruta para abrir un programa lo tengo que abrir
		 //programa = fopen(argv[1], "r");
		 //script = leerArchivo(programa);
		 enviarScriptAlNucleo();
	 }
	 else
	 {
		 log_info(ptrLog, "La consola no pudo iniciarse");
		 return -1;
	 }

	 return EXIT_SUCCESS;
}







