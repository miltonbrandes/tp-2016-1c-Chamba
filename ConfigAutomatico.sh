cd /home/utnso/Escritorio/PruebaConfig/tp-2016-1c-Chamba/elestac/sockets/Debug/
sudo make clean
sudo make all
sudo make install

#Libraries
cd /home/utnso/Escritorio/PruebaConfig/tp-2016-1c-Chamba/elestac/
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/home/utnso/Escritorio/PruebaConfig/tp-2016-1c-Chamba/elestac/sockets/Debug/

cd /home/utnso/Escritorio/PruebaConfig/tp-2016-1c-Chamba/
git clone https://github.com/sisoputnfrba/ansisop-parser.git

echo "DESCARGA DEL PARSER COMPLETA"
echo "PROCEDEMOS A INSTALARLA"
cd ansisop-parser/parser
sudo make clean
sudo make all
sudo make install

cd /home/utnso/Escritorio/PruebaConfig/tp-2016-1c-Chamba/elestac/
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/home/utnso/Escritorio/PruebaConfig/tp-2016-1c-Chamba/ansisop-parser/parser/build/

cd /home/utnso/Escritorio/PruebaConfig/tp-2016-1c-Chamba/
git clone https://github.com/sisoputnfrba/so-commons-library.git

echo "DESCARGA DE LAS COMMONS COMPLETA"
echo "PROCEDEMOS A INSTALARLAS"
cd so-commons-library
sudo make clean
sudo make all
sudo make install

cd /home/utnso/Escritorio/PruebaConfig/tp-2016-1c-Chamba/elestac/
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/home/utnso/Escritorio/PruebaConfig/tp-2016-1c-Chamba/so-commons-library/src/build/