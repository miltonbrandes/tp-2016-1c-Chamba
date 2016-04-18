all:Consola Nucleo ProcesoCpu Swap UMC

Consola:
	-cd Consola/Debug && $(MAKE) all

Nucleo:	
	-cd Nucleo/Debug && $(MAKE) all
	
ProcesoCpu:
	-cd ProcesoCpu/Debug && $(MAKE) all

Swap:
	 -cd Swap/Debug && $(MAKE) all
	 
UMC:
	-cd UMC/Debug && $(MAKE) all
		
clean:
	-cd Consola/Debug && $(MAKE) clean
	-cd Nucle/Debug && $(MAKE) clean
	-cd ProcesoCpu/Debug && $(MAKE) clean
	-cd Swap/Debug && $(MAKE) clean
	-cd UMC/Debug && $(MAKE) clean