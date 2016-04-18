################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm

# All of the sources participating in the build are defined here
-include sources.mk
-include UMC/subdir.mk
-include Swap/subdir.mk
-include ProcesoCpu/subdir.mk
-include Nucleo/subdir.mk
-include Librerias/subdir.mk
-include Consola/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: tp-2016-1c-Chamba

# Tool invocations
tp-2016-1c-Chamba: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc  -o "tp-2016-1c-Chamba" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) tp-2016-1c-Chamba
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
