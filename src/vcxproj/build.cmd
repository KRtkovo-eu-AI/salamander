@echo off

:: Check for command line arguments
if "%~1"=="help" (
  echo Usage: build.cmd [build_config] [build_arch]
  echo.
  echo build_config: Build configuration, default is 'Debug'
  echo build_arch: Build architecture, default is 'x64'
  echo.
  goto :eof
)

:: Check for MSBuild in the PATH
where msbuild >nul 2>&1
if %errorlevel% neq 0 set NO_MSBUILD=1

set "MSB="
call :resolve_msbuild
if not defined MSB (
  echo MSBuild.exe not found. Please install Visual Studio with C++ support or open a Developer Command Prompt.
  echo.
  goto :eof
)

call :check_v143
if errorlevel 1 goto :eof

if "%OPENSAL_BUILD_DIR%"=="" (
  echo Please set OPENSAL_BUILD_DIR environment variable.
  echo.
  goto :eof
)

:: Default values for build_config and build_arch
set build_config=Debug
set build_arch=x64

:: Override default values with command line arguments if provided
if not "%~1"=="" set build_config=%~1
if not "%~2"=="" set build_arch=%~2

call :build %build_config% %build_arch%

goto :eof

:build
  echo Building %~1/%2
  if exist %3 del /q %3
  "%MSB%" salamand.sln /t:build "/p:Configuration=%~1" /p:Platform=%2 /m:%NUMBER_OF_PROCESSORS%
  exit /b

:resolve_msbuild
if not defined NO_MSBUILD (
  set "MSB=msbuild"
  exit /b
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b

for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT exit /b

if exist "%VSROOT%\MSBuild\Current\Bin\MSBuild.exe" (
  set "MSB=%VSROOT%\MSBuild\Current\Bin\MSBuild.exe"
)
exit /b

:check_v143
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
  set "V143ROOT="
  for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.v143.x86.x64 -property installationPath`) do set "V143ROOT=%%I"
  if defined V143ROOT (
    call :select_v143_toolset
    exit /b 0
  )
)

call :select_v143_toolset
if defined V143TOOLS exit /b 0

echo Visual Studio C++ v143 build tools are not installed.
echo Open Salamander requires PlatformToolset=v143. Install component:
echo   Microsoft.VisualStudio.Component.VC.v143.x86.x64
echo.
exit /b 1

:select_v143_toolset
set "V143TOOLS="
if not defined VSROOT (
  if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -property installationPath`) do set "VSROOT=%%I"
  )
)
if not defined VSROOT exit /b

for /f "delims=" %%I in ('dir /b /ad "%VSROOT%\VC\Tools\MSVC\14.4*" 2^>nul') do set "V143TOOLS=%%I"
if not defined V143TOOLS exit /b

if not defined VCToolsVersion set "VCToolsVersion=%V143TOOLS%"
exit /b
