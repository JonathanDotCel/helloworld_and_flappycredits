
REM Part of the UniROM source from github.com/JonathanDotCel
REM 
REM Sends the file via NOPS (hopefully somewhere on your path)
REM over UART to the PSX.
REM 
REM 

@echo off

set PATH=%PATH%;d:\notpsxserial\

REM not necessary, just brings the vid capture window to the top...
REM if this bit doesn't work for you, don't sweat it.
REM start c:\ins\dscaler\dscaler.exe

tools\NOPS /exe UNIROM_B.EXE COM8



