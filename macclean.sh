#!/bin/bash


chmod +r+w+x macshell.sh
chmod +r+w+x macmake.sh
chmod +r+w+x macsend.sh

rm *.rom
rm *.exe
rm *.sym
rm *.map
rm *.obj
rm *.cpe
rom *.dep
rm *.temp
rm *.crunch
rm *.o
rm *.elf

rm comport.txt
rm stderr.txt
rm stdout.txt

#IDA stuff

rm *.idb
rm *.til
rm *.id0
rm *.id1

#./macmake.sh -f buildme.mk clean


