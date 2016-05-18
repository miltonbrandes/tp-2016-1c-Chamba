################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
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
all: Nucleo

dependents:
	-cd /home/utnso/Escritorio/TP\ SO/commons/Debug && $(MAKE) all
	-cd /home/utnso/Escritorio/TP\ SO/sockets/Debug && $(MAKE) all
	-cd /home/utnso/Escritorio/TP\ SO/parser/Debug && $(MAKE) all

# Tool invocations
Nucleo: $(OBJS) $(USER_OBJS) /home/utnso/Escritorio/TP\ SO/commons/Debug/libcommons.so /home/utnso/Escritorio/TP\ SO/sockets/Debug/libsockets.so /home/utnso/Escritorio/TP\ SO/parser/Debug/libparser.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/utnso/Escritorio/TP SO/parser/Debug" -L"/home/utnso/Escritorio/TP SO/commons/Debug" -L"/home/utnso/Escritorio/TP SO/sockets/Debug" -pthread -o "Nucleo" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) Nucleo
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
/home/utnso/Escritorio/TP\ SO/commons/Debug/libcommons.so:
/home/utnso/Escritorio/TP\ SO/sockets/Debug/libsockets.so:
/home/utnso/Escritorio/TP\ SO/parser/Debug/libparser.so:

-include ../makefile.targets
