#Libraries
cd /home/utnso/tp-2016-1c-Chamba/
git clone https://github.com/sisoputnfrba/so-commons-library.git

echo "DESCARGA DE LAS COMMONS COMPLETA"

cd /home/utnso/tp-2016-1c-Chamba/elestac/
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/home/utnso/tp-2016-1c-Chamba/so-commons-library/src/build/

cd /home/utnso/tp-2016-1c-Chamba/elestac/sockets/Debug/
sudo make clean
sudo make all
sudo make install

cd /home/utnso/tp-2016-1c-Chamba/
git clone https://github.com/sisoputnfrba/ansisop-parser.git

echo "DESCARGA DEL PARSER COMPLETA"