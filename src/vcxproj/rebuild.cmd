@echo off

set "MSB="
call :resolve_msbuild
if not defined MSB (
  echo MSBuild.exe not found. Please install Visual Studio with C++ support or open a Developer Command Prompt.
  echo.
  pause
  exit /b
)

call :check_v143
if errorlevel 1 (
  echo.
  pause
  exit /b
)

if "%OPENSAL_BUILD_DIR%"=="" (
  echo Please set OPENSAL_BUILD_DIR environment variable.
  echo.
  pause
  exit /b
)

echo Rebuild Menu:
echo.
echo 3 - rebuild all targets
echo 5 - Internal Build (Utils+Debug x86/x64)
echo 6 - Developers Build (Utils+Debug+Release x86/x64)
echo 8 - Release/Beta Build (Utils+Release x86/x64)
echo 9 - Utils Only (Utils x86/x64)
echo.
set __your_choice=5
set /P __your_choice=Select what you want to rebuild (0-9) [5]: 
echo.

if %__your_choice% LSS 3 goto :choice_is_invalid
if %__your_choice% GTR 9 goto :choice_is_invalid
goto :choice_is_ok

:choice_is_invalid
echo Invalid selection, exiting...
echo.
pause
exit /b
  
:choice_is_ok
set __skip_debug=no
set __skip_release=no
set __skip_release_x64=no
set __skip_utils=no
goto :choice_%__your_choice%

:choice_3
echo Rebuild all targets
goto :end_choice

:choice_5
echo Internal Build (Utils+Debug x86/x64)
set __skip_release=yes
goto :end_choice

:choice_6
echo Developers Build (Utils+Debug+Release x86/x64)
goto :end_choice

:choice_8
echo Release/Beta Build (Utils+Release x86/x64)
set __skip_debug=yes
goto :end_choice

:choice_9
echo Utils Only (Utils x86/x64)
set __skip_release=yes
set __skip_debug=yes
goto :end_choice

:end_choice
echo.

call :remove_files "rebuild_*.log*"

set TIMESLOG=rebuild_times.log
set STARTTIME=%time%
set BUILDSTART=%time%
echo START TIME:%STARTTIME% >%TIMESLOG%

if "%__skip_debug%"=="no" (
  call :rebuild Debug             Win32 rebuild_debug_x86.log
  call :rebuild Debug             x64   rebuild_debug_x64.log
)

if "%__skip_release%"=="no" (
  call :rebuild Release           Win32 rebuild_release_x86.log
  if "%__skip_release_x64%"=="no" (
    call :rebuild Release         x64   rebuild_release_x64.log
  )
)

if "%__skip_utils%"=="no" (
  call :rebuild "Utils (Release)" Win32 rebuild_utils_x86.log
  call :rebuild "Utils (Release)" x64   rebuild_utils_x64.log
)

set ENDTIME=%time%
echo END TIME:%ENDTIME% >>%TIMESLOG%
echo. >>%TIMESLOG%
call ..\..\tools\duration.cmd "%STARTTIME%" "%ENDTIME%" "Total Duration: " >>%TIMESLOG%

if exist *.log.err (
  echo.
  echo REBUILD ERRORS:
  dir /B *.log.err
)
if exist *.log.wrn (
  echo.
  echo REBUILD WARNINGS:
  dir /B *.log.wrn
) else (
  if not exist *.log.err (
    echo.
    echo REBUILD DONE: 0 ERRORS, 0 WARNINGS
  )
)

echo.
pause
exit /b

:rebuild <target> <platform> <logfile>
echo Building %~1/%2
"%MSB%" salamand.sln /t:rebuild "/p:Configuration=%~1" /p:Platform=%2 /l:FileLogger,Microsoft.Build.Engine;logfile=%3;append=false;verbosity=normal;encoding=windows-1250 /flp1:logfile=%3.err;errorsonly /flp2:logfile=%3.wrn;warningsonly /m:%NUMBER_OF_PROCESSORS%
call ..\..\tools\duration.cmd "%BUILDSTART%" "%time%" "%~1/%2: Build Duration: " >>%TIMESLOG%
call :remove_file_if_empty %3.wrn
call :remove_file_if_empty %3.err
set BUILDSTART=%time%
exit /b

:remove_files <wildcard>
if exist %1 (
  echo Deleting files: %1 in %CD%
  dir /B %1
  del /q %1
  echo.
)
exit /b

:remove_file_if_empty <filename>
if %~z1==0 del %1
exit /b

:resolve_msbuild
where msbuild >nul 2>&1
if %errorlevel% equ 0 (
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
