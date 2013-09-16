#!/bin/sh

echo "Starting installation"
echo "Installing dependent binaries"

apt-get install libproc-dev 
echo "Installed libproc-dev package"

apt-get install uthash-dev 
echo "Installed uthash-dev package"
gcc -o monitor codelearnmonitor.c -lproc

cp monitor /usr/bin/

echo "Installation Completed"
