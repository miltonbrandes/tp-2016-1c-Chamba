#!/usr/bin/env bash -l
#Cambios en MakeFiles

#Primero los borro
cd /home/utnso/tp-2016-1c-Chamba/elestac/Consola/Debug/
sudo rm makefile
sudo rm objects.mk
cd /home/utnso/tp-2016-1c-Chamba/elestac/Consola/Debug/src/
sudo rm subdir.mk

cd /home/utnso/tp-2016-1c-Chamba/elestac/Nucleo/Debug/
sudo rm makefile
sudo rm objects.mk
cd /home/utnso/tp-2016-1c-Chamba/elestac/Nucleo/Debug/src/
sudo rm subdir.mk

cd /home/utnso/tp-2016-1c-Chamba/elestac/ProcesoCpu/Debug/
sudo rm makefile
sudo rm objects.mk
cd /home/utnso/tp-2016-1c-Chamba/elestac/ProcesoCpu/Debug/src/
sudo rm subdir.mk

cd /home/utnso/tp-2016-1c-Chamba/elestac/Swap/Debug/
sudo rm makefile
sudo rm objects.mk
cd /home/utnso/tp-2016-1c-Chamba/elestac/Swap/Debug/src/
sudo rm subdir.mk

cd /home/utnso/tp-2016-1c-Chamba/elestac/UMC/Debug/
sudo rm makefile
sudo rm objects.mk
cd /home/utnso/tp-2016-1c-Chamba/elestac/UMC/Debug/src/
sudo rm subdir.mk

#Libraries
cd /home/utnso/tp-2016-1c-Chamba/so-commons-library
sudo make clean
sudo make all
sudo make install

cd /home/utnso/tp-2016-1c-Chamba/elestac/
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/home/utnso/tp-2016-1c-Chamba/so-commons-library/src/build/

cd /home/utnso/tp-2016-1c-Chamba/elestac/sockets/Debug/
sudo make clean
sudo make all
sudo make install

cd /home/utnso/tp-2016-1c-Chamba/ansisop-parser/parser 
sudo make all
ls build/
sudo make install

#Ahora agrego los makefile modificados que tenemos en la otra carpeta
cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/Consola/makefile /home/utnso/tp-2016-1c-Chamba/elestac/Consola/Debug/
cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/Consola/objects.mk /home/utnso/tp-2016-1c-Chamba/elestac/Consola/Debug/
cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/Consola/src/subdir.mk /home/utnso/tp-2016-1c-Chamba/elestac/Consola/Debug/src/

cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/Nucleo/makefile /home/utnso/tp-2016-1c-Chamba/elestac/Nucleo/Debug/
cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/Nucleo/objects.mk /home/utnso/tp-2016-1c-Chamba/elestac/Nucleo/Debug/
cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/Nucleo/src/subdir.mk /home/utnso/tp-2016-1c-Chamba/elestac/Nucleo/Debug/src/

cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/ProcesoCpu/makefile /home/utnso/tp-2016-1c-Chamba/elestac/ProcesoCpu/Debug/
cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/ProcesoCpu/objects.mk /home/utnso/tp-2016-1c-Chamba/elestac/ProcesoCpu/Debug/
cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/ProcesoCpu/src/subdir.mk /home/utnso/tp-2016-1c-Chamba/elestac/ProcesoCpu/Debug/src/

cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/Swap/makefile /home/utnso/tp-2016-1c-Chamba/elestac/Swap/Debug/
cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/Swap/objects.mk /home/utnso/tp-2016-1c-Chamba/elestac/Swap/Debug/
cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/Swap/src/subdir.mk /home/utnso/tp-2016-1c-Chamba/elestac/Swap/Debug/src/

cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/UMC/makefile /home/utnso/tp-2016-1c-Chamba/elestac/UMC/Debug/
cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/UMC/objects.mk /home/utnso/tp-2016-1c-Chamba/elestac/UMC/Debug/
cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/UMC/src/subdir.mk /home/utnso/tp-2016-1c-Chamba/elestac/UMC/Debug/src/

cd /home/utnso/tp-2016-1c-Chamba/elestac/
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/home/utnso/tp-2016-1c-Chamba/ansisop-parser/parser/build/

cd /home/utnso/tp-2016-1c-Chamba/elestac/
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/home/utnso/tp-2016-1c-Chamba/elestac/sockets/Debug/

#Muevo libs
cd /home/utnso/tp-2016-1c-Chamba/elestac/
mkdir LIBRERIAS
cd /home/utnso/tp-2016-1c-Chamba/elestac/LIBRERIAS/

cp -i /home/utnso/tp-2016-1c-Chamba/so-commons-library/src/build/libcommons.so /home/utnso/tp-2016-1c-Chamba/elestac/LIBRERIAS/
cp -i /home/utnso/tp-2016-1c-Chamba/ansisop-parser/parser/build/libparser-ansisop.so /home/utnso/tp-2016-1c-Chamba/elestac/LIBRERIAS/
cp -i /home/utnso/tp-2016-1c-Chamba/elestac/sockets/Debug/libsockets.so /home/utnso/tp-2016-1c-Chamba/elestac/LIBRERIAS/

#Limpio LD_LIBRARY_PATH y agrego la posta
unset LD_LIBRARY_PATH
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}/home/utnso/tp-2016-1c-Chamba/elestac/LIBRERIAS/

#Hago los make

cd /home/utnso/tp-2016-1c-Chamba/elestac/Consola/Debug/
sudo make all

cd /home/utnso/tp-2016-1c-Chamba/elestac/Nucleo/Debug/
sudo make all

cd /home/utnso/tp-2016-1c-Chamba/elestac/ProcesoCpu/Debug/
sudo make all

cd /home/utnso/tp-2016-1c-Chamba/elestac/Swap/Debug/
sudo make all

cd /home/utnso/tp-2016-1c-Chamba/elestac/UMC/Debug/
sudo make all

#Agrego variables de entorno necesarias

export CONSOLA_LOG=/home/utnso/tp-2016-1c-Chamba/elestac/Consola/log.txt
export CONSOLA_CONFIG=/home/utnso/tp-2016-1c-Chamba/elestac/Consola/Consola.txt

export NUCLEO_LOG=/home/utnso/tp-2016-1c-Chamba/elestac/Nucleo/log.txt
export NUCLEO_CONFIG=/home/utnso/tp-2016-1c-Chamba/elestac/Nucleo/Nucleo.txt

export SWAP_LOG=/home/utnso/tp-2016-1c-Chamba/elestac/Swap/log.txt
export SWAP_CONFIG=/home/utnso/tp-2016-1c-Chamba/elestac/Swap/Swap.txt

export CPU_LOG=/home/utnso/tp-2016-1c-Chamba/elestac/ProcesoCpu/log.txt
export CPU_CONFIG=/home/utnso/tp-2016-1c-Chamba/elestac/ProcesoCpu/ProcesoCpu.txt

export UMC_LOG=/home/utnso/tp-2016-1c-Chamba/elestac/UMC/log.txt
export UMC_CONFIG=/home/utnso/tp-2016-1c-Chamba/elestac/UMC/UMC.txt

cd /home/utnso/tp-2016-1c-Chamba/elestac/