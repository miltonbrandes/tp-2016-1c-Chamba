################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../sockets/ClienteFunciones.c \
../sockets/EscrituraLectura.c \
../sockets/OpsUtiles.c \
../sockets/ServidorFunciones.c \
../sockets/StructsUtiles.c 

OBJS += \
./sockets/ClienteFunciones.o \
./sockets/EscrituraLectura.o \
./sockets/OpsUtiles.o \
./sockets/ServidorFunciones.o \
./sockets/StructsUtiles.o 

C_DEPS += \
./sockets/ClienteFunciones.d \
./sockets/EscrituraLectura.d \
./sockets/OpsUtiles.d \
./sockets/ServidorFunciones.d \
./sockets/StructsUtiles.d 


# Each subdirectory must supply rules for building sources it contributes
sockets/%.o: ../sockets/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


