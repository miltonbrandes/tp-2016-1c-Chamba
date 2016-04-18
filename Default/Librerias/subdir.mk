################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Librerias/socketsEscrituraLectura.c 

OBJS += \
./Librerias/socketsEscrituraLectura.o 

C_DEPS += \
./Librerias/socketsEscrituraLectura.d 


# Each subdirectory must supply rules for building sources it contributes
Librerias/%.o: ../Librerias/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


