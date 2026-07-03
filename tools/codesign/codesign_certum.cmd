@echo off
setlocal EnableExtensions DisableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set "POWERSHELL=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if not exist "%POWERSHELL%" set "POWERSHELL=powershell.exe"

if "%~1"=="" goto usage
if /i "%~1"=="--help" goto usage
if /i "%~1"=="/help" goto usage
if /i "%~1"=="-h" goto usage

"%POWERSHELL%" -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%codesign_certum.ps1" %*
exit /b %ERRORLEVEL%

:usage
echo Usage:
echo   %~nx0 --file ^<path-to-exe-dll-or-spl^>
echo   %~nx0 --inno-x64 --payload-dir ^<Release_x64 payload directory^>
echo   %~nx0 --slg-by-lang --payload-dir ^<Release_x64 payload directory^>
echo.
echo Required environment:
echo   CODESIGN_ENABLED=1
echo   CODESIGN_CERT_SHA1=^<Certum certificate thumbprint without spaces^>
echo.
echo Optional environment:
echo   CODESIGN_SIGNTOOL, CODESIGN_TIMESTAMP_URL, CODESIGN_RETRIES,
echo   CODESIGN_RETRY_DELAY_SECONDS, CODESIGN_DESCRIPTION, CODESIGN_DESCRIPTION_URL
echo.
exit /b 2
