
@echo off

del unirom_b.exe
del unirom_b.elf
del unirom_b.o
del unirom_b.sym
del unirom_b.map
del *.temp

call dockermake -f buildme.mk all


