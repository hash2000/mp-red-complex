@echo off
rem Portable RedComplex launcher.
rem
rem Usage:
rem   RedComplex.cmd                 start the application
rem   RedComplex.cmd /update-config  copy the bundled reference configuration
rem                                  over the working copy in the user profile
rem
rem On the first run the reference configuration (bootstrap.yaml, applications,
rem assets, stores) is copied to %LOCALAPPDATA%\Hash2000\red-complex.
setlocal EnableExtensions

set "ROOT=%~dp0"
set "PROFILE=%LOCALAPPDATA%\Hash2000\red-complex"

if /i "%~1"=="/update-config" goto :update

if not exist "%PROFILE%\bootstrap.yaml" (
	echo [RedComplex] First run: copying the reference configuration to "%PROFILE%"
	call :copy_config
	if errorlevel 1 goto :copy_failed
)

start "" "%ROOT%Launcher.exe"
exit /b 0

:update
echo [RedComplex] Copying the bundled configuration to "%PROFILE%"
call :copy_config
if errorlevel 1 goto :copy_failed
echo [RedComplex] Done. Databases in "%PROFILE%\data" were not touched.
exit /b 0

:copy_config
if not exist "%ROOT%Configurations\bootstrap.yaml" (
	echo [RedComplex] The Configurations folder is missing next to Launcher.exe.
	exit /b 1
)
xcopy "%ROOT%Configurations" "%PROFILE%" /E /I /Y >nul
exit /b %errorlevel%

:copy_failed
echo [RedComplex] Failed to copy the configuration.
echo [RedComplex] Copy the Configurations folder to "%PROFILE%" manually.
pause
exit /b 1
