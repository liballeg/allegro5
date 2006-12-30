:: Used by makefile.all's create-install-dirs target.

@echo off

:loop
if [%1] == [] goto :end
if not exist %1\nul mkdir %1
if errorlevel 1 goto :end
:: FreeDOS doesn't seem to support errorlevel.
if not exist %1\nul goto :end
shift
goto :loop

:end
