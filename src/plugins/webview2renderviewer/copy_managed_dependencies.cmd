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
if not exist "%MANAGED_DIR%WebView2RenderViewer.Managed.dll" set "MANAGED_DIR=%PLUGIN_ROOT%managed\bin\%MANAGED_CONFIG%\"
if not exist "%MANAGED_DIR%WebView2RenderViewer.Managed.dll" set "MANAGED_DIR=%MANAGED_DIR%net48\"

if not exist "%MANAGED_DIR%WebView2RenderViewer.Managed.dll" (
  echo WebView2RenderViewer.Managed.dll was not found in "%PLUGIN_ROOT%managed\bin\%MANAGED_TRIMMED%" or the alternate paths checked.
  exit /B 1
)

call :require "%MANAGED_DIR%Markdig.dll" "Markdig.dll was not found next to WebView2RenderViewer.Managed.dll in %MANAGED_DIR%."
call :require "%MANAGED_DIR%Microsoft.Web.WebView2.WinForms.dll" "Microsoft.Web.WebView2.WinForms.dll was not found next to WebView2RenderViewer.Managed.dll in %MANAGED_DIR%."
call :require "%MANAGED_DIR%Microsoft.Web.WebView2.Core.dll" "Microsoft.Web.WebView2.Core.dll was not found next to WebView2RenderViewer.Managed.dll in %MANAGED_DIR%."

set "WEBVIEW2_LOADER=WebView2Loader.dll"
if not exist "%MANAGED_DIR%%WEBVIEW2_LOADER%" (
  if exist "%MANAGED_DIR%x64\WebView2Loader.dll" set "WEBVIEW2_LOADER=x64\WebView2Loader.dll"
)
if not exist "%MANAGED_DIR%%WEBVIEW2_LOADER%" (
  if exist "%MANAGED_DIR%x86\WebView2Loader.dll" set "WEBVIEW2_LOADER=x86\WebView2Loader.dll"
)
if not exist "%MANAGED_DIR%%WEBVIEW2_LOADER%" (
  set "RUNTIME_ARCH=%PLATFORM%"
  if /I "%RUNTIME_ARCH%"=="Win32" set "RUNTIME_ARCH=x86"
  if /I "%RUNTIME_ARCH%"=="AnyCPU" set "RUNTIME_ARCH=x86"
  if /I "%RUNTIME_ARCH%"=="x86" set "RUNTIME_ARCH=x86"
  if /I "%RUNTIME_ARCH%"=="x64" set "RUNTIME_ARCH=x64"
  if exist "%MANAGED_DIR%runtimes\win-%RUNTIME_ARCH%\native\WebView2Loader.dll" set "WEBVIEW2_LOADER=runtimes\win-%RUNTIME_ARCH%\native\WebView2Loader.dll"
)
call :require "%MANAGED_DIR%%WEBVIEW2_LOADER%" "WebView2Loader.dll was not found in %MANAGED_DIR% or its architecture subdirectories."

copy /Y "%MANAGED_DIR%WebView2RenderViewer.Managed.dll" "%TARGET_DIR%WebView2RenderViewer.Managed.dll" >nul
copy /Y "%MANAGED_DIR%Markdig.dll" "%TARGET_DIR%Markdig.dll" >nul
copy /Y "%MANAGED_DIR%Microsoft.Web.WebView2.WinForms.dll" "%TARGET_DIR%Microsoft.Web.WebView2.WinForms.dll" >nul
copy /Y "%MANAGED_DIR%Microsoft.Web.WebView2.Core.dll" "%TARGET_DIR%Microsoft.Web.WebView2.Core.dll" >nul
copy /Y "%MANAGED_DIR%%WEBVIEW2_LOADER%" "%TARGET_DIR%WebView2Loader.dll" >nul
if exist "%MANAGED_DIR%System.Text.Encoding.CodePages.dll" copy /Y "%MANAGED_DIR%System.Text.Encoding.CodePages.dll" "%TARGET_DIR%System.Text.Encoding.CodePages.dll" >nul
if exist "%PLUGIN_ROOT%DEPENDENCIES.md" copy /Y "%PLUGIN_ROOT%DEPENDENCIES.md" "%TARGET_DIR%DEPENDENCIES.md" >nul
if exist "%PLUGIN_ROOT%SUPPORTED_FILE_TYPES.md" copy /Y "%PLUGIN_ROOT%SUPPORTED_FILE_TYPES.md" "%TARGET_DIR%SUPPORTED_FILE_TYPES.md" >nul

rem Copy any additional assemblies (for example, System.Memory) that Markdig
rem depends on so the runtime can load the Markdown renderer successfully.
pushd "%MANAGED_DIR%" >nul 2>&1
if not errorlevel 1 (
  for %%F in (*.dll) do (
    if /I not "%%~nxF"=="WebView2Loader.dll" (
      copy /Y "%%~fF" "%TARGET_DIR%%%~nxF" >nul
    )
  )
  popd >nul
)

endlocal
exit /B 0

:require
if exist "%~1" exit /B 0
echo %~2
exit /B 1
