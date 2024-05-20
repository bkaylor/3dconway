
@echo off

if "%1%"=="release" (
  cl /O2 /Zi main.c -Fegameoflife
) else (
  cl /Zi main.c -Fegameoflife
)
