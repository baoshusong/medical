@echo off
setlocal

set "APP=%~dp0build\AiMedicalWorkstation.exe"
set "QT_BIN=E:\ProgramFiles\Qt\6.11.1\mingw_64\bin"

if not exist "%APP%" goto missing

set "PATH=%~dp0build;%QT_BIN%;%PATH%"
cd /d "%~dp0"

echo Starting AiMedicalWorkstation...
start "AiMedicalWorkstation" /wait "%APP%"
set "EXIT_CODE=%ERRORLEVEL%"
if not "%EXIT_CODE%"=="0" goto failed
endlocal
exit /b 0

:missing
echo [ERROR] Executable not found:
echo %APP%
echo Build the project first.
pause
endlocal
exit /b 1

:failed
echo [ERROR] Application exited with code %EXIT_CODE%.
pause
endlocal
exit /b %EXIT_CODE%
