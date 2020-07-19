@echo off

cls

echo .
echo Hit ctrl+c if this was a mistake.
echo .
pause


REM Regular Build Stuff

del *.rom
del *.exe
del *.sym
del *.map
del *.obj
del *.cpe
del *.dep
del *.temp
del *.crunch
del *.o

del comport.txt

del stderr.txt
del stdout.txt

REM IDA stuff

del *.idb
del *.til
del *.id0
del *.id1
del *.nam
del *.i64


call dockermake -f buildme.mk clean



