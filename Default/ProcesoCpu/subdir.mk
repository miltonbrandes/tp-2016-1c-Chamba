################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ProcesoCpu/procesoCpu.c 

OBJS += \
./ProcesoCpu/procesoCpu.o 

C_DEPS += \
./ProcesoCpu/procesoCpu.d 


# Each subdirectory must supply rules for building sources it contributes
ProcesoCpu/%.o: ../ProcesoCpu/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


