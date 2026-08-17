@echo off
REM Copies sensor DLLs from HardView submodule to extension lib directory
set "SRC=%~dp0..\..\..\..\3rd-party\HardView\HardView\LiveView\64"
set "DST=%~dp0lib"

if not exist "%DST%" mkdir "%DST%"
REM Remove files supplied by Salamander or unused when loading into pwsh.
del /Q "%DST%\HardwareWrapper.deps.json" "%DST%\HardwareWrapper.runtimeconfig.json" 2>nul
del /Q "%DST%\msvcp140.dll" "%DST%\vcruntime140.dll" "%DST%\vcruntime140_1.dll" 2>nul
del /Q "%DST%\hostpolicy.dll" 2>nul
copy /Y "%SRC%\HardwareWrapper.dll" "%DST%\"
copy /Y "%SRC%\LibreHardwareMonitorLib.dll" "%DST%\"
copy /Y "%SRC%\HidSharp.dll" "%DST%\"
copy /Y "%SRC%\Ijwhost.dll" "%DST%\"
copy /Y "%SRC%\System.Management.dll" "%DST%\"
copy /Y "%SRC%\System.CodeDom.dll" "%DST%\"
copy /Y "%SRC%\System.IO.Ports.dll" "%DST%\"
copy /Y "%SRC%\BlackSharp.Core.dll" "%DST%\"
copy /Y "%SRC%\DiskInfoToolkit.dll" "%DST%\"
copy /Y "%SRC%\RAMSPDToolkit-NDD.dll" "%DST%\"
echo Done.
