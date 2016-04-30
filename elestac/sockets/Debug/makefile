################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include sockets/subdir.mk
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
all: libsockets.so

dependents:
	-cd /tp-2016-1c-Chamba/so-commons-library && $(MAKE) all

# Tool invocations
libsockets.so: $(OBJS) $(USER_OBJS) /home/utnso/tp-2016-1c-Chamba/so-commons-library/src/build/libcommons.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/utnso/tp-2016-1c-Chamba/so-commons-library/src/build" -shared -o "libsockets.so" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(LIBRARIES) libsockets.so
	-@echo ' '

install: all
	cp -u /home/utnso/tp-2016-1c-Chamba/elestac/sockets/Debug/libsockets.so /usr/lib

uninstall:
	rm -f /usr/lib/libsockets.so

.PHONY: all clean dependents
.SECONDARY:
/home/utnso/tp-2016-1c-Chamba/so-commons-library/src/build/libcommons.so:

-include ../makefile.targets
