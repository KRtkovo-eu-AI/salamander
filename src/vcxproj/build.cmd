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

call :check_v145
if errorlevel 1 goto :eof

if "%OPENSAL_BUILD_DIR%"=="" (
  echo Please set OPENSAL_BUILD_DIR environment variable.
  echo.
  goto :eof
)

:: link.exe can fail with LNK1000 even with only a few resource-only
:: language DLL links running concurrently. Serialize solution projects by
:: default; individual C++ projects still use the compiler's /MP parallelism.
:: Build agents can explicitly opt into solution-level parallelism.
set "BUILD_JOBS=%OPENSAL_BUILD_JOBS%"
if not defined BUILD_JOBS set "BUILD_JOBS=1"

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
  "%MSB%" salamand.sln /t:build "/p:Configuration=%~1" /p:Platform=%2 /m:%BUILD_JOBS%
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

:check_v145
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
  set "V145ROOT="
  for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "V145ROOT=%%I"
  if defined V145ROOT (
    call :select_v145_toolset
    if defined V145TOOLS exit /b 0
  )
)

call :select_v145_toolset
if defined V145TOOLS exit /b 0

echo Visual Studio C++ v145 build tools are not installed.
echo Open Salamander requires PlatformToolset=v145 (Visual Studio 2026).
echo Install the Desktop development with C++ workload.
echo.
exit /b 1

:select_v145_toolset
set "V145TOOLS="
if not defined VSROOT (
  if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -property installationPath`) do set "VSROOT=%%I"
  )
)
if not defined VSROOT exit /b

for /f "delims=" %%I in ('dir /b /ad "%VSROOT%\VC\Tools\MSVC\14.5*" 2^>nul') do set "V145TOOLS=%%I"
if not defined V145TOOLS exit /b

if not defined VCToolsVersion set "VCToolsVersion=%V145TOOLS%"
exit /b
