################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../sockets/utiles/OpsUtiles.c 

OBJS += \
./sockets/utiles/OpsUtiles.o 

C_DEPS += \
./sockets/utiles/OpsUtiles.d 


# Each subdirectory must supply rules for building sources it contributes
sockets/utiles/%.o: ../sockets/utiles/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


