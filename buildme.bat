
@echo off

del helloworld.exe
del helloworld.elf
del helloworld.o
del helloworld.sym
del helloworld.map
del *.temp

call dockermake -f buildme.mk all


