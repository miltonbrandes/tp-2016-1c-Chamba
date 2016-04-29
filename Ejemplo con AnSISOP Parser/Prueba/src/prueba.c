/*
 * prueba.c
 *
 *  Created on: 23/4/2016
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>
#include <parser/metadata_program.h>

char* ReadFile(char *filename)
{
   char *buffer = NULL;
   int string_size, read_size;
   FILE *handler = fopen(filename, "r");

   if (handler)
   {
       // Seek the last byte of the file
       fseek(handler, 0, SEEK_END);
       // Offset from the first to the last byte, or in other words, filesize
       string_size = ftell(handler);
       // go back to the start of the file
       rewind(handler);

       // Allocate a string that can hold it all
       buffer = (char*) malloc(sizeof(char) * (string_size + 1) );

       // Read it all in one operation
       read_size = fread(buffer, sizeof(char), string_size, handler);

       // fread doesn't set it so put a \0 in the last position
       // and buffer is now officially a string
       buffer[string_size] = '\0';

       if (string_size != read_size)
       {
           // Something went wrong, throw away the memory and set
           // the buffer to NULL
           free(buffer);
           buffer = NULL;
       }

       // Always remember to close the file.
       fclose(handler);
    }

    return buffer;
}


int main()
{
    char *string = ReadFile("../Prueba/ansisop/facil.ansisop");
    if (string)
    {
    	t_metadata_program *program = metadata_desde_literal(string);
    	printf("\n\nCantidad de etiquetas: %d\n", program->cantidad_de_etiquetas);
    	printf("Cantidad de funciones: %d\n", program->cantidad_de_funciones);
    	printf("Etiquetas: %s\n", program->etiquetas);
    	printf("Tamano etiquetas: %i\n", program->etiquetas_size);
    	printf("Cantidad instrucciones: %i\n", program->instrucciones_size);
    	printf("Instruccion inicio: %i\n", program->instruccion_inicio);
    	printf("Instruccion serializado offset: %i\n", program->instrucciones_serializado->offset);
    	printf("Instruccion serializado start: %i\n", program->instrucciones_serializado->start);

//        puts(string);
//        free(string);
    }

    return 0;
}
