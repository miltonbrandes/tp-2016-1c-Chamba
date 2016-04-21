/*
 * Swap.c
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <sockets/ServidorFunciones.h>
#include <commons/collections/list.h>
#include <commons/log.h>


int main() {
	////////////////////////CREO LOS ARCHIVOS DE CONFIGURACION Y LOG/////////////////////////////////////////////
	t_log* log_file;
	t_config* config_file;
	char* config_file_root="/home/utnso/tp-2016-1c-Chamba/elestac/Swap/Swap.txt";
	char* log_file_root="/home/utnso/tp-2016-1c-Chamba/elestac/Swap/log.txt";
	log_file =log_create(log_file_root,"Swap",1,0);
	config_file= config_create(config_file_root);

	int puerto = config_get_int_value(config_file,"PUERTO_ESCUCHA");
	int socketUMC= AbrirSocketServidor(puerto);
	if (socketUMC < 0) {
		printf("holap");
		//aca me deberia mostrar por log que hubo un error
		return -1;
	}
	log_info(log_file, "Esperando conexiones");
	AceptarConexionCliente(socketUMC);





	/*int conectado = AceptarConexionCliente(socket);
	if(conectado>-1){
		printf( "Se establecio conexion al SWAP");
		return 0;
	}
	else{
		printf( "conexion fallida");
		return -1;
	}
*/
	return EXIT_SUCCESS;

}
