#!/bin/sh

make clean
make
if [ $? -eq 0 ] 
then
  cp example.g3a /media/disk
  wc -c example.g3a
  sync
  umount /media/disk
fi
