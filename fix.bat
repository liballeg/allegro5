@echo off

rem Sets up the Allegro package for building with the specified compiler,
rem and if possible converting text files from LF to CR/LF format.

if [%3] == []        goto arg2
goto help

:arg2

if [%2] == [--quick] goto arg1
if [%2] == []        goto arg1
goto help

:arg1

if [%1] == [bcc32]   goto bcc32
if [%1] == [djgpp]   goto djgpp
if [%1] == [mingw32] goto mingw32
if [%1] == [msvc]    goto msvc
if [%1] == [watcom]  goto watcom
goto help

:bcc32

echo Configuring Allegro for Windows/BCC32...
set AL_MAKEFILE=makefile.bcc
set AL_PLATFORM=ALLEGRO_BCC32
goto fix

:djgpp

echo Configuring Allegro for DOS/djgpp...
set AL_MAKEFILE=makefile.dj
set AL_PLATFORM=ALLEGRO_DJGPP
goto fix

:mingw32

echo Configuring Allegro for Windows/Mingw32...
set AL_MAKEFILE=makefile.mgw
set AL_PLATFORM=ALLEGRO_MINGW32
goto fix

:msvc

echo Configuring Allegro for Windows/MSVC...
set AL_MAKEFILE=makefile.vc
set AL_PLATFORM=ALLEGRO_MSVC
goto fix

:watcom

echo Configuring Allegro for DOS/Watcom...
set AL_MAKEFILE=makefile.wat
set AL_PLATFORM=ALLEGRO_WATCOM
goto fix

:help

echo.
echo Usage: fix platform [--quick]
echo.
echo Where platform is one of: bcc32, djgpp, mingw32, msvc or watcom.
echo The --quick parameter is used to turn off LF to CR/LF conversion.
echo.
goto end

:ooes_error

echo.
echo ERROR: Read the section "Out of Environment space" in Allegro's FAQ.
goto end

:fix

if [%AL_MAKEFILE%] == [] goto ooes_error
if [%AL_PLATFORM%] == [] goto ooes_error

echo # generated by fix.bat > makefile
echo MAKEFILE_INC = %AL_MAKEFILE% >> makefile
echo include makefile.all >> makefile

echo /* generated by fix.bat */ > include\allegro\platform\alplatf.h
echo #define %AL_PLATFORM% >> include\allegro\platform\alplatf.h

if [%2] == [--quick] goto done
if [%1] == [bcc32]   goto done
if [%1] == [mingw32] goto done

echo Converting Allegro files to DOS CR/LF format...
utod *.bat .../*.c *.cfg .../*.h .../*.inc .../*.rc
utod .../*.inl .../*.s .../*.txt .../*._tx makefile.*

:done

echo Done!

:end

set AL_PLATFORM=
set AL_MAKEFILE=
