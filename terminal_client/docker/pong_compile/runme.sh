#!/bin/sh

#make websocketpp_client
# sudo cp ./websocketpp_client /pong

#
#mkdir $HOME/glibc2/ && cd $HOME/glibc2
#wget http://ftp.gnu.org/gnu/libc/glibc-2.38.tar.gz
#tar -xvzf glibc-2.38.tar.gz
#mkdir build
#mkdir glibc-2.38-install
#cd build
#~/glibc/glibc-2.38/configure --prefix=$HOME/glibc/glibc-2.38-install
#make
#make install
#
#
#
#
#
#sudo rm -rf /pong/proc
#sudo cp -n /usr/lib -r /pong/lib/
#sudo cp -n /usr/lib64 -r /pong/lib/
#sudo cp -n /etc/ld.so.conf -r /pong/lib/



sudo -p mkdir /pong/lib/
sudo cp -n /lib -r /pong/lib/
sudo cp -n $HOME/lib/boost/boost_1_67_0/lib -r /pong/lib/
sudo cp -n /pong/external_lib/* -r /pong/lib/

export PATH="$PATH:/"

cd /pong
sudo apt-get y cmake
sudo make install_json
sudo make $MODE

#ls /lib
#echo
#ls /usr/lib
#echo
#ls /usr/lib/x86_64-linux-gnu

