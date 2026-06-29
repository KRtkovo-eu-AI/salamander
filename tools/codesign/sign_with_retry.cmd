@echo off
setlocal

rem Visual Studio post-build entry point. Signing is intentionally disabled here
rem by default because Certum/SimplySign may require an interactive PIN prompt.
rem Run tools\codesign\codesign_certum.cmd manually for release signing.

if /i not "%CODESIGN_ALLOW_POSTBUILD%"=="1" (
  echo Code signing skipped by post-build hook. Run tools\codesign\codesign_certum.cmd manually, or set CODESIGN_ALLOW_POSTBUILD=1.
  exit /b 0
)

set "SCRIPT_DIR=%~dp0"
call "%SCRIPT_DIR%codesign_certum.cmd" --file %*
exit /b %ERRORLEVEL%
