@echo off
setlocal EnableExtensions

if "%~4"=="" (
  echo copy_managed_dependencies.cmd expects 4 arguments: plugin_root configuration platform target_dir
  exit /B 1
)

set "PLUGIN_ROOT=%~1"
set "MANAGED_CONFIG=%~2"
set "PLATFORM=%~3"
set "TARGET_DIR=%~4"

if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"

set "MANAGED_TRIMMED=%MANAGED_CONFIG:_x64=%"
set "MANAGED_TRIMMED=%MANAGED_TRIMMED:_Win32=%"
set "MANAGED_DIR=%PLUGIN_ROOT%managed\bin\%MANAGED_TRIMMED%\"
if not exist "%MANAGED_DIR%TextViewer.Managed.dll" set "MANAGED_DIR=%PLUGIN_ROOT%managed\bin\%MANAGED_CONFIG%\"
if not exist "%MANAGED_DIR%TextViewer.Managed.dll" set "MANAGED_DIR=%MANAGED_DIR%net48\"

if not exist "%MANAGED_DIR%TextViewer.Managed.dll" (
  echo TextViewer.Managed.dll was not found in "%PLUGIN_ROOT%managed\bin\%MANAGED_TRIMMED%" or the alternate paths checked.
  exit /B 1
)

call :require "%MANAGED_DIR%PrismSharp.dll" "PrismSharp.dll was not found next to TextViewer.Managed.dll in %MANAGED_DIR%."
call :require "%MANAGED_DIR%Newtonsoft.Json.dll" "Newtonsoft.Json.dll was not found next to TextViewer.Managed.dll in %MANAGED_DIR%."
call :require "%MANAGED_DIR%Microsoft.Web.WebView2.WinForms.dll" "Microsoft.Web.WebView2.WinForms.dll was not found next to TextViewer.Managed.dll in %MANAGED_DIR%."
call :require "%MANAGED_DIR%Microsoft.Web.WebView2.Core.dll" "Microsoft.Web.WebView2.Core.dll was not found next to TextViewer.Managed.dll in %MANAGED_DIR%."

set "WEBVIEW2_LOADER=WebView2Loader.dll"
if not exist "%MANAGED_DIR%%WEBVIEW2_LOADER%" (
  if exist "%MANAGED_DIR%x64\WebView2Loader.dll" set "WEBVIEW2_LOADER=x64\WebView2Loader.dll"
)
if not exist "%MANAGED_DIR%%WEBVIEW2_LOADER%" (
  if exist "%MANAGED_DIR%x86\WebView2Loader.dll" set "WEBVIEW2_LOADER=x86\WebView2Loader.dll"
)
call :require "%MANAGED_DIR%%WEBVIEW2_LOADER%" "WebView2Loader.dll was not found in %MANAGED_DIR% or its architecture subdirectories."

set "MANAGED_DATA_DIR="
for %%I in ("%MANAGED_DIR%TextViewer.Managed.dll") do if not defined MANAGED_DATA_DIR set "MANAGED_DATA_DIR=%%~dpIdata"
if not exist "%MANAGED_DATA_DIR%\languages\languages.json" if exist "%PLUGIN_ROOT%managed\data\languages\languages.json" set "MANAGED_DATA_DIR=%PLUGIN_ROOT%managed\data"
if not defined MANAGED_DATA_DIR (
  echo Failed to resolve PrismSharp data directory from %MANAGED_DIR%.
  exit /B 1
)
if not exist "%MANAGED_DATA_DIR%\languages\languages.json" (
  echo PrismSharp data files were not found under %MANAGED_DATA_DIR%.
  exit /B 1
)

if not exist "%TARGET_DIR%data\" mkdir "%TARGET_DIR%data\"

xcopy /E /I /Y "%MANAGED_DATA_DIR%\*" "%TARGET_DIR%data\" >nul
if errorlevel 4 (
  echo Failed to copy PrismSharp data files from %MANAGED_DATA_DIR%.
  exit /B 1
)

copy /Y "%MANAGED_DIR%TextViewer.Managed.dll" "%TARGET_DIR%TextViewer.Managed.dll" >nul
copy /Y "%MANAGED_DIR%PrismSharp.dll" "%TARGET_DIR%PrismSharp.dll" >nul
copy /Y "%MANAGED_DIR%Newtonsoft.Json.dll" "%TARGET_DIR%Newtonsoft.Json.dll" >nul
copy /Y "%MANAGED_DIR%Microsoft.Web.WebView2.WinForms.dll" "%TARGET_DIR%Microsoft.Web.WebView2.WinForms.dll" >nul
copy /Y "%MANAGED_DIR%Microsoft.Web.WebView2.Core.dll" "%TARGET_DIR%Microsoft.Web.WebView2.Core.dll" >nul
copy /Y "%MANAGED_DIR%%WEBVIEW2_LOADER%" "%TARGET_DIR%WebView2Loader.dll" >nul

endlocal
exit /B 0

:require
if exist "%~1" exit /B 0
echo %~2
exit /B 1
