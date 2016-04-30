################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../sockets/ClienteFunciones.c \
../sockets/EscrituraLectura.c \
../sockets/ServidorFunciones.c 

OBJS += \
./sockets/ClienteFunciones.o \
./sockets/EscrituraLectura.o \
./sockets/ServidorFunciones.o 

C_DEPS += \
./sockets/ClienteFunciones.d \
./sockets/EscrituraLectura.d \
./sockets/ServidorFunciones.d 


# Each subdirectory must supply rules for building sources it contributes
sockets/%.o: ../sockets/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2016-1c-Chamba/so-commons-library" -O2 -g -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


