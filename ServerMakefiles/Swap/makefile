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
all: Swap

dependents:
	-cd /tp-2016-1c-Chamba/so-commons-library && $(MAKE) all
	-cd /tp-2016-1c-Chamba/ansisop-parser/parser && $(MAKE) all
	-cd /tp-2016-1c-Chamba/elestac/sockets/Debug && $(MAKE) all

# Tool invocations
Swap: $(OBJS) $(USER_OBJS) /home/utnso/tp-2016-1c-Chamba/so-commons-library/src/build/libcommons.so /home/utnso/tp-2016-1c-Chamba/elestac/sockets/Debug/libsockets.so /home/utnso/tp-2016-1c-Chamba/ansisop-parser/parser/build/libparser-ansisop.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/utnso/tp-2016-1c-Chamba/so-commons-library/src/build" -L"/home/utnso/tp-2016-1c-Chamba/elestac/sockets/Debug" -L"/home/utnso/tp-2016-1c-Chamba/ansisop-parser/parser/build/" -o "Swap" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) Swap
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
/home/utnso/tp-2016-1c-Chamba/so-commons-library/src/build/libcommons.so:
/home/utnso/tp-2016-1c-Chamba/ansisop-parser/parser/build/libparser-ansisop.so:
/home/utnso/tp-2016-1c-Chamba/elestac/sockets/Debug/libsockets.so:

-include ../makefile.targets
