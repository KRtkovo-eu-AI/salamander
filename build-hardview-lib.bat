@echo off
setlocal

set HARDVIEW_DIR=%~dp03rd-party\HardView
set BUILD_DIR=%~dp0src\extensions\hardware-monitor\hardview-lib
set EXT_SRC=%~dp0src\extensions\hardware-monitor
set HARDVIEW_LIB=%HARDVIEW_DIR%\HardView\LiveView\64

if "%OPENSAL_BUILD_DIR%"=="" (
  echo Please set OPENSAL_BUILD_DIR environment variable.
  exit /b 1
)

set EXT_DST=%OPENSAL_BUILD_DIR%salamander\Release_x64\extensions\hardware-monitor

echo === Building HardView HardwareWrapper ===
echo.

echo [1/3] Restoring NuGet packages...
nuget restore "%BUILD_DIR%\HardwareWrapper\packages.config" -PackagesDirectory "%BUILD_DIR%\packages"

echo [2/3] Building HardwareWrapper.dll...
msbuild "%BUILD_DIR%\HardwareWrapper\HardwareWrapper.vcxproj" /p:Configuration=Release /p:Platform=x64 /v:minimal
if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed!
    exit /b 1
)

echo [3/3] Copying to extensions directory...
if not exist "%EXT_DST%\locales" mkdir "%EXT_DST%\locales"
if not exist "%EXT_DST%\lib" mkdir "%EXT_DST%\lib"

copy /Y "%EXT_SRC%\extension.json" "%EXT_DST%\"
copy /Y "%EXT_SRC%\main.ps1" "%EXT_DST%\"
copy /Y "%EXT_SRC%\icon.svg" "%EXT_DST%\"
copy /Y "%EXT_SRC%\icon-dark.svg" "%EXT_DST%\"
copy /Y "%EXT_SRC%\default.ico" "%EXT_DST%\"
copy /Y "%EXT_SRC%\locales\*.json" "%EXT_DST%\locales\"

copy /Y "%BUILD_DIR%\bin\Release\HardwareWrapper.dll" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\LibreHardwareMonitorLib.dll" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\HidSharp.dll" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\HardwareWrapper.deps.json" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\HardwareWrapper.runtimeconfig.json" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\msvcp140.dll" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\vcruntime140.dll" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\vcruntime140_1.dll" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\System.Management.dll" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\System.CodeDom.dll" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\System.IO.Ports.dll" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\BlackSharp.Core.dll" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\DiskInfoToolkit.dll" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\RAMSPDToolkit-NDD.dll" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\Ijwhost.dll" "%EXT_DST%\lib\"
copy /Y "%HARDVIEW_LIB%\hostpolicy.dll" "%EXT_DST%\lib\"

echo.
echo Done! Hardware Monitor extension copied to %EXT_DST%
