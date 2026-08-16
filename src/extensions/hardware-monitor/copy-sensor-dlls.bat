@echo off
REM Copies sensor DLLs from HardView submodule to extension lib directory
set "SRC=%~dp0..\..\..\..\3rd-party\HardView\HardView\LiveView\64"
set "DST=%~dp0lib"

if not exist "%DST%" mkdir "%DST%"
copy /Y "%SRC%\HardwareWrapper.dll" "%DST%\"
copy /Y "%SRC%\HardwareWrapper.deps.json" "%DST%\"
copy /Y "%SRC%\HardwareWrapper.runtimeconfig.json" "%DST%\"
copy /Y "%SRC%\LibreHardwareMonitorLib.dll" "%DST%\"
copy /Y "%SRC%\HidSharp.dll" "%DST%\"
copy /Y "%SRC%\msvcp140.dll" "%DST%\"
copy /Y "%SRC%\vcruntime140.dll" "%DST%\"
copy /Y "%SRC%\vcruntime140_1.dll" "%DST%\"
copy /Y "%SRC%\hostpolicy.dll" "%DST%\"
copy /Y "%SRC%\Ijwhost.dll" "%DST%\"
copy /Y "%SRC%\System.Management.dll" "%DST%\"
copy /Y "%SRC%\System.CodeDom.dll" "%DST%\"
copy /Y "%SRC%\System.IO.Ports.dll" "%DST%\"
copy /Y "%SRC%\BlackSharp.Core.dll" "%DST%\"
copy /Y "%SRC%\DiskInfoToolkit.dll" "%DST%\"
copy /Y "%SRC%\RAMSPDToolkit-NDD.dll" "%DST%\"
echo Done.
