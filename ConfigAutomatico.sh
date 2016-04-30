#Cambios en MakeFiles

#Primero los borro
cd /home/utnso/tp-2016-1c-Chamba/elestac/Consola/Debug/
sudo rm makefile

cd /home/utnso/tp-2016-1c-Chamba/elestac/Nucleo/Debug/
sudo rm makefile

cd /home/utnso/tp-2016-1c-Chamba/elestac/ProcesoCpu/Debug/
sudo rm makefile

cd /home/utnso/tp-2016-1c-Chamba/elestac/Swap/Debug/
sudo rm makefile

cd /home/utnso/tp-2016-1c-Chamba/elestac/UMC/Debug/
sudo rm makefile

#Ahora agrego los makefile modificados que tenemos en la otra carpeta
cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/Consola/makefile /home/utnso/tp-2016-1c-Chamba/elestac/Consola/Debug/

cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/Nucleo/makefile /home/utnso/tp-2016-1c-Chamba/elestac/Nucleo/Debug/

cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/ProcesoCpu/makefile /home/utnso/tp-2016-1c-Chamba/elestac/ProcesoCpu/Debug/

cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/Swap/makefile /home/utnso/tp-2016-1c-Chamba/elestac/Swap/Debug/

cp -i /home/utnso/tp-2016-1c-Chamba/ServerMakefiles/UMC/makefile /home/utnso/tp-2016-1c-Chamba/elestac/UMC/Debug/

#Libraries
cd /home/utnso/tp-2016-1c-Chamba/
git clone https://github.com/sisoputnfrba/so-commons-library.git

echo "DESCARGA DE LAS COMMONS COMPLETA"
echo "PROCEDEMOS A INSTALARLAS"
cd so-commons-library
sudo make clean
sudo make all
sudo make install

cd /home/utnso/tp-2016-1c-Chamba/elestac/
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}/home/utnso/tp-2016-1c-Chamba/so-commons-library/src/build/

cd /home/utnso/tp-2016-1c-Chamba/elestac/sockets/Debug/
sudo make clean
sudo make all
sudo make install

cd /home/utnso/tp-2016-1c-Chamba/
git clone https://github.com/sisoputnfrba/ansisop-parser.git

echo "DESCARGA DEL PARSER COMPLETA"
echo "PROCEDEMOS A INSTALARLA"
cd ansisop-parser/parser
sudo make clean
sudo make all
sudo make install

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


