
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <Librerias/socketsClienteFunciones.h>
#include <Librerias/socketsEscrituraLectura.h>

extern t_config* config;
int enviarProgramaAlNucleo(char *programa)
{
	//leo del archivo de configuracion el puerto y el ip
	char *direccion = config_get_string_value(config, "IP");
	int puerto = config_get_int_value(config, "PUERTO");

	int socketConexionNucleo;
	if((socketConexionNucleo = AbrirConexion(direccion, puerto)) < 0){
		//aca me deberia mostrar por log que hubo un error
		return -1;
	}
	char* buff = "Hola como estas?";
	char* respuestaServidor;
	//aca se conecto con el nucleo
	if(escribir(socketConexionNucleo, buff, sizeof(buff)+1) < 0)
	{
		//error, no encontro el servidor
		return -1;
	}
	//si pasa este if se conecta correctamente al socket servidor
	printf("se conecto correctamente")
	//Respuesta del socket servidor
	if(leer(socketConexionNucleo, respuestaServidor, sizeof(respuestaServidor)+1)< 0)
	{
		//no pude recibir nada del nucleo
		return -1;
	}
	printf("Recibi %s", respuestaServidor);
	finalizarConexion(socketConexionNucleo);
	return 0;
}

//falta reservar memoria para las variables que envio por los sockets y liberarla una vez que las termino de usar
int main(int argc, char **argv) {
	if(enviarProgramaAlNucleo != 0)
	{
		printf("Hubo un error en el envio");

	}
	return 0;
}