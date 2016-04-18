all:con nu pcpu swap umc

con:
	-cd Consola/Debug && $(MAKE) all

nu:	
	-cd Nucleo/Debug && $(MAKE) all
	
pcpu:
	-cd ProcesoCpu/Debug && $(MAKE) all

swap:
	 -cd Swap/Debug && $(MAKE) all
	 
umc:
	-cd UMC/Debug && $(MAKE) all
		
clean:
	-cd Consola/Debug && $(MAKE) clean
	-cd Nucleo/Debug && $(MAKE) clean
	-cd ProcesoCpu/Debug && $(MAKE) clean
	-cd Swap/Debug && $(MAKE) clean
	-cd UMC/Debug && $(MAKE) clean