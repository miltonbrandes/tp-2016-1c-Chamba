/*
 * Consola.h
 *
 *  Created on: 18/6/2016
 *      Author: utnso
 */

#ifndef SRC_CONSOLA_H_
#define SRC_CONSOLA_H_

int crearLog();
int iniciarConsola();
int enviarScriptAlNucleo(char *script);
char* leerArchivo(FILE *archivo);
int iniciarProcesoLeyendoArchivoPropio();
int enviarScriptANucleoYEscuchar(char * script);

#endif /* SRC_CONSOLA_H_ */
