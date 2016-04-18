################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Nucleo/nucleo.c 

OBJS += \
./Nucleo/nucleo.o 

C_DEPS += \
./Nucleo/nucleo.d 


# Each subdirectory must supply rules for building sources it contributes
Nucleo/%.o: ../Nucleo/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


