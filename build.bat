@echo off
c:\microchip\buildnumber.exe  %1 ".\buildnumber.h"

if "%2"	 == "true" goto debug
:production

goto end
:debug
for %%i in (%5\*-objdump.exe) do "%%~i" -S "%3" > "%6\list.lst"



:end

@echo on
