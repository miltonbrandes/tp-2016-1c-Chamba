################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/PrimitivasAnSISOP.c \
../src/ProcesoCpu.c 

OBJS += \
./src/PrimitivasAnSISOP.o \
./src/ProcesoCpu.o 

C_DEPS += \
./src/PrimitivasAnSISOP.d \
./src/ProcesoCpu.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/Escritorio/TP SO/parser" -I"/home/utnso/Escritorio/TP SO/commons" -I"/home/utnso/Escritorio/TP SO/sockets" -O2 -g -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


