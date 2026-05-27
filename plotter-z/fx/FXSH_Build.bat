@echo off
rem Do not edit! This batch file is created by CASIO fx-9860G SDK.


if exist PLOTTERZ.G1A  del PLOTTERZ.G1A

cd debug
if exist FXADDINror.bin  del FXADDINror.bin
"D:\CASIO\fx-9860G SDK\OS\SH\Bin\Hmake.exe" Addin.mak
cd ..
if not exist debug\FXADDINror.bin  goto error

"D:\CASIO\fx-9860G SDK\Tools\MakeAddinHeader363.exe" "D:\gadget-dev\plotter-z\plotter-z\fx"
if not exist PLOTTERZ.G1A  goto error
echo Build has completed.
goto end

:error
echo Build was not successful.

:end

