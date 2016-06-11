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
	gcc -I"/home/utnso/tp-2016-1c-Chamba/elestac/sockets" -I"/home/utnso/tp-2016-1c-Chamba/so-commons-library"  -I"/home/utnso/tp-2016-1c-Chamba/ansisop-parser" -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


