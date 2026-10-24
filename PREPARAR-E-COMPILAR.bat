@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\setup.ps1"
if errorlevel 1 goto fail
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build.ps1" -Test
if errorlevel 1 goto fail
echo Pronto. Abra ABRIR-STUDIO.bat.
pause
exit /b 0
:fail
echo Nao foi possivel preparar o Studio. Confira o erro acima.
pause
exit /b 1
